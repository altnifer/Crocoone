#ifndef PACKET_MONITOR_H
#define PACKET_MONITOR_H

#include "wifi_controller.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Constanst according to reference
 * 
 * @see Ref: https://gitlab.com/wireshark/wireshark/-/wikis/Development/LibpcapFileFormat#global-header
 */
//@{
#define SNAPLEN 65535
#define PCAP_MAGIC_NUMBER 0xa1b2c3d4

//@}

/**
 * @brief Constanst according to reference
 * 
 * @see Ref: http://www.tcpdump.org/linktypes.html (LINKTYPE_IEEE802_11)
 */
#define LINKTYPE_IEEE802_11 105

/**
 * @brief PCAP global header
 * 
 * @see Ref: https://gitlab.com/wireshark/wireshark/-/wikis/Development/LibpcapFileFormat#global-header
 */
typedef struct {
            uint32_t magic_number;   /* magic number */
            uint16_t version_major;  /* major version number */
            uint16_t version_minor;  /* minor version number */
            int32_t  thiszone;       /* GMT to local correction */
            uint32_t sigfigs;        /* accuracy of timestamps */
            uint32_t snaplen;        /* max length of captured packets, in octets */
            uint32_t network;        /* data link type */
} pcap_global_header_t;

/**
 * @brief PCAP record header
 * 
 * @see Ref: https://gitlab.com/wireshark/wireshark/-/wikis/Development/LibpcapFileFormat
 */
typedef struct {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
} pcap_record_header_t;

#define MAX_BIT_MASK_FILTER 7
#define MIN_BIT_MASK_FILTER 1

bool packet_monitor_start(uint8_t channel, uint8_t filter_bitmask, uint8_t *bssid);

void packet_monitor_stop();

void start_pcap_sender();

void stop_pcap_sender();

void add_pcap_header();

void add_pcap_packet(wifi_promiscuous_pkt_t *frame);

#endif