#ifndef SCANNER_H
#define SCANNER_H

#include "esp_wifi_types.h"
#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_SCAN_LIST_SIZE 20

void wifi_scan(wifi_ap_record_t *ap_info, uint16_t *ap_count);

void wifi_save_scan();

bool get_scan_status(void);

uint16_t * get_aps_count(void);

wifi_ap_record_t * get_aps_info(void);

#endif