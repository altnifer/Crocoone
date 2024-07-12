#ifndef DATA_CONVERTER_H
#define DATA_CONVERTER_H

#include "hccapx_serializer.h"
#include <stdint.h>

/*
                    WPA   *  01   *  PMKID   *   MAC_AP   *   MAC_CLIENT   *   ESSID  ***  MESSAGEPAIR
MAX_HC22000_01_LEN =  3  +1  +2  +1  +16*2  +1     +6*2  +1         +6*2  +1   +32*2   +3           +2
*/
#define MAX_HC22000_01_LEN 135

/*
                    WPA   *  02   *    MIC   *   MAC_AP   *   MAC_CLIENT   *   ESSID   *   NONCE_AP   *   EAPOL_CLIENT   *   MESSAGEPAIR
MAX_HC22000_02_LEN =  3  +1  +2  +1  +16*2  +1     +6*2  +1         +6*2  +1   +32*2  +1      +32*2  +1         +256*2  +1          +1*2
*/
#define MAX_HC22000_02_LEN 711

int32_t hccapx_to_hc22000(char *out_buff, hccapx_t * data);
int32_t pmkid_to_hc22000(char *out_buff, uint8_t * pmkid, uint8_t * mac_ap, uint8_t * mac_client, uint8_t * essid, uint8_t essid_len);

#endif