#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "wifi_control.h"
#include "at_command.h"

#include "string.h"
#include "esp_wifi.h"

void app_main(void) {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	wifi_init();
	wifi_apsta();
    UART_init();
	AT_init();
	attack_init();
}