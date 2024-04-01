#ifndef ATTACK_HANDSHAKE_H
#define ATTACK_HANDSHAKE_H

#include "sniffer.h"
#include "pcap_serializer.h"
#include "hccapx_serializer.h"
#include "frame_analyzer.h"
#include "frame_analyzer_types.h"
#include "frame_analyzer_parser.h"
#include "attack_deauth.h"
#include <string.h>

typedef enum {
    METHOD_NONE,
    METHOD_PASSIVE,
    METHOD_DEAUTH
} handshake_method_t;

void handshake_attack_start(const wifi_ap_record_t * target, const handshake_method_t method);

void handshake_attack_stop(void);

#endif