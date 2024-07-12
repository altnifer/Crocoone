#include "tasks_mannager.h"
#include "uart_controller.h"
#include "wifi_controller.h"
#include "attack_handshake.h"
#include "attack_deauth.h"
#include "attack_beacon.h"
#include "hccapx_serializer.h"
#include "attack_pmkid.h"
#include "packet_monitor.h"
#include "data_converter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_timer.h"
#include "esp_event.h"
#include <stdio.h>
#include <string.h>

static char rx_buff[RX_BUF_SIZE];
static char tx_buff[TX_BUF_SIZE];
static esp_timer_handle_t task_timer_handle;
static cmd_data_t curr_cmd_data;
static bool timer_is_active = false;

void task_timer_callback(void *arg);
void task_mannager(void *args);
bool parse_first_cmd(char *buffer, unsigned int buffer_len, cmd_data_t *output);
unsigned int contains_symbol(char *symbols, unsigned int symbols_len, char symbol);
unsigned int get_first_token(char *str, unsigned int str_len, char *separators, unsigned int sep_count);
uint16_t add_cmd_to_buff(cmd_data_t cmd_data, char *buff);
void send_error_responce(cmd_data_t cmd_data, char *eeror_msg);
void send_responce(cmd_data_t cmd_data);
bool start_task(cmd_data_t cmd_data);
bool stop_curr_task(cmd_data_t cmd_data);
bool deauth_cmd_check(cmd_data_t cmd_data);
bool handshake_cmd_check(cmd_data_t cmd_data);
bool beacon_cmd_check(cmd_data_t cmd_data);
bool pmkid_cmd_check(cmd_data_t cmd_data);
bool packet_monitor_cmd_check(cmd_data_t cmd_data);
void wifi_scan_responce(cmd_data_t cmd_data);

void tasks_mannager_init() {
    const esp_timer_create_args_t task_timer_args = {
        .callback = &task_timer_callback
    };
    ESP_ERROR_CHECK(esp_timer_create(&task_timer_args, &task_timer_handle));
    memset(&curr_cmd_data, 0, sizeof(cmd_data_t));
    xTaskCreate(task_mannager, "task_mannager_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
}

void task_timer_callback(void *arg) {
    stop_curr_task(curr_cmd_data);
}

void task_mannager(void *args) {
    while (1) {
        vTaskDelay(250 / portTICK_PERIOD_MS);

        UART_read(rx_buff, RX_BUF_SIZE);
        unsigned int buffer_len = strlen(rx_buff);
        if (!buffer_len)
            continue;

        cmd_data_t cmd_data;
        if (!parse_first_cmd(rx_buff, buffer_len, &cmd_data))
            continue;

        stop_curr_task(curr_cmd_data);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        memset(&curr_cmd_data, 0, sizeof(cmd_data_t));
        if (start_task(cmd_data)) curr_cmd_data = cmd_data;
        memset(rx_buff, 0, RX_BUF_SIZE);
    }
}


bool parse_first_cmd(char *buffer, unsigned int buffer_len, cmd_data_t *output) {
    if (buffer == NULL || buffer_len == 0) return false;
    char *start_p = buffer;
    if (!(buffer = strstr(buffer, CMD_START))) return false; //not cmd in buff
    buffer_len = get_first_token(buffer, buffer_len - (start_p - buffer), " \n\t\r\0", 5);

    cmd_data_t cmd_data = {};

    buffer = buffer + strlen(CMD_START);
    buffer_len = buffer_len - strlen(CMD_START);
    if (buffer_len == 0) return false; //empty cmd ("AT+")

    int cmdNameLen = buffer_len;
    char *paramStart = buffer + contains_symbol(buffer, buffer_len, CMD_PARAM_START);

    if (paramStart > buffer) {
        int paramLen = buffer_len - (paramStart - buffer);
        cmdNameLen = cmdNameLen - paramLen - 1;

        for (int i = 0; i < MAX_PARAM_COUNT && paramLen > 0; i++) {
            if (!sscanf(paramStart, "%d", cmd_data.param + i)) {
                cmd_data.param[i] = EMPTY_PARAM;
                break;
            }
            int offset = get_first_token(paramStart, paramLen, ",", 1) + 1;
            paramLen = paramLen - offset;
            paramStart = paramStart + offset;
        }
    }
    // buffer is too long or empty ("AT+=1,2,3")
    if (cmdNameLen > MAX_CMD_NAME_LEN || !cmdNameLen) return false; 
    memcpy(cmd_data.cmdName, buffer, cmdNameLen);
    memcpy(output, &cmd_data, sizeof(cmd_data_t));
    return true;
}

unsigned int contains_symbol(char *symbols, unsigned int symbols_len, char symbol) {
  unsigned int pos = 1;
  char *start_p = symbols;
  if(symbols == NULL)
    return 0;
  while(symbols - start_p < symbols_len){
    if(*symbols++ == symbol)
      return pos;
    pos++;
  }
  return 0;
}

unsigned int get_first_token(char *str, unsigned int str_len, char *separators, unsigned int sep_count) {
    if(str == NULL || separators == NULL)
        return 0;

    char *sp = str;
    while(!contains_symbol(separators, sep_count, *sp) && sp - str < str_len) sp++;

    int token_len = sp - str;

    return token_len;
}

uint16_t add_cmd_to_buff(cmd_data_t cmd_data, char *buff) {
    sprintf(buff, "%s%s%c", CMD_START, cmd_data.cmdName, CMD_PARAM_START);
    uint16_t buff_len = strlen(buff);
    for (int i = 0; i < MAX_PARAM_COUNT - 1; i++) {
        sprintf(buff + buff_len, "%d,", cmd_data.param[i]);
        buff_len = strlen(buff);
    }
    sprintf(buff + buff_len, "%d", cmd_data.param[MAX_PARAM_COUNT - 1]);

    return strlen(buff);
}

void send_error_responce(cmd_data_t cmd_data, char *eeror_msg) {
    uint16_t cmd_len = add_cmd_to_buff(cmd_data, tx_buff);
    uint16_t msg_len = strlen(" - cmd accepted with error: ");
    uint16_t error_len = strlen(eeror_msg);
    memcpy(tx_buff + cmd_len, " - cmd accepted with error: ", msg_len);
    memcpy(tx_buff + cmd_len + msg_len, eeror_msg, error_len);
    memcpy(tx_buff + cmd_len + msg_len + error_len, "\n", 2);
    UART_write(tx_buff, strlen(tx_buff));
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void send_responce(cmd_data_t cmd_data) {
    uint16_t cmd_len = add_cmd_to_buff(cmd_data, tx_buff);
    memcpy(tx_buff + cmd_len, " - cmd accepted and started\n", strlen(" - cmd accepted and started\n") + 1);
    UART_write(tx_buff, strlen(tx_buff));
    vTaskDelay(100 / portTICK_PERIOD_MS);
}


bool start_task(cmd_data_t cmd_data) {
    bool cmd_started = true;
    if (!strcmp(cmd_data.cmdName, SCAN_CMD)) {
        wifi_scan_responce(cmd_data);
    } else if (!strcmp(cmd_data.cmdName, DEAUTH_CMD)) {
        cmd_started = deauth_cmd_check(cmd_data);
    } else if (!strcmp(cmd_data.cmdName, BEACON_CMD)) {
        cmd_started = beacon_cmd_check(cmd_data);
    } else if (!strcmp(cmd_data.cmdName, HANDSHAKE_CMD)) {
        cmd_started = handshake_cmd_check(cmd_data);
    } else if (!strcmp(cmd_data.cmdName, PMKID_CMD)) {
        cmd_started = pmkid_cmd_check(cmd_data);
    } else if (!strcmp(cmd_data.cmdName, MONITOR_CMD)) {
        cmd_started = packet_monitor_cmd_check(cmd_data);
    } else if (!strcmp(cmd_data.cmdName, HCCAPX_CMD)) {
        send_responce(cmd_data);
        handshake_get_hccapx_data();
    } else if (!strcmp(cmd_data.cmdName, HC22000_CMD)) {
        send_responce(cmd_data);
        handshake_get_hc22000_data();
    } else if (!strcmp(cmd_data.cmdName, STOP_CMD)) {
        send_responce(cmd_data);
    } else {
        send_error_responce(cmd_data, "wrong cmd");
        cmd_started = false;
    }
    return cmd_started;
}

bool stop_curr_task(cmd_data_t cmd_data) {
    bool task_stopped = true;
    if (!strcmp(cmd_data.cmdName, DEAUTH_CMD)) {
        deauth_attack_stop();
    } else if (!strcmp(cmd_data.cmdName, BEACON_CMD)) {
        beacon_attack_stop();
    } else if (!strcmp(cmd_data.cmdName, HANDSHAKE_CMD)) {
        handshake_attack_stop();
    } else if (!strcmp(cmd_data.cmdName, PMKID_CMD)) {
        pmkid_scan_stop();
    } else if (!strcmp(cmd_data.cmdName, MONITOR_CMD)) {
        packet_monitor_stop();
    } else {
        task_stopped = false;
    }
    if (task_stopped) {
        if (timer_is_active)
            esp_timer_stop(task_timer_handle);
        timer_is_active = false;
    }
    return task_stopped;
}


bool deauth_cmd_check(cmd_data_t cmd_data) {
    uint16_t ap_count = *get_aps_count();
    wifi_ap_record_t *ap_info = get_aps_info();
    int target = cmd_data.param[0];
    int timeout = cmd_data.param[1];

    if (!get_scan_status() || !ap_count) {
        send_error_responce(cmd_data, "no AP's scanned");
        return false;
    }
    if (target > ap_count || target < 0) {
        send_error_responce(cmd_data, "wrong target (param 0)");
        return false;        
    }
    if ((timeout > MAX_TIMEOUT_SEC || timeout < MIN_TIMEOUT_SEC) && timeout != EMPTY_PARAM) {
        send_error_responce(cmd_data, "wrong timeout (param 1)");
        return false;        
    }    
    if (timeout != EMPTY_PARAM) {
        ESP_ERROR_CHECK(esp_timer_start_once(task_timer_handle, timeout * 1000000));
        timer_is_active = true;
    }
    send_responce(cmd_data);
    deauth_attack_start(ap_info + target - 1, 500);
    return true;
}

bool handshake_cmd_check(cmd_data_t cmd_data) {
    uint16_t ap_count = *get_aps_count();
    wifi_ap_record_t *ap_info = get_aps_info();
    int target = cmd_data.param[0];
    int method = cmd_data.param[1];
    int send_pcap = cmd_data.param[2];
    int timeout = cmd_data.param[3];

    if (!get_scan_status() || !ap_count) {
        send_error_responce(cmd_data, "no AP's scanned");
        return false;
    }
    if (target > ap_count || target < 0) {
        send_error_responce(cmd_data, "wrong target (param 0)");
        return false;        
    }
    if ((timeout > MAX_TIMEOUT_SEC || timeout < MIN_TIMEOUT_SEC) && timeout != EMPTY_PARAM) {
        send_error_responce(cmd_data, "wrong timeout (param 3)");
        return false;        
    }    
    if (method != (int)METHOD_PASSIVE && method != (int)METHOD_DEAUTH) {
        send_error_responce(cmd_data, "wrong method (param 1)");
        return false;        
    }    
    if (send_pcap != true && send_pcap != false) {
        send_error_responce(cmd_data, "wrong send pcap flag (param 2)");
        return false;        
    }    
    if (timeout != EMPTY_PARAM) {
        ESP_ERROR_CHECK(esp_timer_start_once(task_timer_handle, timeout * 1000000));
        timer_is_active = true;
    }
    send_responce(cmd_data);
    handshake_attack_start(ap_info + target - 1, (handshake_method_t)method, (bool)send_pcap);
    return true;
}

bool beacon_cmd_check(cmd_data_t cmd_data) {
    int timeout = cmd_data.param[0];

    if ((timeout > MAX_TIMEOUT_SEC || timeout < MIN_TIMEOUT_SEC) && timeout != EMPTY_PARAM) {
        send_error_responce(cmd_data, "wrong channel (param 0)");
        return false;        
    }    
    if (timeout != EMPTY_PARAM) {
        ESP_ERROR_CHECK(esp_timer_start_once(task_timer_handle, timeout * 1000000));
        timer_is_active = true;
    }
    send_responce(cmd_data);
    beacon_attack_start(10);
    return true;
}

bool pmkid_cmd_check(cmd_data_t cmd_data) {
    int timeout = cmd_data.param[0];

    if ((timeout > MAX_TIMEOUT_SEC || timeout < MIN_TIMEOUT_SEC) && timeout != EMPTY_PARAM) {
        send_error_responce(cmd_data, "wrong timeout (param 0)");
        return false;        
    }    
    if (timeout != EMPTY_PARAM) {
        ESP_ERROR_CHECK(esp_timer_start_once(task_timer_handle, timeout * 1000000));
        timer_is_active = true;
    }
    send_responce(cmd_data);
    pmkid_scan_start();
    return true;
}

bool packet_monitor_cmd_check(cmd_data_t cmd_data) {
    uint16_t ap_count = *get_aps_count();
    wifi_ap_record_t *ap_info = get_aps_info();
    int channel = cmd_data.param[0];
    int filter = cmd_data.param[1];
    int target = cmd_data.param[2];
    int timeout = cmd_data.param[3];

    if (target == EMPTY_PARAM && (channel < MIN_CH || channel > MAX_CH)) {
        send_error_responce(cmd_data, "wrong channel (param 0)");
        return false;        
    }    
    if (filter < MIN_BIT_MASK_FILTER || filter > MAX_BIT_MASK_FILTER) {
        send_error_responce(cmd_data, "wrong filter (param 1)");
        return false;        
    }    
    if ((timeout > MAX_TIMEOUT_SEC || timeout < MIN_TIMEOUT_SEC) && timeout != EMPTY_PARAM) {
        send_error_responce(cmd_data, "wrong timeout (param 3)"); 
        return false;        
    }    
    if (target != EMPTY_PARAM && (!get_scan_status() || !ap_count)) {
        send_error_responce(cmd_data, "no AP's scanned");
        return false;
    }
    if (target != EMPTY_PARAM && (target > ap_count || target < 0)) {
        send_error_responce(cmd_data, "wrong target (param 2)");
        return false;        
    }
    if (timeout != EMPTY_PARAM) {
        ESP_ERROR_CHECK(esp_timer_start_once(task_timer_handle, timeout * 1000000));
        timer_is_active = true;
    }
    send_responce(cmd_data);
    packet_monitor_start(
        (target == EMPTY_PARAM) ? ((uint8_t)channel) : ((ap_info + (target - 1))->primary), 
        (uint8_t)filter,
        (target == EMPTY_PARAM) ? (NULL) : ((ap_info + (target - 1))->bssid) 
        );
    return true;
}

void wifi_scan_responce(cmd_data_t cmd_data) {
    send_responce(cmd_data);

    int msg_len = 0;
    wifi_ap_record_t *ap_info;
    uint16_t ap_count;

	wifi_save_scan();

    ap_info = get_aps_info();
    ap_count = *get_aps_count();

    sprintf(tx_buff, "total AP scanned %2u:\n", ap_count);
    msg_len = strlen(tx_buff);
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        sprintf(tx_buff + msg_len, "%d%s%s%c", i+1, "_SSID: ", (char *)ap_info[i].ssid, '\n');	
        msg_len = strlen(tx_buff);			
    }

    UART_write(tx_buff, strlen(tx_buff));
}