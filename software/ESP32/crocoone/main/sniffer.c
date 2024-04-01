/**
 * @file sniffer.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements sniffer logic.
 */
#include "sniffer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

uint32_t tmpPacketCounter;
uint32_t deauths = 0;
int rssiSum;

static const char *TAG = "sniffer"; 

ESP_EVENT_DEFINE_BASE(SNIFFER_EVENTS);

/**
 * @brief Callback for promiscuous reciever. 
 * 
 * It forwards captured frames into event pool and sorts them based on their type
 * - Data
 * - Management
 * - Control
 * 
 * @param buf 
 * @param type 
 */
static void frame_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    ESP_LOGV(TAG, "Captured frame %d.", (int) type);

    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) buf;
    wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)frame->rx_ctrl;

    if (ctrl.sig_len > SNAP_LEN) return;           // packet too long

    int32_t event_id;
    if (type == WIFI_PKT_DATA) {
        event_id = SNIFFER_EVENT_CAPTURED_DATA;
    } else if (type == WIFI_PKT_MGMT) {
        event_id = SNIFFER_EVENT_CAPTURED_MGMT;
        if (frame->payload[0] == 0xA0 || frame->payload[0] == 0xC0 ) deauths++;
    } else if (type == WIFI_PKT_CTRL) {
        event_id = SNIFFER_EVENT_CAPTURED_CTRL;
    } else {
        return;
    }

    //uint32_t packetLength = ctrl.sig_len; //use for SD card log
    //if (type == WIFI_PKT_MGMT) packetLength -= 4;  // fix for known bug in the IDF https://github.com/espressif/esp-idf/issues/886

    tmpPacketCounter++;
    rssiSum += ctrl.rssi;

    ESP_ERROR_CHECK(esp_event_post(SNIFFER_EVENTS, event_id, frame, frame->rx_ctrl.sig_len + sizeof(wifi_promiscuous_pkt_t), portMAX_DELAY));
}

/**
 * @see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html#_CPPv425wifi_promiscuous_filter_t
 */
void wifictl_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl) {
    wifi_promiscuous_filter_t filter = { .filter_mask = 0 };
    if(data) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
    }
    else if(mgmt) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;
    }
    else if(ctrl) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL;
    }
    esp_wifi_set_promiscuous_filter(&filter);
}

void wifictl_sniffer_set_channel(uint8_t channel) {
    ESP_LOGI(TAG, "Starting promiscuous mode...");
    uint8_t ch = channel;
    if (ch > MAX_CH || ch < MIN_CH) ch = 1;
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous_rx_cb(&frame_handler);
    esp_wifi_set_promiscuous(true);
}

void wifictl_sniffer_stop() {
    ESP_LOGI(TAG, "Stopping promiscuous mode...");
    esp_wifi_set_promiscuous(false);
}