#ifndef AT_COMMAND_H
#define AT_COMMAND_H

#include <string.h>
#include "uart_control.h"
#include "scanner.h"
#include "attacks.h"
#include "packet_monitor.h"

#include "attack_pmkid.h"

//#define SEND_MSG 0
#define TX_BUF_SIZE 512
#define RX_BUF_SIZE 32
#define MAX_PKT_SIZE 512

//AT commands//
#define SCANWIFI "AT+WIFI_SCAN"
#define DASTART "AT+DEAUTH_ATTACK_START->"
#define BASTART "AT+BEACON_ATTACK_START->"
#define HSTART "AT+HANDSHAKE_ATTACK->"
#define PSTART "AT+PMKID_ATTACK"
#define PKTASTART "AT+PKT_ANALYZER_START->"
#define STOPTASK "AT+STOP_CURRENT_TASK"
#define GETHCCAPX "AT+GET_HCCAPX"
#define GETPCAP "AT+GET_PCAP"
#define GETHC22000 "AT+GET_HC22000"
//-----------//

typedef enum {
    AT_OK = 1,
    AT_WRONG_CMD,
    AT_WRONG_PARAM,
    AT_NO_WIFI_SCAN
} at_error_t;

void AT_init(void);

void atHandler_task(void *arg);

at_error_t at_wifi_scan(void);

void at_send_msg(at_error_t error_num);

#endif