/*
 * comm.c
 *
 *  Created on: 6 ���. 2023 �.
 *      Author: user
 */

#include "comm.h"
#include "hmi.h"

#include "FreeRTOSConfig.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

xTaskHandle comm_task_handle;
xSemaphoreHandle loop_sem;

static const char* TAG = "COMM";

const struct peer_chr *write_chr;
const struct peer_chr *read_chr;
data_buf data;

int64_t tick_us;
int write_us;
int read_us;

void comm_start(const struct peer *dev_peer)
{
	BaseType_t status;

	status = xTaskCreate((TaskFunction_t)comm_task, TAG, 2048,(void*) dev_peer, 1, &comm_task_handle);
	if(status != pdPASS)
	{
	    ESP_LOGE(TAG, "Can't create task");
	    return;
	}

	hmi_conn_status_set(true);

    ESP_LOGI(TAG, "Communication start");
}

void comm_stop()
{
	ESP_LOGW(TAG, "Communication stop");

	vTaskDelete(comm_task_handle);

	write_chr = NULL;
	read_chr = NULL;

	hmi_conn_status_set(false);
}

void comm_init()
{
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT,MAX_POWER);

	loop_sem = xSemaphoreCreateBinary();

	ESP_LOGI(TAG, "init OK");
}

int comm_read_cb(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             struct ble_gatt_attr *attr,
                             void *arg)
{
	ESP_LOGD(TAG, "read cb %d",error->status);

	if(error->status == 0)
	{
		read_us = esp_timer_get_time() - tick_us;

	    xSemaphoreGive(loop_sem);

		os_mbuf_copydata(attr->om, 0, DATA_BUF_SIZE, &data);

		ble_gap_conn_rssi(conn_handle,&data.client.rssi);

		hmi_show_conn_param(data.power,data.counter,
				data.server.rssi,data.client.rssi,
				write_us,read_us);
	}

	return 0;
}

int comm_write_cb(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             struct ble_gatt_attr *attr,
                             void *arg)
{
	ESP_LOGD(TAG, "write cb %d",error->status);

	if(error->status == 0)
	{
	    write_us = esp_timer_get_time() - tick_us;

	    const struct peer *peer = peer_find(conn_handle);

	    tick_us = esp_timer_get_time();
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

	xSemaphoreGive(loop_sem);

	for(;;)
	{
		xSemaphoreTake(loop_sem, 5000);

		data.power = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);

		tick_us = esp_timer_get_time();
		ble_gattc_write_flat(peer->conn_handle, write_chr->chr.val_handle, &data,sizeof(data), comm_write_cb, NULL);

		vTaskDelay(COMM_DLY_MS / portTICK_RATE_MS);
	}
}
