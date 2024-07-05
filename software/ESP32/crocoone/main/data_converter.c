#include "data_converter.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

const static char *EAPOL_TYPE = "WPA*02*";
const static char *PMKID_TYPE = "WPA*01*";
const static char *alph = "0123456789abcdef";

// out_buff must be greater than input_buff_size * 2
uint16_t hex_convert_to_buff(char * out_buff, const void * input_buff, const uint16_t input_buff_size) {
    uint8_t *buffer_byte = (uint8_t *)input_buff;
    for (uint16_t i = 0; i < input_buff_size; i++) {
        out_buff[i * 2] = alph[(buffer_byte[i] / (0xf + 1)) % (0xf + 1)];
        out_buff[i * 2 + 1] = alph[buffer_byte[i] % (0xf + 1)];
    }
    return input_buff_size*2;
}

//WPA*02*MIC*MAC_AP*MAC_CLIENT*ESSID*NONCE_AP*EAPOL_CLIENT*MESSAGEPAIR
char *hccapx_to_hc22000(hccapx_t * data) {
    if (data == NULL) return NULL;

    unsigned int hc22000_len = 135 + 2*data->essid_len + 2*data->eapol_len + 1;
    char *hc22000 = (char *)malloc(hc22000_len);
    char *hc22000_ans = hc22000;
    memcpy(hc22000, EAPOL_TYPE, 7);
    hc22000 += 7;
    hc22000 += hex_convert_to_buff(hc22000, data->keymic, 16);
    *hc22000 = '*';
    hc22000++;
    hc22000 += hex_convert_to_buff(hc22000, data->mac_ap, 6);
    *hc22000 = '*';
    hc22000++;
    hc22000 += hex_convert_to_buff(hc22000, data->mac_sta, 6);
    *hc22000 = '*';
    hc22000++;
    hc22000 += hex_convert_to_buff(hc22000, data->essid, data->essid_len);
    *hc22000 = '*';
    hc22000++;
    hc22000 += hex_convert_to_buff(hc22000, data->nonce_ap, 32);
    *hc22000 = '*';
    hc22000++;
    hc22000 += hex_convert_to_buff(hc22000, data->eapol, data->eapol_len);
    *hc22000 = '*';
    hc22000++;
    hc22000 += hex_convert_to_buff(hc22000, &data->message_pair, 1);
    *hc22000 = '\0';
    return hc22000_ans;
}

//WPA*01*PMKID*MAC_AP*MAC_CLIENT*ESSID***MESSAGEPAIR
char *pmkid_to_hc22000(uint8_t * pmkid, uint8_t * mac_ap, uint8_t * mac_client, uint8_t * essid, uint8_t essid_len) {
    if (pmkid == NULL) return NULL;

    unsigned int hc22000_len = 71 + 2*essid_len + 1;
    char *hc22000 = (char *)malloc(hc22000_len);
    char *hc22000_ans = hc22000;
    memcpy(hc22000, PMKID_TYPE, 7);
    hc22000 += 7;
    hc22000 += hex_convert_to_buff(hc22000, pmkid, 16);
    *hc22000 = '*';
    hc22000++;
    hc22000 += hex_convert_to_buff(hc22000, mac_ap, 6);
    *hc22000 = '*';
    hc22000++;
    hc22000 += hex_convert_to_buff(hc22000, mac_client, 6);
    *hc22000 = '*';
    hc22000++;
    hc22000 += hex_convert_to_buff(hc22000, essid, essid_len);
    memcpy(hc22000, (char *)"***01", 5);
    hc22000 += 5;
    *hc22000 = '\0';
    return hc22000_ans;
}