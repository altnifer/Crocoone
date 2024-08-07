#ifndef TASKS_MANNAGER_H
#define TASKS_MANNAGER_H

#define CMD_START "AT+"
#define CMD_PARAM_START '='
#define EMPTY_PARAM 0

#define MAX_CMD_NAME_LEN 32
#define MAX_PARAM_COUNT 5 

#define STOP_CMD "STOP_CURRENT_TASK"
#define SCAN_CMD "WIFI_SCAN"
#define DEAUTH_CMD "DEAUTH_ATTACK"
#define BEACON_CMD "BEACON_ATTACK"
#define HANDSHAKE_CMD "HANDSHAKE_ATTACK"
#define PMKID_CMD "PMKID_ATTACK"
#define MONITOR_CMD "PKT_MONITOR"
#define HCCAPX_CMD "HANDSHAKE_GET_HCCAPX"
#define HC22000_CMD "HANDSHAKE_GET_HC22000"

#define RX_BUF_SIZE 128
#define TX_BUF_SIZE 128

typedef struct {
    char cmdName[MAX_CMD_NAME_LEN + 1];
    int param[MAX_PARAM_COUNT];
} cmd_data_t;

void tasks_mannager_init();

#endif