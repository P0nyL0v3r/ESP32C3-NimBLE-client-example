/*
 * comm.c
 *
 *  Created on: 6 ���. 2023 �.
 *      Author: user
 */

#include "comm.h"
#include "hmi.h"

#include "esp_bt.h"

#include "FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

xTaskHandle comm_task_handle;

static const char* TAG = "COMM";

const struct peer_chr *write_chr;
const struct peer_chr *read_chr;
data_buf data;
uint32_t counter;

void comm_start(const struct peer *dev_peer)
{
	BaseType_t status;

	status = xTaskCreate((TaskFunction_t)comm_task, TAG, 2048,(void*) dev_peer, 1, &comm_task_handle);
	if(status != pdPASS)
	{
	    ESP_LOGE(TAG, "Can't create task");
	    return;
	}

    ESP_LOGI(TAG, "Communication start");
}

void comm_stop()
{
	ESP_LOGW(TAG, "Communication stop");

	vTaskDelete(comm_task_handle);

	write_chr = NULL;
	read_chr = NULL;
}

void comm_init()
{
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT,ESP_PWR_LVL_P12);

	data.client.power = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);
	data.server.power = data.client.power;

	ESP_LOGI(TAG, "init OK");
}

int comm_read_cb(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             struct ble_gatt_attr *attr,
                             void *arg)
{
	ESP_LOGI(TAG, "read cb %d",error->status);

	if(error->status == 0)
	{
		int8_t rssi = 0;
		ble_gap_conn_rssi(conn_handle,&rssi);

		os_mbuf_copydata(attr->om, 0, DATA_BUF_SIZE, &data);

		ESP_LOGI(TAG, "client pwr: %d, %d",
				power_level_to_dbm(data.client.power),data.client.counter);
		ESP_LOGI(TAG, "server pwr: %d, %d, rssi: %d",
				power_level_to_dbm(data.server.power),data.server.counter,rssi);
	}

	return 0;
}

int comm_write_cb(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             struct ble_gatt_attr *attr,
                             void *arg)
{
	ESP_LOGI(TAG, "write cb %d",error->status);

	if(error->status == 0)
	{
	    const struct peer *peer = peer_find(conn_handle);

		ble_gattc_read(peer->conn_handle, read_chr->chr.val_handle, comm_read_cb, NULL);
	}

	return 0;
}

void comm_task(	const struct peer *peer)
{
	write_chr = peer_chr_find_uuid(peer,
	                           BLE_UUID16_DECLARE(DEVICE_UUID),
	                           BLE_UUID16_DECLARE(WRITE_UUID));

	read_chr = peer_chr_find_uuid(peer,
	                           BLE_UUID16_DECLARE(DEVICE_UUID),
	                           BLE_UUID16_DECLARE(READ_UUID));

	for(;;)
	{
		counter++;
		data.client.counter = counter;

		esp_power_level_t pwr = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);
		data.client.power = pwr;
		data.server.power = pwr;

		ble_gattc_write_flat(peer->conn_handle, write_chr->chr.val_handle, &data,sizeof(data), comm_write_cb, NULL);

		vTaskDelay(COMM_DLY_MS / portTICK_RATE_MS);
	}
}
