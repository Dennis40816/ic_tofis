#include "tofis_input_parser.h"
#include "tofis_host_api.h"

#include <string.h>

void parse_to_cmd_buf(char *user_input_section, uint8_t *to_tofis_buf,
                      size_t *buf_len) {
  size_t len = strlen(user_input_section);
  for (size_t i = 0; i < len && i < 256; i++) {
    to_tofis_buf[i] = (uint8_t)user_input_section[i];
  }
  *buf_len = len;
}