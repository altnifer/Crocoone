#include "hc22000_converter.h"

static char ch22000_buffer[1024] = {};
static unsigned int ch22000_len = 0;

uint16_t hex_converter(char * out_buff, const void * input_buff, const uint16_t input_buff_size) {
    uint16_t len = 0;
    for (uint16_t i = 0; i < input_buff_size; i++) {
        sprintf(out_buff + len, "%02x", *((uint8_t *)(input_buff + i)));
        len = strlen(out_buff);
    }
    return len;
}

//WPA*02*MIC*MAC_AP*MAC_CLIENT*ESSID*NONCE_AP*EAPOL_CLIENT*MESSAGEPAIR

char * hccapx_to_hc22000(hccapx_t * data) {
    if (data == NULL) return NULL;

    memset(ch22000_buffer, 0, 1024);
    ch22000_len = 0;

    sprintf(ch22000_buffer, "%s*%s*", PROTOCOL, EAPOL_TYPE);
    ch22000_len = strlen(ch22000_buffer);

    ch22000_len += hex_converter(ch22000_buffer + ch22000_len, data->keymic, 16*sizeof(char));
    *(ch22000_buffer + ch22000_len) = '*';
    ch22000_len++;

    ch22000_len += hex_converter(ch22000_buffer + ch22000_len, data->mac_ap, 6*sizeof(char));
    *(ch22000_buffer + ch22000_len) = '*';
    ch22000_len++;

    ch22000_len += hex_converter(ch22000_buffer + ch22000_len, data->mac_sta, 6*sizeof(char));
    *(ch22000_buffer + ch22000_len) = '*';
    ch22000_len++;

    ch22000_len += hex_converter(ch22000_buffer + ch22000_len, data->essid, data->essid_len*sizeof(char));
    *(ch22000_buffer + ch22000_len) = '*';
    ch22000_len++;

    ch22000_len += hex_converter(ch22000_buffer + ch22000_len, data->nonce_ap, 32*sizeof(char));
    *(ch22000_buffer + ch22000_len) = '*';
    ch22000_len++;

    ch22000_len += hex_converter(ch22000_buffer + ch22000_len, data->eapol, data->eapol_len*sizeof(char));
    *(ch22000_buffer + ch22000_len) = '*';
    ch22000_len++;

    sprintf(ch22000_buffer + ch22000_len, "%02x", data->message_pair);
    ch22000_len = strlen(ch22000_buffer);

    return ch22000_buffer;
}

//WPA*01*PMKID*MAC_AP*MAC_CLIENT*ESSID***MESSAGEPAIR

char * pmkid_to_hc22000(uint8_t * pmkid, uint8_t * mac_ap, uint8_t * mac_client, uint8_t * essid, uint8_t essid_len) {
    if (pmkid == NULL) return NULL;

    memset(ch22000_buffer, 0, 1024);
    ch22000_len = 0;

    sprintf(ch22000_buffer, "%s*%s*", PROTOCOL, PMKID_TYPE);
    ch22000_len = strlen(ch22000_buffer);

    ch22000_len += hex_converter(ch22000_buffer + ch22000_len, pmkid, 16*sizeof(char));
    *(ch22000_buffer + ch22000_len) = '*';
    ch22000_len++;

    ch22000_len += hex_converter(ch22000_buffer + ch22000_len, mac_ap, 6*sizeof(char));
    *(ch22000_buffer + ch22000_len) = '*';
    ch22000_len++;

    ch22000_len += hex_converter(ch22000_buffer + ch22000_len, mac_client, 6*sizeof(char));
    *(ch22000_buffer + ch22000_len) = '*';
    ch22000_len++;

    ch22000_len += hex_converter(ch22000_buffer + ch22000_len, essid, essid_len);
    sprintf(ch22000_buffer + ch22000_len, "***01");
    ch22000_len += 5;

    return ch22000_buffer;
}

unsigned int hc22000_size_get() {
    return ch22000_len;
}