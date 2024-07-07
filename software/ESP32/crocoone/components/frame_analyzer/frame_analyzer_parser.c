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

#include "esp_wifi_types.h"

#include "frame_analyzer_types.h"

bool is_frame_bssid_matching(wifi_promiscuous_pkt_t *frame, uint8_t *bssid) {
    data_frame_mac_header_t *mac_header = (data_frame_mac_header_t *) frame->payload;
    return memcmp(mac_header->addr3, bssid, 6) == 0;
}

eapol_packet_t *parse_eapol_packet(data_frame_t *frame) {
    uint8_t *frame_buffer = frame->body;

    if(frame->mac_header.frame_control.protected_frame == 1) {
        return NULL;
    }

    if(frame->mac_header.frame_control.subtype > 7) {
        // Skipping QoS field (2 bytes)
        frame_buffer += 2;
    }

    // Skipping LLC SNAP header (6 bytes)
    frame_buffer += sizeof(llc_snap_header_t);

    // Check if frame is type of EAPoL
    if(ntohs(*(uint16_t *) frame_buffer) == ETHER_TYPE_EAPOL) {
        frame_buffer += 2;
        return (eapol_packet_t *) frame_buffer; 
    }
    return NULL;
}

eapol_key_packet_t *parse_eapol_key_packet(eapol_packet_t *eapol_packet) {
    if(eapol_packet->header.packet_type != EAPOL_KEY){
        return NULL;
    }
    return (eapol_key_packet_t *) eapol_packet->packet_body;
}

key_data_t *parse_pmkid(eapol_key_packet_t *eapol_key) {
    if(eapol_key->key_data_length == 0){
        return NULL;
    }

    if(eapol_key->key_information.encrypted_key_data == 1){
        return NULL;
    }

    key_data_t *key_data = (key_data_t *)eapol_key->key_data;
    uint8_t data_type = key_data->key_data_type;
    uint8_t *oui = key_data->oui;
    uint8_t oui_type = key_data->oui_type;
    uint8_t *pmkid = key_data->pmkid;

    if (data_type != KEY_DATA_TYPE) {
        return NULL;
    }

    for (uint8_t i = 0; i < 3; i++) {
        if (oui[i] != KEY_DATA_OUI[i]) {
            return NULL;
        }
    }

    if (oui_type != KEY_DATA_DATA_TYPE_PMKID_KDE) {
        return NULL;
    }

    bool empty_pmkid = 1;
    for (uint8_t i = 0; i < 16; i++) {
        if (pmkid[i] != 0) empty_pmkid = 0;
    }

    if (empty_pmkid) return NULL;
    
    return key_data;
}