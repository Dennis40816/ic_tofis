#pragma once

#include <stdint.h>

void parse_to_cmd_buf(char *user_input_section, uint8_t *to_tofis_buf,
                      size_t *buf_len);