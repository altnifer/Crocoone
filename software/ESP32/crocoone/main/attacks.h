#ifndef ATTACKS_H
#define ATTACKS_H

#include "esp_timer.h"
#include "scanner.h"
#include "attack_handshake.h"
#include "attack_beacon.h"
#include "attack_deauth.h"
#include "attack_pmkid.h"
#include "uart_control.h"

#define MAX_HANDSHAKE_TIMEOUT_SEC 240
#define MAX_DEAUTH_TIMEOUT_SEC 300
#define MIN_TIMEOUT_SEC 10

typedef enum {
    NONE,
    DEAUTH,
    BEACON,
    HANDSHAKE,
    PMKID
} attacks_t;

typedef enum {
    ATTACK_OK,
    WRONG_TYPE,
    WRONG_TARGET,
    WRONG_TIMEOUT,
    WRONG_METHOD, 
    NO_TARGET
} attacks_error_t;

void attack_init(void);

attacks_error_t start_attack(const attacks_t type, const int target, const int timeout_sec, const handshake_method_t method);

void stop_current_attack(void);

void target_scan();

uint16_t * get_ap_count(void);

wifi_ap_record_t * get_ap_info(void);

#endif