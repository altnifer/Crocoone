/**
 * @file attack_pmkid.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-03
 * @copyright Copyright (c) 2021
 * 
 * @brief Provides interface to control PMKID attack.
 */
#ifndef ATTACK_PMKID_H
#define ATTACK_PMKID_H

#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "wifi_control.h"
#include "sniffer.h"
#include "frame_analyzer.h"
#include "frame_analyzer_types.h"
#include <string.h>

#include "esp_timer.h"

#include "hc22000_converter.h"
#include "scanner.h"
#include "uart_control.h"

/*typedef enum {
    SCAN,
    TARGET,
    METHOD_DEAUTH
} pmkid_method_t;*/

/**
 * @brief Starts PMKID attack with given attack_config_t.
 * 
 * To stop PMKID attack, call attack_pmkid_stop().
 * 
 * @param attack_config attack configuration with valid ap_record
 */
void attack_pmkid_start(wifi_ap_record_t * ap_record);
/**
 * @brief Stops PMKID attack.
 * 
 * It stops everything that attack_pmkid_start() started and resets values to original state.
 */
void attack_pmkid_stop();

void pmkid_scan_start();

void pmkid_scan_stop();

#endif