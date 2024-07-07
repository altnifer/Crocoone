#ifndef UART_CONTROLLER_H
#define UART_CONTROLLER_H

#include <stddef.h>

#define TXD_PIN 17 //19 for esp32 c6 // 17 for esp32 c3
#define RXD_PIN 16 //20 for esp32 c6 // 16 for esp32 c3

#define UART_PORT_NUM 2 //1 for esp32 c6 //2 for esp32 c3

void UART_init();

int UART_write (const void *src, size_t size);

int UART_read(char *data_buff, unsigned int len);

#endif