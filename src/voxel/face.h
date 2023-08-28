#pragma once
#include <stddef.h>
#include <stdint.h>

typedef union {
    uint32_t info;
    struct {
        uint8_t x;
        uint8_t y;
        uint8_t z;
    } position;
} face_t;