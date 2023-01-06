/*
 * hmi.c
 *
 *  Created on: 6 ���. 2023 �.
 *      Author: user
 */

#include "hmi.h"

#include "esp_bt.h"
#include "esp_log.h"

#include "FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#define I2C_MASTER_SCL_IO           GPIO_NUM_0      		   /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           GPIO_NUM_1      		   /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          100000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define MIN_POWER ESP_PWR_LVL_N0
#define MAX_POWER ESP_PWR_LVL_P21

#define BUTTON_PIN GPIO_NUM_9

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
	esp_err_t i2c_status;

    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);
    i2c_status = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if(i2c_status != ESP_OK)
    {
	    ESP_LOGE(TAG, "Can't install I2C driver");
	    return;
    }

	BaseType_t status = xTaskCreate((TaskFunction_t)button_task, TAG, 2048,NULL, 1, &button_task_handle);
	if(status != pdPASS)
	{
	    ESP_LOGE(TAG, "Can't create task");
	    return;
	}

	ESP_LOGI(TAG,"init OK");
}
