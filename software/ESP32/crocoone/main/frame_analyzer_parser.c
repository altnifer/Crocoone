/**
 * @file frame_analyzer_parser.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements parsing functionality
 */
#include "frame_analyzer_parser.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "arpa/inet.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_wifi_types.h"

#include "frame_analyzer_types.h"

static const char *TAG = "frame_analyzer:parser";

ESP_EVENT_DEFINE_BASE(FRAME_ANALYZER_EVENTS);

/**
 * @brief Debug function to print raw frame to serial
 * 
 * @param frame 
 */
void print_raw_frame(const wifi_promiscuous_pkt_t *frame){
    for(unsigned i = 0; i < frame->rx_ctrl.sig_len; i++) {
        printf("%02x", frame->payload[i]);
    }
    printf("\n");
}

/**
 * @brief Debug functions to print MAC address from given buffer to serial
 * 
 * @param a mac address buffer
 */
void print_mac_address(const uint8_t *a){
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
    a[0], a[1], a[2], a[3], a[4], a[5]);
    printf("\n");
}

bool is_frame_bssid_matching(wifi_promiscuous_pkt_t *frame, uint8_t *bssid) {
    data_frame_mac_header_t *mac_header = (data_frame_mac_header_t *) frame->payload;
    return memcmp(mac_header->addr3, bssid, 6) == 0;
}

eapol_packet_t *parse_eapol_packet(data_frame_t *frame) {
    uint8_t *frame_buffer = frame->body;

    if(frame->mac_header.frame_control.protected_frame == 1) {
        ESP_LOGV(TAG, "Protected frame, skipping...");
        return NULL;
    }

    if(frame->mac_header.frame_control.subtype > 7) {
        ESP_LOGV(TAG, "QoS data frame");
        // Skipping QoS field (2 bytes)
        frame_buffer += 2;
    }

    // Skipping LLC SNAP header (6 bytes)
    frame_buffer += sizeof(llc_snap_header_t);

    // Check if frame is type of EAPoL
    if(ntohs(*(uint16_t *) frame_buffer) == ETHER_TYPE_EAPOL) {
        ESP_LOGD(TAG, "EAPOL packet");
        frame_buffer += 2;
        return (eapol_packet_t *) frame_buffer; 
    }
    return NULL;
}

eapol_key_packet_t *parse_eapol_key_packet(eapol_packet_t *eapol_packet) {
    if(eapol_packet->header.packet_type != EAPOL_KEY){
        ESP_LOGD(TAG, "Not an EAPoL-Key packet.");
        return NULL;
    }
    return (eapol_key_packet_t *) eapol_packet->packet_body;
}

key_data_t *parse_pmkid(eapol_key_packet_t *eapol_key) {
    if(eapol_key->key_data_length == 0){
        printf("Empty Key Data\n");
        return NULL;
    }

    if(eapol_key->key_information.encrypted_key_data == 1){
        ESP_LOGD(TAG, "Key Data encrypted");
        return NULL;
    }

    key_data_t *key_data = (key_data_t *)eapol_key->key_data;
    uint8_t data_type = key_data->key_data_type;
    uint8_t *oui = key_data->oui;
    uint8_t oui_type = key_data->oui_type;
    uint8_t *pmkid = key_data->pmkid;

    if (data_type != KEY_DATA_TYPE) {
        printf("{PMKID,WRONG_KEY_TYPE,%02X,}\n", data_type);
        return NULL;
    }

    for (uint8_t i = 0; i < 3; i++) {
        if (oui[i] != KEY_DATA_OUI[i]) {
            printf("{PMKID,WRONG_OUI,%02X%02X%02X,}\n", oui[0], oui[1], oui[2]);
            return NULL;
        }
    }

    if (oui_type != KEY_DATA_DATA_TYPE_PMKID_KDE) {
        printf("{PMKID,WRONG_KDE,%02X,}\n", oui_type);
        return NULL;
    }

    bool empty_pmkid = 1;
    for (uint8_t i = 0; i < 16; i++) {
        if (pmkid[i] != 0) empty_pmkid = 0;
    }

    if (empty_pmkid) return NULL;
    
    return key_data;
}