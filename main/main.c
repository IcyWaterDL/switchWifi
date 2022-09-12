#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/ringbuf.h"
#include "freertos/queue.h"

#include "./Button/Button.h"
#include "./Pair/QuickMode/SmartConfig.h"
#include "./Pair/HttpServer/WebServer.h"
#include "./jsonUser/json_user.h"
#include "./SPIFFS/spiffs_user.h"
#include "./WiFi/WiFi_proc.h"
#include "./Mqtt/mqtt.h"
#include "./LED/led.h"
#include "common.h"

static const char *TAG = "MAIN";
Device Device_Infor;
char brokerInfor[100] = "";

enum system_state_t STATE = UNKNOW;

char topic_deviceaction[100] = {'\0'};
char topic_msg[100] = {'\0'};
char topic_actionack[100] = {'\0'};
char topic_cmd_set[100] = {'\0'};
char svalue[200] = {'\0'};

__NOINIT_ATTR bool Flag_quick_pair;

void app_main(void)
{
	nvs_flash_init();
	init_wifi();
	led_init();
	xTaskCreate(led_status_task, "led_status_task", 1024, NULL, 200, NULL);
	xTaskCreate(button_task, "button_task", 4096, NULL, 200, NULL);

	mountSPIFFS();
	get_device_infor(&Device_Infor, brokerInfor);
	if (strlen(brokerInfor) == 0) {
		memcpy(brokerInfor, BROKER, strlen(BROKER) + 1);
	}
	ESP_LOGI(TAG, "BROKER: %s, ID: %s, TOK: %s", brokerInfor, Device_Infor.id, Device_Infor.token);

	sprintf(topic_cmd_set, "ont2mqtt/%s/commands/set", Device_Infor.id);
	sprintf(topic_msg, "messages/%s/attribute", Device_Infor.id);
	sprintf(topic_actionack, "ont2mqtt/%s/actionack", Device_Infor.id);
	sprintf(topic_deviceaction, "ont2mqtt/%s/deviceaction", Device_Infor.id);

	if( esp_reset_reason() == ESP_RST_UNKNOWN || esp_reset_reason() == ESP_RST_POWERON)
	{
		Flag_quick_pair = false;
	}
	if (Flag_quick_pair)
	{
		start_smartconfig();
		STATE = QUICK_MODE;
		ESP_LOGI(TAG, "STATE = QUICK_MODE");
	}
	else if (Flag_quick_pair == false)
	{
		wifi_config_t wifi_config = {
			.sta = {
				.threshold.authmode = WIFI_AUTH_WPA2_PSK,
				.pmf_cfg = {
					.capable = true,
					.required = false
				},
			},
		};
		if (esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config) == ESP_OK)
		{
			ESP_LOGI(TAG, "Wifi configuration already stored in flash partition called NVS");
			ESP_LOGI(TAG, "%s" ,wifi_config.sta.ssid);
			ESP_LOGI(TAG, "%s" ,wifi_config.sta.password);
			wifi_init_sta(wifi_config, WIFI_MODE_STA);
			mqtt_app_start(brokerInfor, Device_Infor.id, Device_Infor.token);
		}
	}
}

