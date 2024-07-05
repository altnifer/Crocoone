#ifndef ATTACK_DEAUTH_H
#define ATTACK_DEAUTH_H

#include "wifi_controller.h"

void deauth_attack_start(const wifi_ap_record_t * target, const unsigned int period_millisec);

void deauth_attack_stop();

#endif