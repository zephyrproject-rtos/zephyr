/*
 * Copyright (c) 2022 Kickmaker
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/satellite.h>
#include <zephyr/drivers/satellite/kim1.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(kim1_sample);

K_SEM_DEFINE(kim1_sem, 0, 1);

/** @brief Implement result callback for pool send final status */
static void status_returned(bool tx_status)
{
	if (tx_status) {
		LOG_INF("Finished sending pool of messages successfully");
	} else {
		LOG_ERR("An error occurred while sending pool of messages");
	}

	k_sem_give(&kim1_sem);
}

void main(void)
{
	int ret;
	struct satellite_modem_config config;
	uint8_t tx_data[] = "0123";
	const struct device *kineis_modem = DEVICE_DT_GET_ANY(kineis_kim1);

	if (kineis_modem == NULL) {
		LOG_ERR("\nError: no kim1 module found.\n");
		return;
	}

	if (!device_is_ready(kineis_modem)) {
		LOG_ERR("\nError: Device \"%s\" is not ready; "
		  "check the driver initialization logs for errors.\n",
		  kineis_modem->name);
		return;
	}

	LOG_INF("Modem ID: %s", kineis_get_modem_id());
	LOG_INF("Modem FW version: %s", kineis_get_modem_fw_version());
	LOG_INF("Modem SN: %s", kineis_get_modem_serial_number());

	/* Set TX power to 1000mW */
	config.tx_power = 1000U;

	ret = satellite_config(kineis_modem, &config);
	if (ret < 0) {
		LOG_ERR("Satellite config failed");
		return;
	}

	/* Send a message synchronously (wait for tx confirmation) */
	ret = satellite_send_sync(kineis_modem, tx_data, sizeof(tx_data) - 1);
	if (ret == 0) {
		LOG_INF("Synchronous send OK");
	} else {
		LOG_ERR("Synchronous send NOK");
	}

	while (1) {

		LOG_INF("Launch send pool of 10 messages with 60 seconds delay between them");

		/* Send a pool of message (10) with a delay of (60 seconds) between them.
		 * Once finished, status return callback will be executed with final TX status
		 */
		ret = satellite_send_pool_async(kineis_modem, tx_data, sizeof(tx_data) - 1,
					10U, K_SECONDS(60),
					(satellite_api_send_result_cb *) &status_returned);
		if (ret < 0) {
			LOG_ERR("Satellite satellite_send_pool failed");
			return;
		}

		/* Wait for send of pool message finished */
		(void) k_sem_take(&kim1_sem, K_FOREVER);
	}

}
