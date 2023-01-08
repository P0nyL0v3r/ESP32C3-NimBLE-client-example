/*
 * hmi.h
 *
 *  Created on: 6 ���. 2023 �.
 *      Author: user
 */

#ifndef MAIN_HMI_H_
#define MAIN_HMI_H_

#include "esp_bt.h"

#define MIN_POWER ESP_PWR_LVL_N0
#define MAX_POWER ESP_PWR_LVL_P21

int power_level_to_dbm(esp_power_level_t level);
void change_power();

void button_task(void * arg);

void hmi_init();
void hmi_conn_status_set(bool status);
void hmi_show_conn_param(esp_power_level_t level, uint16_t counter,
		int8_t server_rssi, int8_t client_rssi,
		int write_us, int read_us);
#endif /* MAIN_HMI_H_ */
