#ifndef HC22000_CONVERTER_H
#define HC22000_CONVERTER_H

#include "hccapx_serializer.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#define PROTOCOL "WPA"
#define PMKID_TYPE "01"
#define EAPOL_TYPE "02"

char * hccapx_to_hc22000(hccapx_t * data);

char * pmkid_to_hc22000(uint8_t * pmkid, uint8_t * mac_ap, uint8_t * mac_client, uint8_t * essid, uint8_t essid_len);

unsigned int hc22000_size_get();

#endif