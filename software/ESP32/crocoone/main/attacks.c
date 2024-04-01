#include "attacks.h"

static esp_timer_handle_t attack_timeout_handle;
attacks_t current_attack = NONE;

wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
uint16_t ap_count;
bool scan_status = 0;

void attack_timeout(void *arg) {
    if (current_attack == DEAUTH) {
        deauth_attack_stop();
        UART_write ("STOP DEAUTH\n", strlen("STOP DEAUTH\n"));
    } else if (current_attack == HANDSHAKE) {
        handshake_attack_stop();
        UART_write ("STOP HANDSHAKE\n", strlen("STOP HANDSHAKE\n"));
    } else if (current_attack == BEACON) {
        beacon_attack_stop();
        UART_write ("STOP BEACON\n", strlen("STOP BEACON\n"));
    }

    current_attack = NONE;
}

void attack_init(void) {
    const esp_timer_create_args_t attack_timeout_args = {
        .callback = &attack_timeout
    };
    ESP_ERROR_CHECK(esp_timer_create(&attack_timeout_args, &attack_timeout_handle));
}

attacks_error_t start_attack(const attacks_t type, const int target, const int timeout_sec, const handshake_method_t method) {
    //stop previous attack if it run//
    stop_current_attack();
    //------------------------------//

    //attack error check//
    if (type == DEAUTH) {
        if (target > ap_count || target < 0) return WRONG_TARGET;
        if ((timeout_sec > MAX_DEAUTH_TIMEOUT_SEC || timeout_sec < MIN_TIMEOUT_SEC) && timeout_sec != -1) return WRONG_TIMEOUT;
        if (!scan_status || !ap_count) return NO_TARGET; 
    } else if (type == HANDSHAKE) {
        if (target > ap_count || target < 0) return WRONG_TARGET;
        if (timeout_sec > MAX_HANDSHAKE_TIMEOUT_SEC || timeout_sec < MIN_TIMEOUT_SEC) return WRONG_TIMEOUT;
        if (method < METHOD_PASSIVE || method > METHOD_DEAUTH) return WRONG_METHOD;
        if (!scan_status || !ap_count) return NO_TARGET;
    } else if (type == BEACON) {
        if ((timeout_sec > MAX_DEAUTH_TIMEOUT_SEC || timeout_sec < MIN_TIMEOUT_SEC) && timeout_sec != -1) return WRONG_TIMEOUT;
    } else {
        return WRONG_TYPE;
    }
    //------------------//

    //attack start//
    if (type == DEAUTH) {
        deauth_attack_start(ap_info + target, 500);
        // set timeout, timeout = -1 - no timeout
        if (timeout_sec != -1) ESP_ERROR_CHECK(esp_timer_start_once(attack_timeout_handle, timeout_sec * 1000000));
    } else if (type == HANDSHAKE) {
        handshake_attack_start(ap_info + target, method);
        ESP_ERROR_CHECK(esp_timer_start_once(attack_timeout_handle, timeout_sec * 1000000));
    } else if (type == BEACON) {
        beacon_attack_start(10);
        // set timeout, timeout = -1 - no timeout
        if (timeout_sec != -1) ESP_ERROR_CHECK(esp_timer_start_once(attack_timeout_handle, timeout_sec * 1000000));
    }
    //------------//

    current_attack = type;
    return ATTACK_OK;
}

//stop attack before timeout
void stop_current_attack(void) {
    if (current_attack == NONE) return;

    esp_timer_stop(attack_timeout_handle);

    if (current_attack == DEAUTH) {
        deauth_attack_stop();
    } else if (current_attack == HANDSHAKE) {
        handshake_attack_stop();
    } else if (current_attack == BEACON) {
        beacon_attack_stop();
    }

    current_attack = NONE;
}

void target_scan() {
    wifi_scan(ap_info, &ap_count);
    scan_status = 1;
}

uint16_t * get_ap_count(void) {
    return &ap_count;
}

wifi_ap_record_t * get_ap_info(void) {
    return ap_info;
}