#ifndef ATTACK_PMKID_H
#define ATTACK_PMKID_H

#include "esp_wifi_types.h"

#define AP_SCAN_TIME_MS 2000

void attack_pmkid_start(wifi_ap_record_t * ap_record);

void attack_pmkid_stop();

void pmkid_scan_start();

void pmkid_scan_stop();

#endif