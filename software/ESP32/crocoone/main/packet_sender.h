#ifndef PACKET_SENDER_H
#define PACKET_SENDER_H

#include "esp_wifi_types.h"
#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "wifi_control.h"

#define BEACON_SSID_OFFSET 38
#define SRCADDR_OFFSET 10
#define BSSID_OFFSET 16
#define SEQNUM_OFFSET 22

char *ssids[] = {
	"01 Never gonna give you up",
	"02 Never gonna let you down",
	"03 Never gonna run around",
	"04 and desert you",
	"05 Never gonna make you cry",
	"06 Never gonna say goodbye",
	"07 Never gonna tell a lie",
	"08 and hurt you"
};

#define TOTAL_LINES (sizeof(ssids) / sizeof(char *))

void send_raw_frame(const uint8_t *frame_buffer, int size);

void send_deauth_frame(const wifi_ap_record_t *ap_record);

void send_beacon_frame(void);

void send_probe_frame(const wifi_ap_record_t *ap_record);

void deauth_task(void *arg);

void beacon_task(void *arg);

#endif