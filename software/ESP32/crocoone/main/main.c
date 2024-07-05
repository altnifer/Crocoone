#include "wifi_controller.h"
#include "uart_controller.h"
#include "tasks_mannager.h"

#include "esp_wifi.h"
#include "nvs_flash.h"

void app_main(void)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_event_loop_create_default());

	UART_init();

	wifi_init();

	tasks_mannager_init();
}
