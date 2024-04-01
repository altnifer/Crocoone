#ifndef PACKET_MONITOR_H
#define PACKET_MONITOR_H

#include "wifi_control.h"
#include "sniffer.h"
#include "esp_timer.h"
#include "uart_control.h"

bool packet_monitor_start(uint8_t channel);

void packet_monitor_stop();

#endif