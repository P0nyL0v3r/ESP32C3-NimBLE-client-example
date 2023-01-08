/*
 * hmi.c
 *
 *  Created on: 6 ���. 2023 �.
 *      Author: user
 */

#include "hmi.h"
#include "comm.h"

#include "string.h"

#include "esp_bt.h"
#include "esp_log.h"

#include "FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"

#include "ssd1306.h"
#include "font8x8_basic.h"

#define BUTTON_PIN GPIO_NUM_9

SSD1306_t disp;

xTaskHandle button_task_handle;

static const char* TAG = "HMI";

struct
{
	struct
	{
		int server;
		int client;
	}rssi;
}avg;
struct
{
	struct
	{
		int server;
		int client;
	}rssi;
}sum;


int power_level_to_dbm(esp_power_level_t level)
{
	static const int buf[] = {-24,-21,-18,-15,-12,-9,-6,-3,
			0,3,6,9,12,15,18,21};

	return buf[level];
}

void change_power()
{
	esp_power_level_t pwr = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);
	pwr++;

	if(pwr > MAX_POWER)
		pwr = MIN_POWER;

	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, pwr);
}

void avg_calc(int8_t server_rssi, int8_t client_rssi)
{
	static int idx = 0;

	if(idx >= 5)
	{
		avg.rssi.server = sum.rssi.server/idx;
		avg.rssi.client = sum.rssi.client/idx;

		sum.rssi.server = 0;
		sum.rssi.client = 0;
		idx = 0;
	}
	else
	{
		sum.rssi.server += server_rssi;
		sum.rssi.client += client_rssi;
		idx++;
	}
}


void button_task(void * arg)
{
	gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
	int i = 0;
	for(;;)
	{
		i = 0;
		while(gpio_get_level(BUTTON_PIN) == 0)
		{
			i++;
			vTaskDelay(10 / portTICK_RATE_MS);
		}
		if(i > 5)
			change_power();
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}

void hmi_init()
{
	i2c_master_init(&disp, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
	disp._flip = true;
	ssd1306_init(&disp, 128, 32);
	ssd1306_clear_screen(&disp, false);
	ssd1306_contrast(&disp, 0xff);
	hmi_conn_status_set(false);

	BaseType_t status = xTaskCreate((TaskFunction_t)button_task, TAG, 2048,NULL, 1, &button_task_handle);
	if(status != pdPASS)
	{
	    ESP_LOGE(TAG, "Can't create task");
	    return;
	}

	ESP_LOGI(TAG,"init OK");
}

void hmi_conn_status_set(bool status)
{
#define CONNECTED_STR "connected"
#define DISCONNECTED_STR "disconnected"
	ssd1306_clear_line(&disp, 0, false);
	if(status)
	{
		ssd1306_display_text(&disp, 0, CONNECTED_STR, sizeof(CONNECTED_STR), false);
	}
	else
	{
		ssd1306_display_text(&disp, 0, DISCONNECTED_STR, sizeof(DISCONNECTED_STR), true);
	}
}

void hmi_show_conn_param(esp_power_level_t level, uint16_t counter,
		int8_t server_rssi, int8_t client_rssi,
		int write_us, int read_us)
{
	char str[64];

	avg_calc(server_rssi, client_rssi);

	sprintf(str, "Tx,dBm: %02d, %04d",power_level_to_dbm(level),counter);
	ssd1306_display_text(&disp, 1, str, strlen(str), false);

	sprintf(str,"RSSI s:%02d c:%02d",avg.rssi.server,avg.rssi.client);
	ssd1306_display_text(&disp, 2, str, strlen(str), false);

	float write_kb_s = (DATA_BUF_SIZE * 8.0 / 1024.0) / ((float)write_us / 1000000.0);
	float read_kb_s = (DATA_BUF_SIZE * 8.0 / 1024.0) / ((float)read_us / 1000000.0);
	sprintf(str,"kb/s w:%02d  r:%02d ",(int)write_kb_s, (int)read_kb_s);
	ssd1306_display_text(&disp, 3, str, strlen(str), false);
}
