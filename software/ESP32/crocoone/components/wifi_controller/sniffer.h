/**
 * @file sniffer.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-05
 * @copyright Copyright (c) 2021
 * 
 * @brief Provides an interface for sniffer functionality.
 */
#ifndef SNIFFER_H
#define SNIFFER_H

#include "esp_wifi_types.h"
#include <stdint.h>
#include <stdbool.h>

#define SNAP_LEN 2324
#define MAX_CH 14
#define MIN_CH 1

extern uint32_t tmpPacketCounter;
extern uint32_t deauths;
extern int rssiSum;

ESP_EVENT_DECLARE_BASE(SNIFFER_EVENTS);

enum {
    SNIFFER_EVENT_CAPTURED_DATA,
    SNIFFER_EVENT_CAPTURED_MGMT,
    SNIFFER_EVENT_CAPTURED_CTRL
};

/**
 * @brief Sets sniffer filter for specific frame types. 
 * 
 * @param data sniff data frames
 * @param mgmt sniff management frames
 * @param ctrl sniff control frames
 */
void wifi_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl);

/**
 * @brief Start promiscuous mode on given channel
 * 
 * @param channel channel on which sniffer should operate
 */
void wifi_sniffer_start(uint8_t channel);

/**
 * @brief Stop promisuous mode
 * 
 */
void wifi_sniffer_stop();

#endif