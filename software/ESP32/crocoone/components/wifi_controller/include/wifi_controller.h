#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include "esp_wifi_types.h"
#include <stdint.h>

#include "../scanner.h"
#include "../sniffer.h"

#define MAXWIFICH 13
#define MINWIFICH 1

void wifi_init();

void wifi_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]);

void wifi_sta_disconnect();

esp_err_t wifi_set_channel(uint8_t channel);

void wifi_get_sta_mac(uint8_t *mac_sta);

#endif