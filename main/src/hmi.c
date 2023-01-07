/*
 * hmi.c
 *
 *  Created on: 6 ���. 2023 �.
 *      Author: user
 */

#include "hmi.h"

#include "string.h"

#include "esp_bt.h"
#include "esp_log.h"

#include "FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

//#include "driver/i2c.h"
#include "driver/gpio.h"

#include "ssd1306.h"
#include "font8x8_basic.h"

//#define I2C_MASTER_SCL_IO           GPIO_NUM_0      		   /*!< GPIO number used for I2C master clock */
//#define I2C_MASTER_SDA_IO           GPIO_NUM_1      		   /*!< GPIO number used for I2C master data  */
//#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
//#define I2C_MASTER_FREQ_HZ          100000                     /*!< I2C master clock frequency */
//#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
//#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
//#define I2C_MASTER_TIMEOUT_MS       1000

#define MIN_POWER ESP_PWR_LVL_N0
#define MAX_POWER ESP_PWR_LVL_P21

#define BUTTON_PIN GPIO_NUM_9

SSD1306_t disp;

xTaskHandle button_task_handle;

static const char* TAG = "HMI";

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

int8_t avg_rssi(int8_t rssi)
{
	static int avg = 0, idx = 0, sum = 0;

	if(idx >= 20)
	{
		avg = sum/idx;
		idx = 0;
		sum = 0;
	}
	else
	{
		sum +=rssi;
		idx++;
	}
	return avg;
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
#define CONNECTED_STR "  dBm cntr  rssi"
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

void hmi_show_param(esp_power_level_t client_pwr, uint16_t client_count,
					esp_power_level_t server_pwr, uint16_t server_count,
					int8_t rssi)
{
	char str[64];

	sprintf(str, "c %02d  %05d  %02d",
			power_level_to_dbm(client_pwr),	client_count,
			avg_rssi(rssi));

	ssd1306_display_text(&disp, 1, str, strlen(str), false);

	sprintf(str, "s %02d  %05d",
			power_level_to_dbm(server_pwr),server_count);

	ssd1306_display_text(&disp, 2, str, strlen(str), false);
}
