#include "attack_beacon.h"
#include "packet_sender.h"
#include "wifi_controller.h"
#include "esp_timer.h"
#include <stdbool.h>

static esp_timer_handle_t beacon_timer_handle;
static bool beacon_is_run = false;

static void timer_send_beacon_frame(void *arg) {
    send_beacon_frame();
}

void beacon_attack_start(unsigned int period_millisec) {
    if (beacon_is_run)
        beacon_attack_stop();
    const esp_timer_create_args_t beacon_timer_args = {
        .callback = &timer_send_beacon_frame
    };
    ESP_ERROR_CHECK(esp_timer_create(&beacon_timer_args, &beacon_timer_handle));        
    ESP_ERROR_CHECK(esp_timer_start_periodic(beacon_timer_handle, period_millisec * 1000));
    beacon_is_run = true;
}

void beacon_attack_stop() {
    if (!beacon_is_run)
        return;
    ESP_ERROR_CHECK(esp_timer_stop(beacon_timer_handle));
    esp_timer_delete(beacon_timer_handle);
    beacon_is_run = false;
}