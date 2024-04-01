#ifndef ATTACK_DEAUTH_H
#define ATTACK_DEAUTH_H

#include "packet_sender.h"
#include "wifi_control.h"
#include "scanner.h"
#include "esp_timer.h"

void deauth_attack_start(const wifi_ap_record_t * target, const unsigned int period_millisec);

void deauth_attack_stop(void);

#endif