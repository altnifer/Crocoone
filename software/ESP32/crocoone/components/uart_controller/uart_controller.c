#include "uart_controller.h"
#include "driver/uart.h"
#include "driver/gpio.h"

void UART_init() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_PORT_NUM, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

int UART_write(const void *src, size_t size) {
    return uart_write_bytes(UART_PORT_NUM, src, size);
}

int UART_read(char *data_buff, unsigned int len) {
    return uart_read_bytes(UART_PORT_NUM, data_buff, len, 250 / portTICK_PERIOD_MS);
}