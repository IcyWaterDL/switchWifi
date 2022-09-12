/* MQTT (over TCP) Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/uart.h"
#include "../jsonUser/json_user.h"
#include "../common.h"
#include "../LED/led.h"

static const char *TAG = "MQTT";
esp_mqtt_client_config_t mqtt_cfg;
esp_mqtt_client_handle_t client;
RingbufHandle_t buf_handle;

extern char topic_cmd_set[100];
extern char topic_msg[100];
extern enum system_state_t STATE;

extern uint8_t value;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;
	switch ((esp_mqtt_event_id_t)event_id) {
	case MQTT_EVENT_CONNECTED:
		STATE = NORMAL;
		ESP_LOGI(TAG, "STATE = NORMAL");
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		esp_mqtt_client_subscribe(client, topic_cmd_set, 0);

		char *data = "{\"endpoint_count\":1}";
		esp_mqtt_client_publish(client, topic_msg, data, strlen(data), 0, 0);
		break;
	case MQTT_EVENT_DISCONNECTED:
		STATE = LOCAL_MODE;
		ESP_LOGI(TAG, "STATE = LOCAL_MODE");
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		break;

	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_DATA:
	{
		UBaseType_t res =  xRingbufferSendFromISR(buf_handle, event->data, event->data_len, (BaseType_t *)10);
		if (res != pdTRUE) {
			ESP_LOGE(TAG, "Failed to send item to buf_handle\n");
		}
		break;
	}
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		break;
	default:
		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
		break;
	}
}

void mqtt_handle(void *arg)
{
	cmd command_set;
	char * item = NULL;
	size_t item_size;
	while(1)
	{
		item = (char *)xRingbufferReceiveFromISR(buf_handle, &item_size);
		if(item_size && item)
		{
			item[item_size] = '\0';
			ESP_LOGI(TAG, "PAYLOAD: %s", item);
			memset(&command_set, 0, sizeof(command_set));
			JSON_analyze_SUB_MQTT(item, &command_set);
			ESP_LOGI(TAG, "action: %s\n", (char*)command_set.action);
			if(strcmp(command_set.action, "on-off") == 0){
				value = (strcmp(command_set.value, "on") == 0) ? 1:0;
				gpio_set_level(LED_BLINK, value);
			}
			vRingbufferReturnItem(buf_handle, (void *)item);
		}
		vTaskDelay(50/portTICK_RATE_MS);
	}
}

void mqtt_app_start(char *broker, char *client_id, char *passowrd)
{
	buf_handle = xRingbufferCreate(4096, RINGBUF_TYPE_NOSPLIT);
	if (buf_handle == NULL) {
		ESP_LOGE(TAG, "Failed to create ring buffer\n");
	}
	mqtt_cfg.uri = broker;
	mqtt_cfg.username = client_id;
	mqtt_cfg.client_id = client_id;
	mqtt_cfg.password = passowrd;
	mqtt_cfg.keepalive = 60;
	client = esp_mqtt_client_init(&mqtt_cfg);
	xTaskCreate(mqtt_handle, "mqtt_handle", 4096, NULL, 3, NULL);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
	esp_mqtt_client_start(client);
}
