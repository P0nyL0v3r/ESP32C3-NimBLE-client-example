/*
 * hmi.h
 *
 *  Created on: 6 ���. 2023 �.
 *      Author: user
 */

#ifndef MAIN_HMI_H_
#define MAIN_HMI_H_

#include "esp_bt.h"

int power_level_to_dbm(esp_power_level_t level);
void change_power();

void button_task(void * arg);

void hmi_init();
void hmi_conn_status_set(bool status);
void hmi_show_param(esp_power_level_t client_pwr, uint16_t client_count,
					esp_power_level_t server_pwr, uint16_t server_count,
					int8_t rssi);

#endif /* MAIN_HMI_H_ */
