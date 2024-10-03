#include "check_sum.h"

// should pass pointer point to tofis_data_packet_t.data 
uint8_t calculate_checksum(uint8_t *buffer, int length) {
    uint8_t checksum = 0;
    for(int i = 0; i < length; i++) {
        checksum ^= buffer[i];
    }
    return checksum;
}