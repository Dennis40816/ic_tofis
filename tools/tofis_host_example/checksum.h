// checksum.h
#pragma once

#include <stdint.h>

static inline uint8_t calculate_checksum(uint8_t *buffer, int length) {
    uint8_t checksum = 0;
    for(int i = 0; i < length; i++) {
        checksum ^= buffer[i];
    }
    return checksum;
}
