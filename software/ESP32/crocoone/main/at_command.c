#include "at_command.h"
#include "hc22000_converter.h"

char tx_buff[TX_BUF_SIZE] = {0};
char rx_buff[RX_BUF_SIZE] = {0};

void AT_init(void) {
    xTaskCreate(atHandler_task, "at_handler_task", 1024*2, NULL, configMAX_PRIORITIES - 1, NULL);
}

void at_send_msg(at_error_t error_num) {
    char str[TX_BUF_SIZE + 20];
    sprintf(str, "AT/ /%s%c", tx_buff, '\n');
    str[strlen("AT/")] = (char)error_num;
    UART_write (str, strlen(tx_buff) + 6);
}

void send_big_msg(const void *src, size_t size) {
    uint32_t bytes_sended = 0;
    uint8_t pkt_count = size / MAX_PKT_SIZE;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for (uint8_t i = 0; i < pkt_count; i++) {
        UART_write (src + bytes_sended, MAX_PKT_SIZE);
        bytes_sended += MAX_PKT_SIZE;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if (size > pkt_count * MAX_PKT_SIZE) {
        UART_write (src + bytes_sended, size - pkt_count * MAX_PKT_SIZE);
    }
}

at_error_t at_wifi_scan(void) {
    int msg_len = 0;
    wifi_ap_record_t *ap_info;
    uint16_t ap_count;

	target_scan();

    ap_info = get_ap_info();
    ap_count = *get_ap_count();

    sprintf(tx_buff, "%d %s", ap_count, "total AP scaned:\n");
    msg_len = strlen(tx_buff);
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        sprintf(tx_buff + msg_len, "%d%s%s%c", i+1, "_SSID: ", (char *)ap_info[i].ssid, '\n');	
        msg_len = strlen(tx_buff);			
    }

    return AT_OK;
}

at_error_t at_scan_deauth_param() {
    //AT+DEAUTH_ATTACK_START->target/timeout
    //example AT+DEAUTH_ATTACK_START->2/-1
    int target;
    int timeout;
    char * scanBuff = strstr(rx_buff, DASTART) + strlen(DASTART);
    if (!sscanf(scanBuff, "%d/%d", &target, &timeout)) {
        sprintf(tx_buff, "AT_WRONG_PARAM");
        return AT_WRONG_PARAM;
    }
    if (start_attack(DEAUTH, target - 1, timeout, NONE) != ATTACK_OK) {
        sprintf(tx_buff, "AT_WRONG_PARAM");
        return AT_WRONG_PARAM;
    }
    sprintf(tx_buff, "DEAUTH_START");
    return AT_OK;
}

at_error_t at_scan_beacon_param() {
    //AT+DEAUTH_ATTACK_START->timeout
    //example AT+DEAUTH_ATTACK_START->50
    int timeout;
    char * scanBuff = strstr(rx_buff, BASTART) + strlen(BASTART);

    if (!sscanf(scanBuff, "%d", &timeout)) {
        sprintf(tx_buff, "AT_WRONG_PARAM");
        return AT_WRONG_PARAM;
    }
    if (start_attack(BEACON, -1, timeout, NONE) != ATTACK_OK) {
        sprintf(tx_buff, "AT_WRONG_PARAM");
        return AT_WRONG_PARAM;
    }
    sprintf(tx_buff, "BEACON_START");
    return AT_OK;
}

at_error_t at_scan_handshake_param() {
    //AT+DEAUTH_ATTACK_START->target/timeout/method
    //example AT+DEAUTH_ATTACK_START->1/120/2
    int target;
    int timeout;
    int method;
    char * scanBuff = strstr(rx_buff, HSTART) + strlen(HSTART);
    if (!sscanf(scanBuff, "%d/%d/%d", &target, &timeout, &method)) {
        sprintf(tx_buff, "AT_WRONG_PARAM");
        return AT_WRONG_PARAM;
    }
    if (start_attack(HANDSHAKE, target - 1, timeout, (handshake_method_t)method) != ATTACK_OK) {
        sprintf(tx_buff, "AT_WRONG_PARAM");
        return AT_WRONG_PARAM;
    }
    sprintf(tx_buff, "HANDSHAKE_START");
    return AT_OK;
}

at_error_t at_scan_packet_monitor_param() {
    //AT+DEAUTH_ATTACK_START->channel
    //example AT+DEAUTH_ATTACK_START->5
    uint16_t channel;
    char * scanBuff = strstr(rx_buff, PKTASTART) + strlen(PKTASTART);

    if (!sscanf(scanBuff, "%hu", &channel) && channel <= MAX_CH) {
        sprintf(tx_buff, "AT_WRONG_PARAM");
        return AT_WRONG_PARAM;
    }
    if (!packet_monitor_start(channel)) {
        sprintf(tx_buff, "AT_WRONG_PARAM");
        return AT_WRONG_PARAM;
    }
    sprintf(tx_buff, "PKT_ANALYZER_START");
    return AT_OK;
}

void at_give_hc22000_data() {
    hccapx_t * hccapx_buffer = hccapx_serializer_get();
    if (hccapx_buffer == NULL) {
        sprintf(tx_buff, "no hc22000");
        at_send_msg(AT_OK);
        return;
    }
    char * hc22000_buffer = hccapx_to_hc22000(hccapx_buffer);
    unsigned int size = hc22000_size_get();
    sprintf(tx_buff, "hc22000 size %u:", size);
    at_send_msg(AT_OK);
    send_big_msg(hc22000_buffer, size); 
}

void at_give_pcap_data() {
    unsigned int size = pcap_serializer_get_size();
    uint8_t * buffer = pcap_serializer_get_buffer();
    if (size == 0) {
        sprintf(tx_buff, "no pcap");
        at_send_msg(AT_OK);
        return;
    }
    sprintf(tx_buff, "pcap size %u:", size);
    at_send_msg(AT_OK);
    send_big_msg(buffer, size);
}

void at_give_hccapx_data() {
    unsigned int size = sizeof(hccapx_t);
    hccapx_t * buffer = hccapx_serializer_get();
    if (buffer == NULL) {
        sprintf(tx_buff, "no hccapx");
        at_send_msg(AT_OK);
        return;
    }
    sprintf(tx_buff, "hccapx size %u:", size);
    at_send_msg(AT_OK);
    send_big_msg(buffer, size);
}

void atHandler_task(void *arg)
{
    while (1) {
        uart_read_bytes(UART_PORT_NUM, rx_buff, RX_BUF_SIZE, 250 / portTICK_PERIOD_MS);
        if (!strstr(rx_buff, "AT+")) continue;

        at_error_t at_error = AT_OK;
        bool send_msg = 1;

        stop_current_attack();
        packet_monitor_stop();  
        if (strstr(rx_buff, SCANWIFI)) {         
            at_error = at_wifi_scan();
        } else if (strstr(rx_buff, DASTART)) {
            at_error = at_scan_deauth_param();
        } else if (strstr(rx_buff, BASTART)) {
            at_error = at_scan_beacon_param();
        } else if (strstr(rx_buff, PKTASTART)) {
            at_error = at_scan_packet_monitor_param();
        } else if (strstr(rx_buff, HSTART)) {
            at_error = at_scan_handshake_param();
        } else if (strstr(rx_buff, PSTART)) {
            send_msg = 0;
            sprintf(tx_buff, "PMKID_START");
            at_send_msg(AT_OK);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            pmkid_scan_start();
        } else if (strstr(rx_buff, GETHCCAPX)) {
            send_msg = 0;
            at_give_hccapx_data();
        } else if (strstr(rx_buff, GETPCAP)) {
            send_msg = 0;
            at_give_pcap_data();
        } else if (strstr(rx_buff, GETHC22000)) {
            send_msg = 0;
            at_give_hc22000_data();
        } else if (strstr(rx_buff, STOPTASK)) {
            sprintf(tx_buff, "task stoped");
            pmkid_scan_stop();
            stop_current_attack();
        } else {
            sprintf(tx_buff, "%s", "ERROR: wrong command");
            at_error = AT_WRONG_CMD;
        }  

        if (send_msg) at_send_msg(at_error);
        memset(rx_buff, 0, RX_BUF_SIZE);
        memset(tx_buff, 0, TX_BUF_SIZE);
    }
}