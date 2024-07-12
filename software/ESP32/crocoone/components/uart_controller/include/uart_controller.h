#ifndef UART_CONTROLLER_H
#define UART_CONTROLLER_H

#include <stddef.h>

#define TXD_PIN 17
#define RXD_PIN 16

#define UART_PORT_NUM 2

void UART_init();

int UART_write(const void *src, size_t size);

int UART_read(char *data_buff, unsigned int len);

#endif