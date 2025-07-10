/*
 * Copyright (c) 2025, Eduard Iten
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/misc/vnd7050aj/vnd7050aj.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(vnd7050aj_sample, LOG_LEVEL_DBG);

const struct device *vnd7050aj_dev = DEVICE_DT_GET(DT_INST(0, st_vnd7050aj));

int main(void)
{
	int32_t current_ma, voltage_mv, temp_c;
	int err;

	LOG_INF("*** VND7050AJ Sample ***");

	if (!device_is_ready(vnd7050aj_dev)) {
		LOG_ERR("VND7050AJ device not ready.");
		return -ENODEV;
	}

	while (1) {
		/* --- Channel 0 --- */
		LOG_INF("Channel 0 ON");
		vnd7050aj_set_output_state(vnd7050aj_dev, VND7050AJ_CHANNEL_0, true);
		k_msleep(1000);
		err = vnd7050aj_read_load_current(vnd7050aj_dev, VND7050AJ_CHANNEL_0, &current_ma);
		if (err) {
			LOG_ERR("Failed to read current on channel 0: %d", err);
		} else {
			LOG_INF("Load current on channel 0: %d mA", current_ma);
		}
		LOG_INF("Channel 0 OFF");
		vnd7050aj_set_output_state(vnd7050aj_dev, VND7050AJ_CHANNEL_0, false);
		k_msleep(1000);

		/* --- Channel 1 --- */
		LOG_INF("Channel 1 ON");
		vnd7050aj_set_output_state(vnd7050aj_dev, VND7050AJ_CHANNEL_1, true);
		k_msleep(1000);
		err = vnd7050aj_read_load_current(vnd7050aj_dev, VND7050AJ_CHANNEL_1, &current_ma);
		if (err) {
			LOG_ERR("Failed to read current on channel 1: %d", err);
		} else {
			LOG_INF("Load current on channel 1: %d mA", current_ma);
		}
		LOG_INF("Channel 1 OFF");
		vnd7050aj_set_output_state(vnd7050aj_dev, VND7050AJ_CHANNEL_1, false);
		k_msleep(1000);

		/* --- Diagnostics --- */
		err = vnd7050aj_read_supply_voltage(vnd7050aj_dev, &voltage_mv);
		if (err) {
			LOG_ERR("Failed to read supply voltage: %d", err);
		} else {
			LOG_INF("Supply voltage: %d mV", voltage_mv);
		}

		err = vnd7050aj_read_chip_temp(vnd7050aj_dev, &temp_c);
		if (err) {
			LOG_ERR("Failed to read chip temperature: %d", err);
		} else {
			LOG_INF("Chip temperature: %d C", temp_c);
		}

		k_msleep(5000);
	}
	return 0;
}
