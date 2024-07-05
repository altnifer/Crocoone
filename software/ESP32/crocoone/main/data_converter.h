#ifndef DATA_CONVERTER_H
#define DATA_CONVERTER_H

#include "hccapx_serializer.h"
#include <stdint.h>

uint16_t hex_convert_to_buff(char * out_buff, const void * input_buff, const uint16_t input_buff_size);

char *hccapx_to_hc22000(hccapx_t * data);

char *pmkid_to_hc22000(uint8_t * pmkid, uint8_t * mac_ap, uint8_t * mac_client, uint8_t * essid, uint8_t essid_len);

#endif