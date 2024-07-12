/*
 * pcap_parser.h
 *
 *  Created on: Jul 11, 2024
 *      Author: nikit
 */

#ifndef INC_PCAP_PARSER_H_
#define INC_PCAP_PARSER_H_

#include <stdint.h>
#include <stdbool.h>

#define MONITOR_BUFF_LEN 4096
#define FRAME_HEADER_LEN 34

void pcap_parser_start();
void pcap_parser_stop();
void pcap_parser_set_use_sd_flag(bool flag);
uint32_t get_pkts_sum();
bool check_sd_write_error();

#endif /* INC_PCAP_PARSER_H_ */
