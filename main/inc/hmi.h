/*
 * hmi.h
 *
 *  Created on: 6 џэт. 2023 у.
 *      Author: user
 */

#ifndef MAIN_HMI_H_
#define MAIN_HMI_H_

#include "esp_bt.h"

int power_level_to_dbm(esp_power_level_t level);
void change_power();
void button_task(void * arg);
void hmi_init();


#endif /* MAIN_HMI_H_ */
