#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libserialport.h>
#include <signal.h>

#include <pthread.h>

#include <winsock2.h>
#include <windows.h>

#include "reader.h"
#include "log.h"

volatile sig_atomic_t _g_is_interrupted = 0;

static struct sp_port *port;

static struct app_reader_ctx _g_reader_ctx;

/* Helper function for error handling. */
int check(enum sp_return result);

static void handle_sigint(int signo)
{
    _g_is_interrupted = 1;
}

void print_buf(uint8_t *buf, int len)
{
    int i, offset;
    char strTemp[1024];

    /* If length is zero or less return as no data to write */
    if (len < 1)
    {
        return;
    }

    offset = 0;
    /* Print comma separated HEX data to stdout till second last byte */
    for (i = 0; i < (len - 1); i++)
    {
        offset += snprintf(&strTemp[offset], sizeof(strTemp) - offset, "%02X, ", buf[i]);
    }

    /* Print last byte followed by a newline */
    offset += snprintf(&strTemp[offset], sizeof(strTemp) - offset, "%02X\r\n", buf[i]);

    log_info(0, "%s", strTemp);
}

void write_stdout(uint8_t *buf, int len)
{
    print_buf(buf, len);

    sp_blocking_write(port, buf, len, 1000);
}

void _parse_args(int argc, char *argv[], char **port)
{
    if (argc != 2)
    {
        log_error(0, "Provide Serial Port name\r\n");
        exit(1);
    }

    /* Set Serial port to the first argument */
    *port = argv[1];
}

void *th_serial_main(void *ptr)
{
    char *port_name = (char *)ptr;
    int ret, read_bytes = 0;
    int errTimeout;
    static uint8_t buf[1024];
    static enum _sp_state {

        SP_STATE_IDLE,
        SP_STATE_OPENING,
        SP_STATE_WAIT_READING,
        SP_STATE_READING,
        SP_STATE_ERROR,
    } _state;

    _state = SP_STATE_OPENING;
    read_bytes = 0;

    while (_g_is_interrupted == 0)
    {
        switch (_state)
        {
        case SP_STATE_OPENING:
        {
            /* Init Serial port */
            log_info(0, "Looking for port %s.\n", port_name);
            if (check(sp_get_port_by_name(port_name, &port)) != SP_OK)
            {
                errTimeout = 100;
                _state = SP_STATE_ERROR;
                break;
            }
            log_info(0, "Opening port.\n");

            if (check(sp_open(port, SP_MODE_READ_WRITE)) != SP_OK)
            {
                errTimeout = 100;
                _state = SP_STATE_ERROR;
                break;
            }

            log_info(0, "Setting port to 9600 8N1, no flow control.\n");
            if (check(sp_set_baudrate(port, 9600)) != SP_OK)
            {
                errTimeout = 100;
                _state = SP_STATE_ERROR;
                break;
            }

            if (check(sp_set_bits(port, 8)) != SP_OK)
            {
                errTimeout = 100;
                _state = SP_STATE_ERROR;
                break;
            }

            if (check(sp_set_parity(port, SP_PARITY_NONE)) != SP_OK)
            {
                errTimeout = 100;
                _state = SP_STATE_ERROR;
                break;
            }

            if (check(sp_set_stopbits(port, 1)) != SP_OK)
            {
                errTimeout = 100;
                _state = SP_STATE_ERROR;
                break;
            }

            if (check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE)) != SP_OK)
            {
                errTimeout = 100;
                _state = SP_STATE_ERROR;
                break;
            }

            _state = SP_STATE_WAIT_READING;
        }
        break;
        case SP_STATE_WAIT_READING:
        {
            ret = sp_blocking_read_next(port, buf, sizeof(buf), 10);
            if (ret < 0)
            {
                log_error(0, "Error reading from port: %d\r\n", ret);
                errTimeout = 100;
                _state = SP_STATE_ERROR;
                break;
            }
            if (ret == 0)
            {
                break;
            }
            read_bytes += ret;
            _state = SP_STATE_READING;
        }
        break;
        case SP_STATE_READING:
        {
            ret = sp_blocking_read(port, &buf[read_bytes], sizeof(buf) - read_bytes, 1);
            if (ret < 0)
            {
                log_error(0, "Error reading from port: %d\r\n", ret);
                errTimeout = 100;
                _state = SP_STATE_ERROR;
                break;
            }
            if (ret != 0)
            {
                read_bytes += ret;
                continue;
            }

            log_info(0, "Received %d bytes\r\n", read_bytes);
            // print_buf(buf, read_bytes);

            app_reader_add_bytes(&_g_reader_ctx, buf, read_bytes);

            read_bytes = 0;
            _state = SP_STATE_WAIT_READING;
        }
        break;
        case SP_STATE_ERROR:
        {
            if (errTimeout > 0)
            {
                errTimeout--;
                usleep(10000);
                break;
            }
            _state = SP_STATE_OPENING;
            break;
        }
        break;
        default:
            break;
        }

        // Check for arriving connections on the listening socket.
    }
}

int main(int argc, char *argv[])
{
    FILE *fp;
    uint16_t regData[10];
    uint8_t sp_buf[1024];
    uint8_t rd_buf[2048];

    char *port_name;
    pthread_t th_serial;
    int sleepCnt = 0;
    int ret, count, offset, readCnt;
    int total_read;

    if (signal(SIGINT, NULL) == SIG_ERR)
    {
        perror("signal()");
        return EXIT_FAILURE;
    }

    log_set_level(LOG_DEBUG);

    /* Parse arguments */
    _parse_args(argc, argv, &port_name);

    /* Init reader context */
    ret = app_reader_init(&_g_reader_ctx, sp_buf, sizeof(sp_buf));
    log_info(0, "Reader init returned %d", ret);

    pthread_create(&th_serial, NULL, &th_serial_main, (void *)port_name);

    if (signal(SIGINT, handle_sigint) == SIG_ERR)
    {
        perror("signal()");
        return EXIT_FAILURE;
    }

    sleepCnt = 100;
    count = 10;
    offset = 0;
    readCnt = 0;

    total_read = 0;

    fp = fopen("myfile.bin", "wb");
    if (fp == NULL)
    {
        _g_is_interrupted = 1;

        log_error(0, "Error opening file\n");
        return 1;
    }

    while (_g_is_interrupted == 0)
    {

        if (sleepCnt > 0)
        {
            sleepCnt--;
            usleep(100);
            continue;
        }

        sleepCnt = 100;

        readCnt = app_reader_available(&_g_reader_ctx);
        if (readCnt > 0)
        {
            readCnt = app_reader_read(&_g_reader_ctx, rd_buf, sizeof(rd_buf));
            if (readCnt > 0)
            {
                offset += readCnt;
                count = 10;
            }
        }

        if (offset > 0)
        {
            total_read += offset;

            log_info(0, "Read %d bytes from reader, total read: %d", offset, total_read);
            print_buf(rd_buf, offset);

            fwrite(rd_buf, 1, offset, fp);

            count = 10;
            offset = 0;
            readCnt = 0;
        }
    }

        log_info(0, "Interrupt received\r\n");

    fclose(fp);

    return 0;
}

/* Helper function for error handling. */
int check(enum sp_return result)
{
    /* For this example we'll just exit on any error by calling abort(). */
    char *error_message;
    switch (result)
    {
    case SP_ERR_ARG:
        log_error(0, "Error: Invalid argument.\n");
        // abort();
        break;
    case SP_ERR_FAIL:
        error_message = sp_last_error_message();
        log_error(0, "Error: Failed: %s\n", error_message);
        sp_free_error_message(error_message);
        // abort();
        break;
    case SP_ERR_SUPP:
        log_error(0, "Error: Not supported.\n");
        // abort();
        break;
    case SP_ERR_MEM:
        log_error(0, "Error: Couldn't allocate memory.\n");
        // abort();
        break;
    case SP_OK:
    default:
        return result;
    }

    return result;
}
