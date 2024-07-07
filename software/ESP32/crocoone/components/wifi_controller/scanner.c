#include "scanner.h"
#include "esp_wifi.h"
#include <string.h>

static wifi_ap_record_t aps_info[DEFAULT_SCAN_LIST_SIZE];
static uint16_t aps_count;
static bool scan_status = 0;

void wifi_scan(wifi_ap_record_t *ap_info, uint16_t *ap_count) {
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    memset(ap_info, 0, sizeof(wifi_ap_record_t) * number);

    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(ap_count));
}

void wifi_save_scan() {
    wifi_scan(aps_info, &aps_count);
    scan_status = 1;
}

bool get_scan_status() {
    return scan_status;
}

uint16_t * get_aps_count() {
    return &aps_count;
}

wifi_ap_record_t * get_aps_info() {
    return aps_info;
}