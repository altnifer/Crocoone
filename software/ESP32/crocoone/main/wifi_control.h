#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define CONFIG_AP_WIFI_SSID "AP_SSID_TEST"
#define CONFIG_AP_WIFI_PASSWORD "AP_SSID_TEST_PASS123"
#define CONFIG_AP_MAX_STA_CONN 3
#define CONFIG_AP_WIFI_CHANNEL 1
#define CONFIG_STA_WIFI_SSID "STA_SSID_TEST"
#define CONFIG_STA_WIFI_PASSWORD "STA_SSID_TEST_PASS123"

void wifi_init(void);

void wifi_apsta(void);

void wifi_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]);

void wifi_sta_disconnect();

esp_err_t wifi_set_channel(uint8_t channel);

void wifi_get_sta_mac(uint8_t *mac_sta);

#endif