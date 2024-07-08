#include "packet_monitor.h"
#include "wifi_controller.h"
#include "pcap_serializer.h"
#include "esp_timer.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "esp_event.h"
#include "uart_controller.h"
#include "frame_analyzer_parser.h"
#if (ASCII_HEX_FORMAT == 1)
#include "data_converter.h"
#endif

#define MIN_UART_WRITE_TIMEOUT_MS 100
//MONITOR_BUFF_LEN = MAX_DATA_LEN + strlen("PCAP data (000 pkts, 0000 bytes): ") + '\n'
//strlen("PCAP data (000 pkts, 0000 bytes): ") = 34
#define MONITOR_BUFF_LEN 4096

static const uint16_t frame_header_len = 34;
//strlen("PCAP data (000 pkts, 0000 bytes): ") = 34

static char monitor_buff[MONITOR_BUFF_LEN] = {};
static char *curr_mon_buff_p = NULL;
static bool mon_buff_is_empty = true;
static bool packet_sender_task_handler = false;
static uint16_t packet_counter;
static bool use_bssid_filter;
static uint8_t bssid_filter[6];
static bool uart_busy = false;

void packet_sender_task(void *args);
static void sniffer_events_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data);
void frame_header_update(uint16_t pkts, uint16_t bytes);


bool packet_monitor_start(uint8_t channel, uint8_t filter_bitmask, uint8_t *bssid) {
    if ((channel < MIN_CH || channel > MAX_CH) || (filter_bitmask < MIN_BIT_MASK_FILTER || filter_bitmask > MAX_BIT_MASK_FILTER))
        return false;

    if (packet_sender_task_handler)
        packet_monitor_stop();

    packet_counter = 0;
    curr_mon_buff_p = monitor_buff + frame_header_len;
    mon_buff_is_empty = false;

    if (bssid != NULL) {
        memcpy(bssid_filter, bssid, 6);
        use_bssid_filter = true;
    } else {
        use_bssid_filter = false;
    }

    pcap_global_header_t pcap_global_header = {
        .magic_number = PCAP_MAGIC_NUMBER,
        .version_major = 2,
        .version_minor = 4,
        .thiszone = 0,
        .sigfigs = 0,
        .snaplen = SNAPLEN,
        .network = LINKTYPE_IEEE802_11
    };

    #if (ASCII_HEX_FORMAT == 1)
    curr_mon_buff_p += hex_convert_to_buff(curr_mon_buff_p, &pcap_global_header, sizeof(pcap_global_header));
    #elif (ASCII_HEX_FORMAT == 0)
    memcpy(curr_mon_buff_p, &pcap_global_header, sizeof(pcap_global_header));
    curr_mon_buff_p += sizeof(pcap_global_header);
    #endif

    bool ctrl = filter_bitmask % 2;
    filter_bitmask = filter_bitmask / 2;
    bool mgmt = filter_bitmask % 2;
    filter_bitmask = filter_bitmask / 2;
    bool data = filter_bitmask % 2;
    filter_bitmask = filter_bitmask / 2;
    wifi_sniffer_filter_frame_types(data, mgmt, ctrl);
    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, ESP_EVENT_ANY_ID, &sniffer_events_handler, NULL));
    packet_sender_task_handler = true;
    xTaskCreate(packet_sender_task, "monitor_task", 1024*2, NULL, configMAX_PRIORITIES - 1, NULL);
    wifi_sniffer_start(channel);

    return true;
}

void packet_monitor_stop() {
    if (packet_sender_task_handler) {
        wifi_sniffer_stop();
        ESP_ERROR_CHECK(esp_event_handler_unregister(SNIFFER_EVENTS, ESP_EVENT_ANY_ID, &sniffer_events_handler));
    }
    packet_sender_task_handler = false;
}

void packet_sender_task(void *args) {

    while (packet_sender_task_handler) {
        vTaskDelay(MIN_UART_WRITE_TIMEOUT_MS / portTICK_PERIOD_MS);

        if (mon_buff_is_empty)
            continue;
        uart_busy = true;
        *curr_mon_buff_p = '\n';
        uint16_t packet_len = curr_mon_buff_p - monitor_buff;
        frame_header_update(packet_counter, packet_len - frame_header_len);
        packet_len++;
        UART_write(monitor_buff, packet_len);
        curr_mon_buff_p = monitor_buff + frame_header_len;
        packet_counter = 0;
        mon_buff_is_empty = true;
        uart_busy = false;
    }

    vTaskDelete(NULL);
}

static void sniffer_events_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (uart_busy)
        return;

    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;

    if (use_bssid_filter && !is_frame_bssid_matching(frame, bssid_filter))
        return;

    unsigned size = frame->rx_ctrl.sig_len;
    unsigned ts_usec = frame->rx_ctrl.timestamp;

    #if (ASCII_HEX_FORMAT == 1)
    if (size == 0 || (MONITOR_BUFF_LEN - (curr_mon_buff_p - monitor_buff) - 1) < (size + sizeof(pcap_record_header_t)) * 2)
        return;
    #elif (ASCII_HEX_FORMAT == 0)
    if (size == 0 || (MONITOR_BUFF_LEN - (curr_mon_buff_p - monitor_buff) - 1) < (size + sizeof(pcap_record_header_t)))
        return;    
    #endif

    pcap_record_header_t pcap_record_header = {
        .ts_sec = ts_usec / 1000000,
        .ts_usec = ts_usec % 1000000,
        .incl_len = size,
        .orig_len = size,
    };

    #if (ASCII_HEX_FORMAT == 1)
    curr_mon_buff_p += hex_convert_to_buff(curr_mon_buff_p, &pcap_record_header, sizeof(pcap_record_header_t));
    curr_mon_buff_p += hex_convert_to_buff(curr_mon_buff_p, frame->payload, size);
    #elif (ASCII_HEX_FORMAT == 0)
    memcpy(curr_mon_buff_p, &pcap_record_header, sizeof(pcap_record_header_t));
    curr_mon_buff_p += sizeof(pcap_record_header_t);   
    memcpy(curr_mon_buff_p, frame->payload, size); 
    curr_mon_buff_p += size;   
    #endif 

    mon_buff_is_empty = false;
    packet_counter++;
}

void frame_header_update(uint16_t pkts, uint16_t bytes) {
    sprintf(monitor_buff, "PCAP data (%3u pkts, %4u bytes):", pkts, bytes);
    monitor_buff[frame_header_len - 1] = ' ';
}