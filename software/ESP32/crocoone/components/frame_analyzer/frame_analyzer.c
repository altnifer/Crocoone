/**
 * @file frame_analyzer.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements frame analysis
 */
#include "frame_analyzer.h"

#include <stdint.h>
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"

#include "wifi_controller.h"
#include "frame_analyzer_parser.h"
#include "frame_analyzer_types.h"

ESP_EVENT_DEFINE_BASE(FRAME_ANALYZER_EVENTS);

static uint8_t target_bssid[6];
static search_type_t search_type = -1;

/**
 * @brief Analyzes data frames from sniffer.
 *  
 * @param args 
 * @param event_base 
 * @param event_id 
 * @param event_data 
 */
static void data_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;

    if (!is_frame_bssid_matching(frame, target_bssid)) {
        return;
    }

    eapol_packet_t *eapol_packet = parse_eapol_packet((data_frame_t *) frame->payload);
    if (eapol_packet == NULL) {
        return;
    }

    eapol_key_packet_t *eapol_key_packet = parse_eapol_key_packet(eapol_packet);
    if (eapol_key_packet == NULL) {
        return;
    }

    if (search_type == SEARCH_HANDSHAKE) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_post(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, frame, sizeof(wifi_promiscuous_pkt_t) + frame->rx_ctrl.sig_len, portMAX_DELAY));
        return;
    }

    if (search_type == SEARCH_PMKID) {
        key_data_t *key_data;
        if((key_data = parse_pmkid(eapol_key_packet)) == NULL){
            return;
        }
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_post(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_PMKID, key_data, sizeof(key_data_t), portMAX_DELAY));
        return;
    }
}

void frame_analyzer_capture_start(search_type_t search_type_arg, const uint8_t *bssid) {
    search_type = search_type_arg;
    if (bssid != NULL)
        memcpy(&target_bssid, bssid, 6);
    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_DATA, &data_frame_handler, NULL));
}

void frame_analyzer_capture_stop() {
    ESP_ERROR_CHECK(esp_event_handler_unregister(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_DATA, &data_frame_handler));
}

void frame_analyzer_change_bssid_filter(const uint8_t *bssid) {
    memcpy(&target_bssid, bssid, 6);
}