/*
 * Initialization code for the MAX32664C biometric sensor hub.
 *
 * Copyright (c) 2025, Daniel Kampert
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "max32664c.h"

LOG_MODULE_REGISTER(maxim_max32664c_init, CONFIG_SENSOR_LOG_LEVEL);

/** @brief      Set the SpO2 calibration coefficients.
 *              NOTE: See page 10 of the SpO2 and Heart Rate User Guide for additional information.
 *  @param dev  Pointer to device
 *  @return     0 when successful
 */
static int max32664c_set_spo2_coeffs(const struct device *dev)
{
	const struct max32664c_config *config = dev->config;

	uint8_t tx[15];
	uint8_t rx;

	/* Write the calibration coefficients */
	tx[0] = 0x50;
	tx[1] = 0x07;
	tx[2] = 0x00;

	/* Copy the A value (index 0) into the transmission buffer */
	memcpy(&tx[3], &config->spo2_calib[0], sizeof(int32_t));

	/* Copy the B value (index 1) into the transmission buffer */
	memcpy(&tx[7], &config->spo2_calib[1], sizeof(int32_t));

	/* Copy the C value (index 2) into the transmission buffer */
	memcpy(&tx[11], &config->spo2_calib[2], sizeof(int32_t));

	return max32664c_i2c_transmit(dev, tx, sizeof(tx), &rx, sizeof(rx),
				      MAX32664C_DEFAULT_CMD_DELAY);
}

/** @brief      Write the default configuration to the sensor hub.
 *  @param dev  Pointer to device
 *  @return     0 when successful
 */
static int max32664c_write_config(const struct device *dev)
{
	uint8_t rx;
	uint8_t tx[5];
	const struct max32664c_config *config = dev->config;
	struct max32664c_data *data = dev->data;

	/* Write the default settings */
	tx[0] = 0x50;
	tx[1] = 0x07;
	tx[2] = 0x13;
	tx[3] = config->min_integration_time_idx;
	if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not write minimum integration time!");
		return -EINVAL;
	}

	tx[0] = 0x50;
	tx[1] = 0x07;
	tx[2] = 0x14;
	tx[3] = config->min_sampling_rate_idx;
	if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not write minimum sampling rate!");
		return -EINVAL;
	}

	tx[0] = 0x50;
	tx[1] = 0x07;
	tx[2] = 0x15;
	tx[3] = config->max_integration_time_idx;
	if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not write maximum integration time!");
		return -EINVAL;
	}

	tx[0] = 0x50;
	tx[1] = 0x07;
	tx[2] = 0x16;
	tx[3] = config->max_sampling_rate_idx;
	if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not write maximum sampling rate!");
		return -EINVAL;
	}

	tx[0] = 0x10;
	tx[1] = 0x02;
	tx[2] = config->report_period;
	if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not set report period!");
		return -EINVAL;
	}

	/* Configure WHRM */
	tx[0] = 0x50;
	tx[1] = 0x07;
	tx[2] = 0x17;
	tx[3] = config->hr_config[0];
	tx[4] = config->hr_config[1];
	LOG_DBG("Configuring WHRM: 0x%02X%02X", tx[3], tx[4]);
	if (max32664c_i2c_transmit(dev, tx, 5, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not configure WHRM!");
		return -EINVAL;
	}

	/* Configure SpO2 */
	tx[0] = 0x50;
	tx[1] = 0x07;
	tx[2] = 0x18;
	tx[3] = config->spo2_config[0];
	tx[4] = config->spo2_config[1];
	LOG_DBG("Configuring SpO2: 0x%02X%02X", tx[3], tx[4]);
	if (max32664c_i2c_transmit(dev, tx, 5, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not configure SpO2!");
		return -EINVAL;
	}

	/* Set the interrupt threshold */
	tx[0] = 0x10;
	tx[1] = 0x01;
	tx[2] = 0x01;
	if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not set interrupt threshold!");
		return -EINVAL;
	}

	if (max32664c_set_spo2_coeffs(dev)) {
		LOG_ERR("Can not set SpO2 calibration coefficients!");
		return -EINVAL;
	}

	data->motion_time = config->motion_time;
	data->motion_threshold = config->motion_threshold;
	memcpy(data->led_current, config->led_current, sizeof(data->led_current));

	return 0;
}

/** @brief      Read the configuration from the sensor hub.
 *  @param dev  Pointer to device
 *  @return     0 when successful
 */
static int max32664c_read_config(const struct device *dev)
{
	uint8_t tx[3];
	uint8_t rx[2];
	struct max32664c_data *data = dev->data;

	tx[0] = 0x11;
	tx[1] = 0x02;
	if (max32664c_i2c_transmit(dev, tx, 2, rx, 2, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not read report period!");
		return -EINVAL;
	}
	data->report_period = rx[1];

	tx[0] = 0x51;
	tx[1] = 0x07;
	tx[2] = 0x13;
	if (max32664c_i2c_transmit(dev, tx, 3, rx, 2, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not read minimum integration time!");
		return -EINVAL;
	}
	data->min_integration_time_idx = rx[1];

	tx[0] = 0x51;
	tx[1] = 0x07;
	tx[2] = 0x14;
	if (max32664c_i2c_transmit(dev, tx, 3, rx, 2, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not read minimum sampling rate!");
		return -EINVAL;
	}
	data->min_sampling_rate_idx = rx[1];

	tx[0] = 0x51;
	tx[1] = 0x07;
	tx[2] = 0x15;
	if (max32664c_i2c_transmit(dev, tx, 3, rx, 2, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not read maximum integration time!");
		return -EINVAL;
	}
	data->max_integration_time_idx = rx[1];

	tx[0] = 0x51;
	tx[1] = 0x07;
	tx[2] = 0x16;
	if (max32664c_i2c_transmit(dev, tx, 3, rx, 2, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not read maximum sampling rate!");
		return -EINVAL;
	}
	data->max_sampling_rate_idx = rx[1];

	return 0;
}

int max32664c_init_hub(const struct device *dev)
{
	struct max32664c_data *data = dev->data;

	LOG_DBG("Initialize sensor hub");

	if (max32664c_write_config(dev)) {
		LOG_ERR("Can not write default configuration!");
		return -EINVAL;
	}

	if (max32664c_read_config(dev)) {
		LOG_ERR("Can not read configuration!");
		return -EINVAL;
	}

	data->is_thread_running = true;
	data->thread_id = k_thread_create(&data->thread, data->thread_stack,
					  K_THREAD_STACK_SIZEOF(data->thread_stack),
					  (k_thread_entry_t)max32664c_worker, (void *)dev, NULL,
					  NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_suspend(data->thread_id);
	k_thread_name_set(data->thread_id, "max32664c_worker");

	LOG_DBG("Initial configuration:");

#ifndef CONFIG_MAX32664C_USE_STATIC_MEMORY
	LOG_DBG("\tUsing dynamic memory for queues and buffers");
#else
	LOG_DBG("\tUsing static memory for queues and buffers");
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */

#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
	LOG_DBG("\tUsing extended reports");
#else
	LOG_DBG("\tUsing normal reports");
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS*/

	LOG_DBG("\tReport period: %u", data->report_period);
	LOG_DBG("\tMinimum integration time: %u", data->min_integration_time_idx);
	LOG_DBG("\tMinimum sampling rate: %u", data->min_sampling_rate_idx);
	LOG_DBG("\tMaximum integration time: %u", data->max_integration_time_idx);
	LOG_DBG("\tMaximum sampling rate: %u", data->max_sampling_rate_idx);

	return 0;
}
