#ifndef _APP_UTILS_READER_H_
#define _APP_UTILS_READER_H_

#include <stdint.h>

struct app_reader_ctx
{
    uint8_t res[100];
};

int app_reader_init(struct app_reader_ctx *ectx, uint8_t *buf, int len);

int app_reader_add_bytes(struct app_reader_ctx *ctx, uint8_t *buf, int len);

int app_reader_read(struct app_reader_ctx *ctx, uint8_t *buf, int len);

int app_reader_available(struct app_reader_ctx *ctx);

int app_reader_flush(struct app_reader_ctx *ctx);

#endif /* _APP_UTILS_READER_H_ */