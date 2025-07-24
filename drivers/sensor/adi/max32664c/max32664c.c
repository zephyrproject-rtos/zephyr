/*
 * Copyright (c) 2025, Daniel Kampert
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>

#include "max32664c.h"

#define DT_DRV_COMPAT maxim_max32664c

#if (DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0)
#warning "max32664c driver enabled without any devices"
#endif

LOG_MODULE_REGISTER(maxim_max32664c, CONFIG_SENSOR_LOG_LEVEL);

int max32664c_i2c_transmit(const struct device *dev, uint8_t *tx_buf, uint8_t tx_len,
			   uint8_t *rx_buf, uint32_t rx_len, uint16_t delay_ms)
{
	const struct max32664c_config *config = dev->config;

	/* Wake up the sensor hub before the transmission starts (min. 300 us) */
	gpio_pin_set_dt(&config->mfio_gpio, false);
	k_usleep(500);

	if (i2c_write_dt(&config->i2c, tx_buf, tx_len)) {
		LOG_ERR("I2C transmission error!");
		return -EBUSY;
	}

	k_msleep(delay_ms);

	if (i2c_read_dt(&config->i2c, rx_buf, rx_len)) {
		LOG_ERR("I2C read error!");
		return -EBUSY;
	}

	k_msleep(MAX32664C_DEFAULT_CMD_DELAY);

	/* The sensor hub can enter sleep mode again now */
	gpio_pin_set_dt(&config->mfio_gpio, true);
	k_usleep(300);

	/* Check the status byte for a valid transaction */
	if (rx_buf[0] != 0) {
		return -EINVAL;
	}

	return 0;
}

/** @brief      Check the accelerometer and AFE WHOAMI registers.
 *              This function is called during device initialization.
 *  @param dev  Pointer to device
 *  @return     0 when successful
 */
static int max32664c_check_sensors(const struct device *dev)
{
	uint8_t afe_id;
	uint8_t tx[3];
	uint8_t rx[2];
	struct max32664c_data *data = dev->data;
	const struct max32664c_config *config = dev->config;

	LOG_DBG("Checking sensors...");

	/* Read MAX86141 WHOAMI */
	tx[0] = 0x41;
	tx[1] = 0x00;
	tx[2] = 0xFF;
	if (max32664c_i2c_transmit(dev, tx, 3, rx, 2, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	if (config->use_max86141) {
		LOG_DBG("\tUsing MAX86141 as AFE");
		afe_id = 0x25;
	} else if (config->use_max86161) {
		LOG_DBG("\tUsing MAX86161 as AFE");
		afe_id = 0x36;
	} else {
		LOG_ERR("\tNo AFE defined!");
		return -ENODEV;
	}

	data->afe_id = rx[1];
	if (data->afe_id != afe_id) {
		LOG_ERR("\tAFE WHOAMI failed: 0x%X", data->afe_id);
		return -ENODEV;
	}

	LOG_DBG("\tAFE WHOAMI OK: 0x%X", data->afe_id);

	/* Read Accelerometer WHOAMI */
	tx[0] = 0x41;
	tx[1] = 0x04;
	tx[2] = 0x0F;
	if (max32664c_i2c_transmit(dev, tx, 3, rx, 2, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	data->accel_id = rx[1];
	/* The sensor hub firmware supports only two accelerometers and one is set to */
	/* EoL. The remaining one is the ST LIS2DS12. */
	if (data->accel_id != 0x43) {
		LOG_ERR("\tAccelerometer WHOAMI failed: 0x%X", data->accel_id);
		return -ENODEV;
	}

	LOG_DBG("\tAccelerometer WHOAMI OK: 0x%X", data->accel_id);

	return 0;
}

/** @brief      Stop the current algorithm.
 *  @param dev  Pointer to device
 *  @return     0 when successful
 */
static int max32664c_stop_algo(const struct device *dev)
{
	uint8_t rx;
	uint8_t tx[3];
	struct max32664c_data *data = dev->data;

	if (data->op_mode == MAX32664C_OP_MODE_IDLE) {
		LOG_DBG("No algorithm running, nothing to stop.");
		return 0;
	}

	LOG_DBG("Stop the current algorithm...");

	/* Stop the algorithm */
	tx[0] = 0x52;
	tx[1] = 0x07;
	tx[2] = 0x00;
	if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, 120)) {
		return -EINVAL;
	}

	switch (data->op_mode) {
	case MAX32664C_OP_MODE_RAW: {
#ifndef CONFIG_MAX32664C_USE_STATIC_MEMORY
		k_msgq_cleanup(&data->raw_report_queue);
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */
		break;
	}
#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
	case MAX32664C_OP_MODE_ALGO_AEC_EXT:
	case MAX32664C_OP_MODE_ALGO_AGC_EXT: {
#ifndef CONFIG_MAX32664C_USE_STATIC_MEMORY
		k_msgq_cleanup(&data->ext_report_queue);
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */
		break;
	}
#else
	case MAX32664C_OP_MODE_ALGO_AEC:
	case MAX32664C_OP_MODE_ALGO_AGC: {
#ifndef CONFIG_MAX32664C_USE_STATIC_MEMORY
		k_msgq_cleanup(&data->report_queue);
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */
		break;
	}
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
	case MAX32664C_OP_MODE_SCD: {
#ifndef CONFIG_MAX32664C_USE_STATIC_MEMORY
		k_msgq_cleanup(&data->scd_report_queue);
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */
		break;
	}
	default: {
		LOG_ERR("Unknown algorithm mode: %d", data->op_mode);
		return -EINVAL;
	}
	};

	data->op_mode = MAX32664C_OP_MODE_IDLE;

	k_thread_suspend(data->thread_id);

	return 0;
}

/** @brief      Put the device into raw measurement mode.
 *  @param dev  Pointer to device
 *  @return     0 when successful
 */
static int max32664c_set_mode_raw(const struct device *dev)
{
	uint8_t rx;
	uint8_t tx[4];
	struct max32664c_data *data = dev->data;

	/* Stop the current algorithm mode */
	if (max32664c_stop_algo(dev)) {
		LOG_ERR("Failed to stop the algorithm!");
		return -EINVAL;
	}

	LOG_INF("Entering RAW mode...");

	/* Set the output format to sensor data only */
	tx[0] = 0x10;
	tx[1] = 0x00;
	tx[2] = MAX32664C_OUT_SENSOR_ONLY;
	if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	/* Enable the AFE */
	tx[0] = 0x44;
	tx[1] = 0x00;
	tx[2] = 0x01;
	tx[3] = 0x00;
	if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, 250)) {
		return -EINVAL;
	}

	/* Enable the accelerometer */
	if (max32664c_acc_enable(dev, true)) {
		return -EINVAL;
	}

	/* Set AFE sample rate to 100 Hz */
	tx[0] = 0x40;
	tx[1] = 0x00;
	tx[2] = 0x12;
	tx[3] = 0x18;
	if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	/* Set the LED current */
	for (uint8_t i = 0; i < sizeof(data->led_current); i++) {
		tx[0] = 0x40;
		tx[1] = 0x00;
		tx[2] = 0x23 + i;
		tx[3] = data->led_current[i];
		LOG_INF("Set LED%d current: %u", i + 1, data->led_current[i]);
		if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			LOG_ERR("Can not set LED%d current", i + 1);
			return -EINVAL;
		}
	}

#ifndef CONFIG_MAX32664C_USE_STATIC_MEMORY
	if (k_msgq_alloc_init(&data->raw_report_queue, sizeof(struct max32664c_raw_report_t),
			      CONFIG_MAX32664C_QUEUE_SIZE)) {
		LOG_ERR("Failed to allocate RAW report queue!");
		return -ENOMEM;
	}
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */

	data->op_mode = MAX32664C_OP_MODE_RAW;

	k_thread_resume(data->thread_id);

	return 0;
}

/** @brief              Put the sensor hub into algorithm mode.
 *  @param dev          Pointer to device
 *  @param device_mode  Target device mode
 *  @param algo_mode    Target algorithm mode
 *  @param extended     Set to #true when the extended mode should be used
 *  @return             0 when successful
 */
static int max32664c_set_mode_algo(const struct device *dev, enum max32664c_device_mode device_mode,
				   enum max32664c_algo_mode algo_mode, bool extended)
{
	uint8_t rx;
	uint8_t tx[5];
	struct max32664c_data *data = dev->data;

	/* Stop the current algorithm mode */
	if (max32664c_stop_algo(dev)) {
		LOG_ERR("Failed to stop the algorithm!");
		return -EINVAL;
	}

	LOG_DBG("Entering algorithm mode...");

#ifndef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
	if (extended) {
		LOG_ERR("No support for extended reports enabled!");
		return -EINVAL;
	}
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */

	/* Set the output mode to sensor and algorithm data */
	tx[0] = 0x10;
	tx[1] = 0x00;
	tx[2] = MAX32664C_OUT_ALGO_AND_SENSOR;
	if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	/* Set the algorithm mode */
	tx[0] = 0x50;
	tx[1] = 0x07;
	tx[2] = 0x0A;
	tx[3] = algo_mode;
	if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	if (device_mode == MAX32664C_OP_MODE_ALGO_AEC) {
		LOG_DBG("Entering AEC mode...");

		/* Enable AEC */
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x0B;
		tx[3] = 0x01;
		if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			return -EINVAL;
		}

		/* Enable Auto PD */
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x12;
		tx[3] = 0x01;
		if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			return -EINVAL;
		}

		/* Enable SCD */
		LOG_DBG("Enabling SCD...");
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x0C;
		tx[3] = 0x01;
		if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			return -EINVAL;
		}

		data->op_mode = MAX32664C_OP_MODE_ALGO_AEC;

		if (extended) {
			data->op_mode = MAX32664C_OP_MODE_ALGO_AEC_EXT;
		}
	} else if (device_mode == MAX32664C_OP_MODE_ALGO_AGC) {
		LOG_DBG("Entering AGC mode...");

		/* TODO: Test if this works */
		/* Set the LED current */
		for (uint8_t i = 0; i < sizeof(data->led_current); i++) {
			tx[0] = 0x40;
			tx[1] = 0x00;
			tx[2] = 0x23 + i;
			tx[3] = data->led_current[i];
			LOG_INF("Set LED%d current: %u", i + 1, data->led_current[i]);
			if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1,
						   MAX32664C_DEFAULT_CMD_DELAY)) {
				LOG_ERR("Can not set LED%d current", i + 1);
				return -EINVAL;
			}
		}

		/* Enable AEC */
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x0B;
		tx[3] = 0x01;
		if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			return -EINVAL;
		}

		/* Disable PD auto current calculation */
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x12;
		tx[3] = 0x00;
		if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			return -EINVAL;
		}

		/* Disable SCD */
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x0C;
		tx[3] = 0x00;
		if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			return -EINVAL;
		}

		/* Set AGC target PD current to 10 uA */
		/* TODO: Add setting of PD current via API or DT? */
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x11;
		tx[3] = 0x00;
		tx[4] = 0x64;
		if (max32664c_i2c_transmit(dev, tx, 5, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			return -EINVAL;
		}

		data->op_mode = MAX32664C_OP_MODE_ALGO_AGC;

		if (extended) {
			data->op_mode = MAX32664C_OP_MODE_ALGO_AGC_EXT;
		}
	} else {
		LOG_ERR("Invalid mode!");
		return -EINVAL;
	}

	/* Enable HR and SpO2 algorithm */
	tx[2] = 0x01;
	if (extended) {
		tx[2] = 0x02;
	}

	tx[0] = 0x52;
	tx[1] = 0x07;

	/* Use the maximum time to cover all modes (see Table 6 and 12 in the User Guide) */
	if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, 500)) {
		return -EINVAL;
	}

#ifndef CONFIG_MAX32664C_USE_STATIC_MEMORY
	if (k_msgq_alloc_init(&data->raw_report_queue, sizeof(struct max32664c_raw_report_t),
			      CONFIG_MAX32664C_QUEUE_SIZE)) {
		LOG_ERR("Failed to allocate RAW report queue!");
		return -ENOMEM;
	}

	if (!extended && k_msgq_alloc_init(&data->report_queue, sizeof(struct max32664c_report_t),
					   CONFIG_MAX32664C_QUEUE_SIZE)) {
		LOG_ERR("Failed to allocate report queue!");
		return -ENOMEM;
	}

#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
	if (extended &&
	    k_msgq_alloc_init(&data->ext_report_queue, sizeof(struct max32664c_ext_report_t),
			      CONFIG_MAX32664C_QUEUE_SIZE)) {
		LOG_ERR("Failed to allocate extended report queue!");
		return -ENOMEM;
	}
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */

	k_thread_resume(data->thread_id);

	return 0;
}

/** @brief      Enable the skin contact detection only mode.
 *  @param dev  Pointer to device
 *  @return     0 when successful
 */
static int max32664c_set_mode_scd(const struct device *dev)
{
	uint8_t rx;
	uint8_t tx[4];
	struct max32664c_data *data = dev->data;

	/* Stop the current algorithm mode */
	if (max32664c_stop_algo(dev)) {
		LOG_ERR("Failed to stop the algorithm!");
		return -EINVAL;
	}

	LOG_DBG("MAX32664C entering SCD mode...");

	/* Use LED2 for SCD */
	tx[0] = 0xE5;
	tx[1] = 0x02;
	if (max32664c_i2c_transmit(dev, tx, 2, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	/* Set the output mode to algorithm data */
	tx[0] = 0x10;
	tx[1] = 0x00;
	tx[2] = MAX32664C_OUT_ALGORITHM_ONLY;
	if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	/* Enable SCD only algorithm */
	tx[0] = 0x52;
	tx[1] = 0x07;
	tx[2] = 0x03;
	if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, 500)) {
		return -EINVAL;
	}

#ifndef CONFIG_MAX32664C_USE_STATIC_MEMORY
	if (k_msgq_alloc_init(&data->scd_report_queue, sizeof(struct max32664c_scd_report_t),
			      CONFIG_MAX32664C_QUEUE_SIZE)) {
		LOG_ERR("Failed to allocate SCD report queue!");
		return -ENOMEM;
	}
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */

	data->op_mode = MAX32664C_OP_MODE_SCD;

	k_thread_resume(data->thread_id);

	return 0;
}

static int max32664c_set_mode_wake_on_motion(const struct device *dev)
{
	uint8_t rx;
	uint8_t tx[6];
	struct max32664c_data *data = dev->data;

	LOG_DBG("MAX32664C entering wake on motion mode...");

	/* Stop the current algorithm */
	tx[0] = 0x52;
	tx[1] = 0x07;
	tx[2] = 0x00;
	if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	/* Set the motion detection threshold (see Table 12 in the SpO2 and Heart Rate Using Guide)
	 */
	tx[0] = 0x46;
	tx[1] = 0x04;
	tx[2] = 0x00;
	tx[3] = 0x01;
	tx[4] = MAX32664C_MOTION_TIME(data->motion_time);
	tx[5] = MAX32664C_MOTION_THRESHOLD(data->motion_threshold);
	if (max32664c_i2c_transmit(dev, tx, 6, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	/* Set the output mode to sensor data */
	tx[0] = 0x10;
	tx[1] = 0x00;
	tx[2] = MAX32664C_OUT_SENSOR_ONLY;
	if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	/* Enable the accelerometer */
	if (max32664c_acc_enable(dev, true)) {
		return -EINVAL;
	}

	data->op_mode = MAX32664C_OP_MODE_WAKE_ON_MOTION;

	return 0;
}

static int max32664c_exit_mode_wake_on_motion(const struct device *dev)
{
	uint8_t rx;
	uint8_t tx[6];
	struct max32664c_data *data = dev->data;

	LOG_DBG("MAX32664C exiting wake on motion mode...");

	/* Exit wake on motion mode */
	tx[0] = 0x46;
	tx[1] = 0x04;
	tx[2] = 0x00;
	tx[3] = 0x00;
	tx[4] = 0xFF;
	tx[5] = 0xFF;
	if (max32664c_i2c_transmit(dev, tx, 6, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	/* Disable the accelerometer */
	if (max32664c_acc_enable(dev, false)) {
		return -EINVAL;
	}

	data->op_mode = MAX32664C_OP_MODE_IDLE;

	return 0;
}

static int max32664c_disable_sensors(const struct device *dev)
{
	uint8_t rx;
	uint8_t tx[4];
	struct max32664c_data *data = dev->data;

	if (max32664c_stop_algo(dev)) {
		LOG_ERR("Failed to stop the algorithm!");
		return -EINVAL;
	}

	/* Leave wake on motion first because we disable the accelerometer */
	if (max32664c_exit_mode_wake_on_motion(dev)) {
		LOG_ERR("Failed to exit wake on motion mode!");
		return -EINVAL;
	}

	LOG_DBG("Disable the sensors...");

	/* Disable the AFE */
	tx[0] = 0x44;
	tx[1] = 0x00;
	tx[2] = 0x00;
	tx[3] = 0x00;
	if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, 250)) {
		return -EINVAL;
	}

	/* Disable the accelerometer */
	if (max32664c_acc_enable(dev, false)) {
		return -EINVAL;
	}

	data->op_mode = MAX32664C_OP_MODE_IDLE;

	return 0;
}

static int max32664c_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct max32664c_data *data = dev->data;

	switch (data->op_mode) {
	case MAX32664C_OP_MODE_STOP_ALGO:
	case MAX32664C_OP_MODE_IDLE:
		LOG_DBG("Device is idle, no data to fetch!");
		return -EAGAIN;
	case MAX32664C_OP_MODE_SCD:
		k_msgq_get(&data->scd_report_queue, &data->scd, K_NO_WAIT);
		return 0;
#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
	case MAX32664C_OP_MODE_ALGO_AEC_EXT:
	case MAX32664C_OP_MODE_ALGO_AGC_EXT:
		k_msgq_get(&data->ext_report_queue, &data->ext, K_NO_WAIT);
		return 0;
#else
	case MAX32664C_OP_MODE_ALGO_AEC:
	case MAX32664C_OP_MODE_ALGO_AGC:
		k_msgq_get(&data->report_queue, &data->report, K_NO_WAIT);
		return 0;
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
	/* Raw data are reported with normal and extended algorithms so we need to fetch them too */
	case MAX32664C_OP_MODE_RAW:
		k_msgq_get(&data->raw_report_queue, &data->raw, K_NO_WAIT);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int max32664c_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct max32664c_data *data = dev->data;

	switch ((int)chan) {
	case SENSOR_CHAN_ACCEL_X: {
		val->val1 = data->raw.acc.x;
		break;
	}
	case SENSOR_CHAN_ACCEL_Y: {
		val->val1 = data->raw.acc.y;
		break;
	}
	case SENSOR_CHAN_ACCEL_Z: {
		val->val1 = data->raw.acc.z;
		break;
	}
	case SENSOR_CHAN_GREEN: {
		val->val1 = data->raw.PPG1;
		break;
	}
	case SENSOR_CHAN_IR: {
		val->val1 = data->raw.PPG2;
		break;
	}
	case SENSOR_CHAN_RED: {
		val->val1 = data->raw.PPG3;
		break;
	}
	case SENSOR_CHAN_MAX32664C_HEARTRATE: {
#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
		val->val1 = data->ext.hr;
		val->val2 = data->ext.hr_confidence;
#else
		val->val1 = data->report.hr;
		val->val2 = data->report.hr_confidence;
#endif
		break;
	}
	case SENSOR_CHAN_MAX32664C_RESPIRATION_RATE: {
#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
		val->val1 = data->ext.rr;
		val->val2 = data->ext.rr_confidence;
#else
		val->val1 = data->report.rr;
		val->val2 = data->report.rr_confidence;
#endif
		break;
	}
	case SENSOR_CHAN_MAX32664C_BLOOD_OXYGEN_SATURATION: {
#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
		val->val1 = data->ext.spo2_meas.value;
		val->val2 = data->ext.spo2_meas.confidence;
#else
		val->val1 = data->report.spo2_meas.value;
		val->val2 = data->report.spo2_meas.confidence;
#endif
		break;
	}
	case SENSOR_CHAN_MAX32664C_SKIN_CONTACT: {
		val->val1 = data->report.scd_state;
		break;
	}
	default: {
		LOG_ERR("Channel %u not supported!", chan);
		return -ENOTSUP;
	}
	}

	return 0;
}

static int max32664c_attr_set(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	int err;
	uint8_t tx[5];
	uint8_t rx;
	struct max32664c_data *data = dev->data;

	err = 0;

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY: {
		break;
	}
	case SENSOR_ATTR_MAX32664C_HEIGHT: {
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x06;
		tx[3] = (val->val1 & 0xFF00) >> 8;
		tx[4] = val->val1 & 0x00FF;
		if (max32664c_i2c_transmit(dev, tx, 5, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			LOG_ERR("Can not set height!");
			return -EINVAL;
		}

		break;
	}
	case SENSOR_ATTR_MAX32664C_WEIGHT: {
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x07;
		tx[3] = (val->val1 & 0xFF00) >> 8;
		tx[4] = val->val1 & 0x00FF;
		if (max32664c_i2c_transmit(dev, tx, 5, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			LOG_ERR("Can not set weight!");
			return -EINVAL;
		}

		break;
	}
	case SENSOR_ATTR_MAX32664C_AGE: {
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x08;
		tx[3] = val->val1 & 0x00FF;
		if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			LOG_ERR("Can not set age!");
			return -EINVAL;
		}

		break;
	}
	case SENSOR_ATTR_MAX32664C_GENDER: {
		tx[0] = 0x50;
		tx[1] = 0x07;
		tx[2] = 0x08;
		tx[3] = val->val1 & 0x00FF;
		if (max32664c_i2c_transmit(dev, tx, 4, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			LOG_ERR("Can not set gender!");
			return -EINVAL;
		}

		break;
	}
	case SENSOR_ATTR_SLOPE_DUR: {
		data->motion_time = val->val1;
		break;
	}
	case SENSOR_ATTR_SLOPE_TH: {
		data->motion_threshold = val->val1;
		break;
	}
	case SENSOR_ATTR_CONFIGURATION: {
		switch ((int)chan) {
		case SENSOR_CHAN_GREEN: {
			data->led_current[0] = val->val1 & 0xFF;
			break;
		}
		case SENSOR_CHAN_IR: {
			data->led_current[1] = val->val1 & 0xFF;
			break;
		}
		case SENSOR_CHAN_RED: {
			data->led_current[2] = val->val1 & 0xFF;
			break;
		}
		default: {
			LOG_ERR("Channel %u not supported for setting attribute!", (int)chan);
			return -ENOTSUP;
		}
		}
		break;
	}
	case SENSOR_ATTR_MAX32664C_OP_MODE: {
		switch (val->val1) {
		case MAX32664C_OP_MODE_ALGO_AEC: {
#ifndef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
			err = max32664c_set_mode_algo(dev, MAX32664C_OP_MODE_ALGO_AEC, val->val2,
						      false);
#else
			return -EINVAL;
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
			break;
		}
		case MAX32664C_OP_MODE_ALGO_AEC_EXT: {
#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
			err = max32664c_set_mode_algo(dev, MAX32664C_OP_MODE_ALGO_AEC, val->val2,
						      true);
#else
			return -EINVAL;
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
			break;
		}
		case MAX32664C_OP_MODE_ALGO_AGC: {
#ifndef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
			err = max32664c_set_mode_algo(dev, MAX32664C_OP_MODE_ALGO_AGC, val->val2,
						      false);
#else
			return -EINVAL;
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
			break;
		}
		case MAX32664C_OP_MODE_ALGO_AGC_EXT: {
#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
			err = max32664c_set_mode_algo(dev, MAX32664C_OP_MODE_ALGO_AGC, val->val2,
						      true);
#else
			return -EINVAL;
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
			break;
		}
		case MAX32664C_OP_MODE_RAW: {
			err = max32664c_set_mode_raw(dev);
			break;
		}
		case MAX32664C_OP_MODE_SCD: {
			err = max32664c_set_mode_scd(dev);
			break;
		}
		case MAX32664C_OP_MODE_WAKE_ON_MOTION: {
			err = max32664c_set_mode_wake_on_motion(dev);
			break;
		}
		case MAX32664C_OP_MODE_EXIT_WAKE_ON_MOTION: {
			err = max32664c_exit_mode_wake_on_motion(dev);
			break;
		}
		case MAX32664C_OP_MODE_STOP_ALGO: {
			err = max32664c_stop_algo(dev);
			break;
		}
		case MAX32664C_OP_MODE_IDLE: {
			err = max32664c_disable_sensors(dev);
			break;
		}
		default: {
			LOG_ERR("Unsupported sensor operation mode");
			return -ENOTSUP;
		}
		}

		break;
	}
	default: {
		LOG_ERR("Unsupported sensor attribute!");
		return -ENOTSUP;
	}
	}

	return err;
}

static int max32664c_attr_get(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, struct sensor_value *val)
{
	struct max32664c_data *data = dev->data;

	switch ((int)attr) {
	case SENSOR_ATTR_MAX32664C_OP_MODE: {
		val->val1 = data->op_mode;
		val->val2 = 0;
		break;
	}
	case SENSOR_ATTR_CONFIGURATION: {
		switch ((int)chan) {
		case SENSOR_CHAN_GREEN: {
			val->val1 = data->led_current[0];
			break;
		}
		case SENSOR_CHAN_IR: {
			val->val1 = data->led_current[1];
			break;
		}
		case SENSOR_CHAN_RED: {
			val->val1 = data->led_current[2];
			break;
		}
		default: {
			LOG_ERR("Channel %u not supported for getting attribute!", (int)chan);
			return -ENOTSUP;
		}
		}
		break;
	}
	default: {
		LOG_ERR("Unsupported sensor attribute!");
		return -ENOTSUP;
	}
	}

	return 0;
}

static DEVICE_API(sensor, max32664c_driver_api) = {
	.attr_set = max32664c_attr_set,
	.attr_get = max32664c_attr_get,
	.sample_fetch = max32664c_sample_fetch,
	.channel_get = max32664c_channel_get,
};

static int max32664c_init(const struct device *dev)
{
	uint8_t tx[2];
	uint8_t rx[4];
	const struct max32664c_config *config = dev->config;
	struct max32664c_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT);
	gpio_pin_configure_dt(&config->mfio_gpio, GPIO_OUTPUT);

	/* Put the hub into application mode */
	LOG_DBG("Set app mode");
	gpio_pin_set_dt(&config->reset_gpio, false);
	k_msleep(20);

	gpio_pin_set_dt(&config->mfio_gpio, true);
	k_msleep(20);

	/* Wait for 50 ms (switch into app mode) + 1500 ms (initialization) */
	/* (see page 17 of the User Guide) */
	gpio_pin_set_dt(&config->reset_gpio, true);
	k_msleep(1600);

	/* Read the device mode */
	tx[0] = 0x02;
	tx[1] = 0x00;
	if (max32664c_i2c_transmit(dev, tx, 2, rx, 2, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	data->op_mode = rx[1];
	LOG_DBG("Mode: %x ", data->op_mode);
	if (data->op_mode != 0) {
		return -EINVAL;
	}

	/* Read the firmware version */
	tx[0] = 0xFF;
	tx[1] = 0x03;
	if (max32664c_i2c_transmit(dev, tx, 2, rx, 4, MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	memcpy(data->hub_ver, &rx[1], 3);

	LOG_DBG("Version: %d.%d.%d", data->hub_ver[0], data->hub_ver[1], data->hub_ver[2]);

	if (max32664c_check_sensors(dev)) {
		return -EINVAL;
	}

	if (max32664c_init_hub(dev)) {
		return -EINVAL;
	}

#ifdef CONFIG_MAX32664C_USE_STATIC_MEMORY
	k_msgq_init(&data->raw_report_queue, data->raw_report_queue_buffer,
		    sizeof(struct max32664c_raw_report_t),
		    sizeof(data->raw_report_queue_buffer) / sizeof(struct max32664c_raw_report_t));

	k_msgq_init(&data->scd_report_queue, data->scd_report_queue_buffer,
		    sizeof(struct max32664c_scd_report_t),
		    sizeof(data->scd_report_queue_buffer) / sizeof(struct max32664c_scd_report_t));

#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
	k_msgq_init(&data->ext_report_queue, data->ext_report_queue_buffer,
		    sizeof(struct max32664c_ext_report_t),
		    sizeof(data->ext_report_queue_buffer) / sizeof(struct max32664c_ext_report_t));
#else
	k_msgq_init(&data->report_queue, data->report_queue_buffer,
		    sizeof(struct max32664c_report_t),
		    sizeof(data->report_queue_buffer) / sizeof(struct max32664c_report_t));
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int max32664c_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME: {
		break;
	}
	case PM_DEVICE_ACTION_SUSPEND: {
		const struct max32664c_config *config = dev->config;

		/* Pulling MFIO high will cause the hub to enter sleep mode */
		gpio_pin_set_dt(&config->mfio_gpio, true);
		k_msleep(20);
		break;
	}
	case PM_DEVICE_ACTION_TURN_OFF: {
		uint8_t rx;
		uint8_t tx[3];

		/* Send a shut down command */
		/* NOTE: Toggling RSTN is needed to wake the device */
		tx[0] = 0x01;
		tx[1] = 0x00;
		tx[2] = 0x01;
		if (max32664c_i2c_transmit(dev, tx, 3, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
			return -EINVAL;
		}
		break;
	}
	case PM_DEVICE_ACTION_TURN_ON: {
		/* Toggling RSTN is needed to turn the device on */
		max32664c_init(dev);
		break;
	}
	default: {
		return -ENOTSUP;
	}
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#define MAX32664C_INIT(inst)                                                                       \
	static struct max32664c_data max32664c_data_##inst;                                        \
                                                                                                   \
	static const struct max32664c_config max32664c_config_##inst = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                            \
		.mfio_gpio = GPIO_DT_SPEC_INST_GET(inst, mfio_gpios),                              \
		.spo2_calib = DT_INST_PROP(inst, spo2_calib),                                      \
		.hr_config = DT_INST_PROP(inst, hr_config),                                        \
		.spo2_config = DT_INST_PROP(inst, spo2_config),                                    \
		.use_max86141 = DT_INST_PROP(inst, use_max86141),                                  \
		.use_max86161 = DT_INST_PROP(inst, use_max86161),                                  \
		.motion_time = DT_INST_PROP(inst, motion_time),                                    \
		.motion_threshold = DT_INST_PROP(inst, motion_threshold),                          \
		.min_integration_time_idx = DT_INST_ENUM_IDX(inst, min_integration_time),          \
		.min_sampling_rate_idx = DT_INST_ENUM_IDX(inst, min_sampling_rate),                \
		.max_integration_time_idx = DT_INST_ENUM_IDX(inst, max_integration_time),          \
		.max_sampling_rate_idx = DT_INST_ENUM_IDX(inst, max_sampling_rate),                \
		.report_period = DT_INST_PROP(inst, report_period),                                \
		.led_current = DT_INST_PROP(inst, led_current),                                    \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, max32664c_pm_action);                                       \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, max32664c_init, PM_DEVICE_DT_INST_GET(inst),            \
				     &max32664c_data_##inst, &max32664c_config_##inst,             \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                     \
				     &max32664c_driver_api)

DT_INST_FOREACH_STATUS_OKAY(MAX32664C_INIT)
