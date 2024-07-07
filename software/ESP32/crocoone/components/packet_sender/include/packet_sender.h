#ifndef PACKET_SENDER_H
#define PACKET_SENDER_H

#include "esp_wifi_types.h"
#include <stdint.h>

#define BEACON_SSID_OFFSET 38
#define SRCADDR_OFFSET 10
#define BSSID_OFFSET 16
#define SEQNUM_OFFSET 22

void send_raw_frame(const uint8_t *frame_buffer, int size);

void send_deauth_frame(const wifi_ap_record_t *ap_record);

void send_beacon_frame(void);

#endif
