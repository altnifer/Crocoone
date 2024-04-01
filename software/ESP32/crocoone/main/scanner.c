#include "scanner.h"

//wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
//uint16_t ap_count;
//bool wifi_scan_flag = 0;

void wifi_scan(wifi_ap_record_t *ap_info, uint16_t *ap_count) {
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    memset(ap_info, 0, sizeof(wifi_ap_record_t) * number);

    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(ap_count));
}

/*bool wifi_scan_status(void) {
    return wifi_scan_flag;
}

uint16_t * get_ap_count(void) {
    return &ap_count;
}

wifi_ap_record_t * get_ap_info(void) {
    return ap_info;
}*/