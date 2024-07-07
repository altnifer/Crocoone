#ifndef PACKET_MONITOR_H
#define PACKET_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_BIT_MASK_FILTER 7
#define MIN_BIT_MASK_FILTER 1

#define ASCII_HEX_FORMAT 0

bool packet_monitor_start(uint8_t channel, uint8_t filter_bitmask, uint8_t *bssid);

void packet_monitor_stop();

#endif