#ifndef SCANNER_H
#define SCANNER_H

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <stdint.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>

#define DEFAULT_SCAN_LIST_SIZE 20

void wifi_scan(wifi_ap_record_t *ap_info, uint16_t *ap_count);

/*bool wifi_scan_status(void);

uint16_t * get_ap_count(void);

wifi_ap_record_t * get_ap_info(void);*/

#endif