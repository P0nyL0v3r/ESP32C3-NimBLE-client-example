/*
 * comm.h
 *
 *  Created on: 6 ���. 2023 �.
 *      Author: user
 */

#ifndef MAIN_COMM_H_
#define MAIN_COMM_H_

#include "esp_log.h"
/* BLE */
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "blecent.h"

#define DATA_BUF_SIZE	256
#define COMM_DLY_MS		50

#define DEVICE_UUID 0x180
#define READ_UUID 0xFEF4
#define WRITE_UUID 0xDEAD

typedef union
{
  uint8_t raw[DATA_BUF_SIZE];
  struct
  {
	  struct
	  {
		  uint32_t power;
		  uint32_t counter;
	  }client;
	  struct
	  {
		  uint32_t power;
		  uint32_t counter;
	  }server;
  };
} data_buf;


void comm_start(const struct peer *dev_peer);
void comm_stop();
void comm_init();
void comm_task(	const struct peer *peer);
void button_task(void * arg);

#endif /* MAIN_COMM_H_ */
