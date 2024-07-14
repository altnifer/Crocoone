#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include "esp_wifi_types.h"
#include <stdint.h>

#include "../scanner.h"
#include "../sniffer.h"

#define DEFAULT_AP_SSID "Crocoone"
#define DEFAULT_AP_PASSWORD "password"
#define DEFAULT_AP_MAX_CONNECTIONS 1

#define MAXWIFICH 13
#define MINWIFICH 1

void wifi_init();
void wifi_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]);
void wifi_ap_start(wifi_config_t *wifi_config);
void wifi_default_ap_start();
void wifi_sta_disconnect();
esp_err_t wifi_set_channel(const uint8_t channel);
void wifi_get_sta_mac(uint8_t *mac_sta);
void wifi_set_ap_mac(uint8_t *mac_ap);

#endif