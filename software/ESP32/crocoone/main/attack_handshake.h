#ifndef ATTACK_HANDSHAKE_H
#define ATTACK_HANDSHAKE_H

#include "wifi_controller.h"

typedef enum {
    METHOD_PASSIVE,
    METHOD_DEAUTH,
    METHOD_EVIL_AP
} handshake_method_t;

void handshake_attack_start(wifi_ap_record_t * target, handshake_method_t method, bool send_pcap_flag);
void handshake_attack_stop();
void handshake_get_hccapx_data();
void handshake_get_hc22000_data();

#endif