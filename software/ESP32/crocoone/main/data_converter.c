#include "data_converter.h"
#include <stdio.h>
#include <string.h>

const static char *EAPOL_TYPE = "WPA*02*";
const static char *PMKID_TYPE = "WPA*01*";
const static char *alph = "0123456789abcdef";

//out_buff size must be greater than input_buff_size * 2 + 1!
uint16_t hex_convert_to_buff(char * out_buff, const void * input_buff, const uint16_t input_buff_size) {
    uint8_t *buffer_byte = (uint8_t *)input_buff;
    for (uint16_t i = 0; i < input_buff_size; i++) {
        out_buff[i * 2] = alph[(buffer_byte[i] / (0xf + 1)) % (0xf + 1)];
        out_buff[i * 2 + 1] = alph[buffer_byte[i] % (0xf + 1)];
    }
    out_buff[input_buff_size * 2] = '\0';
    return input_buff_size * 2; //hex data len
}

//WPA*02*MIC*MAC_AP*MAC_CLIENT*ESSID*NONCE_AP*EAPOL_CLIENT*MESSAGEPAIR
//out_buff size must be greater than MAX_HC22000_02_LEN + 1!
int32_t hccapx_to_hc22000(char *out_buff, hccapx_t * data) {
    if (data == NULL || out_buff == NULL) return -1;

    char *out_buff_p = out_buff;
    memcpy(out_buff_p, EAPOL_TYPE, 7);
    out_buff_p += 7;
    out_buff_p += hex_convert_to_buff(out_buff_p, data->keymic, 16);
    *out_buff_p++ = '*';
    out_buff_p += hex_convert_to_buff(out_buff_p, data->mac_ap, 6);
    *out_buff_p++ = '*';
    out_buff_p += hex_convert_to_buff(out_buff_p, data->mac_sta, 6);
    *out_buff_p++ = '*';
    out_buff_p += hex_convert_to_buff(out_buff_p, data->essid, data->essid_len);
    *out_buff_p++ = '*';
    out_buff_p += hex_convert_to_buff(out_buff_p, data->nonce_ap, 32);
    *out_buff_p++ = '*';
    out_buff_p += hex_convert_to_buff(out_buff_p, data->eapol, data->eapol_len);
    *out_buff_p++ = '*';
    out_buff_p += hex_convert_to_buff(out_buff_p, &data->message_pair, 1);

    return (int32_t)(out_buff_p - out_buff); //hc22000 length
}

//WPA*01*PMKID*MAC_AP*MAC_CLIENT*ESSID***MESSAGEPAIR
//out_buff size must be greater than MAX_HC22000_01_LEN + 1!
int32_t pmkid_to_hc22000(char *out_buff, uint8_t * pmkid, uint8_t * mac_ap, uint8_t * mac_client, uint8_t * essid, uint8_t essid_len) {
    if (pmkid == NULL || out_buff == NULL) return -1;

    char *out_buff_p = out_buff;
    memcpy(out_buff_p, PMKID_TYPE, 7);
    out_buff_p += 7;
    out_buff_p += hex_convert_to_buff(out_buff_p, pmkid, 16);
    *out_buff_p++ = '*';
    out_buff_p += hex_convert_to_buff(out_buff_p, mac_ap, 6);
    *out_buff_p = '*';
    out_buff_p += hex_convert_to_buff(out_buff_p, mac_client, 6);
    *out_buff_p++ = '*';
    out_buff_p += hex_convert_to_buff(out_buff_p, essid, essid_len);
    memcpy(out_buff_p, (char *)"***01", 6);

    return (int32_t)(out_buff_p - out_buff); //hc22000 length
}