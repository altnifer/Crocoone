#ifndef ATTACK_BEACON_H
#define ATTACK_BEACON_H

#include "packet_sender.h"
#include "wifi_control.h"
#include "esp_timer.h"

void beacon_attack_start(unsigned int period_millisec);

void beacon_attack_stop(void);

#endif