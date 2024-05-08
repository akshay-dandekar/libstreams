#ifndef _ADIO_CONV_H_
#define _ADIO_CONV_H_

#include <stdint.h>

union break2
{
    uint8_t raw[2];
    uint8_t u8;
    int8_t i8;

    uint16_t u16;
    int16_t i16;
};

union break4
{
    uint8_t raw[4];
    uint8_t u8;
    int8_t i8;

    uint16_t u16;
    int16_t i16;

    uint32_t u32;
    int32_t i32;

    float f32;
};

#endif /* _ADIO_CONV_H_ */