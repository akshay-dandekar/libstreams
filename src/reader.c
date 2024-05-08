#include <string.h>
#include <pthread.h>

#include "log.h"

#include "reader.h"

struct _app_reader_ctx
{
    uint8_t *buf;
    int max_len;
    int cur_len;
    int idx;

    pthread_mutex_t lock;
};

static void _lock(struct _app_reader_ctx *ctx);
static void _unlock(struct _app_reader_ctx *ctx);

int app_reader_init(struct app_reader_ctx *ectx, uint8_t *buf, int len)
{
    struct _app_reader_ctx *ctx = (struct _app_reader_ctx *)ectx;

    ctx->buf = buf;
    ctx->max_len = len;
    ctx->cur_len = 0;
    ctx->idx = 0;

    if (pthread_mutex_init(&ctx->lock, NULL) != 0)
    {
        log_error(0, "\n modbus mutex init has failed\n");
        return 1;
    }

    return 0;
}

int app_reader_add_bytes(struct app_reader_ctx *ectx, uint8_t *buf, int len)
{
    int wrLen, wrTemp, wrIdxToEnd;
    int availableBuffer;

    struct _app_reader_ctx *ctx = (struct _app_reader_ctx *)ectx;

    _lock(ctx);

    availableBuffer = ctx->max_len - ctx->cur_len;
    wrLen = availableBuffer > len ? len : availableBuffer;

    wrIdxToEnd = ctx->max_len - ctx->idx;
    if (wrIdxToEnd > wrLen)
    {
        memcpy(&ctx->buf[ctx->idx], buf, wrLen);
        ctx->idx += wrLen;
    }
    else
    {
        wrTemp = wrIdxToEnd;
        memcpy(&ctx->buf[ctx->idx], buf, wrTemp);

        wrTemp = (wrLen - wrIdxToEnd);
        memcpy(&ctx->buf[0], &buf[wrIdxToEnd], wrTemp);
        ctx->idx = wrTemp;
    }

    ctx->cur_len += wrLen;

    _unlock(ctx);

    return wrLen;
}

int app_reader_available(struct app_reader_ctx *ectx)
{
    struct _app_reader_ctx *ctx = (struct _app_reader_ctx *)ectx;

    return ctx->cur_len;
}

int app_reader_read(struct app_reader_ctx *ectx, uint8_t *buf, int len)
{
    int rdLen, rdTemp, rdIdxToStart;
    int offset;

    struct _app_reader_ctx *ctx = (struct _app_reader_ctx *)ectx;

    _lock(ctx);
    rdLen = ctx->cur_len > len ? len : ctx->cur_len;
    rdIdxToStart = ctx->idx - rdLen;

    if (rdIdxToStart < 0)
    {
        offset = 0;
        rdTemp = rdIdxToStart * -1;
        memcpy(&buf[offset], &ctx->buf[ctx->max_len - rdTemp], rdTemp);
        offset += rdTemp;

        rdTemp = rdLen - rdTemp;
        memcpy(&buf[offset], &ctx->buf, rdTemp);
        offset += rdTemp;
    }
    else
    {
        memcpy(buf, &ctx->buf[rdIdxToStart], rdLen);
    }

    ctx->cur_len -= rdLen;
    if (ctx->cur_len <= 0)
    {
        ctx->idx = 0;
    }

    _unlock(ctx);

    return rdLen;
}

int app_reader_flush(struct app_reader_ctx *ectx)
{
    struct _app_reader_ctx *ctx = (struct _app_reader_ctx *)ectx;

    _lock(ctx);

    memset(ctx->buf, 0, ctx->max_len);

    ctx->cur_len = 0;
    ctx->idx = 0;

    _unlock(ctx);

    return 0;
}

static void _lock(struct _app_reader_ctx *ctx)
{
    int ret;

    do
    {
        ret = pthread_mutex_lock(&ctx->lock);
        if (ret == 0)
        {
            break;
        }
        log_error(0, "Lock not acquired by %d: %d\r\n", pthread_self());

    } while (1);

    // log_info(0, "Lock acquired by %d\r\n", pthread_self());
}

static void _unlock(struct _app_reader_ctx *ctx)
{
    int ret;

    do
    {
        ret = pthread_mutex_unlock(&ctx->lock);
        if (ret == 0)
        {
            break;
        }
        log_error(0, "Lock not released by %d: %d\r\n", pthread_self());

    } while (1);

    // log_info(0, "Lock released by %d\r\n", pthread_self());
}
