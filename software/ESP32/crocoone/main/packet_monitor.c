#include "packet_monitor.h"

static esp_timer_handle_t packet_monitor_timer_handle;
static bool packet_monitor_run = 0;

static void timer_packet_monitor(void *arg) {
    char str[10];
    sprintf(str, "%lu %lu%c", tmpPacketCounter, deauths, '\n');
    UART_write(str, strlen(str));
    tmpPacketCounter = 0;
    deauths = 0;
    rssiSum = 0;
}

bool packet_monitor_start(uint8_t channel) {
    if (channel < MIN_CH || channel > MAX_CH) return 0;
    if (packet_monitor_run) {
        wifictl_sniffer_set_channel(channel);
        return 1;
    }
    wifictl_sniffer_set_channel(channel);
    const esp_timer_create_args_t pkt_timer_args = {
        .callback = &timer_packet_monitor,
    };
    ESP_ERROR_CHECK(esp_timer_create(&pkt_timer_args, &packet_monitor_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(packet_monitor_timer_handle, 1000000));
    packet_monitor_run = 1;
    return 1;
}

void packet_monitor_stop() {
    if (packet_monitor_run) {
        ESP_ERROR_CHECK(esp_timer_stop(packet_monitor_timer_handle));
        esp_timer_delete(packet_monitor_timer_handle);
        esp_wifi_set_promiscuous(false);
        packet_monitor_run = 0;
    }
}