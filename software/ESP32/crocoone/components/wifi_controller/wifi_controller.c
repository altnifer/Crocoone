#include "wifi_controller.h"
#include "esp_wifi.h"
#include <string.h>

void wifi_init() {
	ESP_ERROR_CHECK(esp_netif_init());
	
	esp_netif_create_default_wifi_ap();
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
	ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]) {
    wifi_config_t sta_wifi_config = {
        .sta = {
            .channel = ap_record->primary,
            .scan_method = WIFI_FAST_SCAN,
            .pmf_cfg.capable = false,
            .pmf_cfg.required = false
        },
    };
    mempcpy(sta_wifi_config.sta.ssid, ap_record->ssid, 32);

    if(password != NULL){
        if(strlen(password) >= 64) {
            return;
        }
        memcpy(sta_wifi_config.sta.password, password, strlen(password) + 1);
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void wifi_sta_disconnect() {
    ESP_ERROR_CHECK(esp_wifi_disconnect());
}

esp_err_t wifi_set_channel(uint8_t channel) {
    if((channel < MINWIFICH) || (channel >  MAXWIFICH)){
        return ESP_ERR_INVALID_ARG;
    }
    return esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void wifi_get_sta_mac(uint8_t *mac_sta) {
    esp_wifi_get_mac(WIFI_IF_STA, mac_sta);
}
