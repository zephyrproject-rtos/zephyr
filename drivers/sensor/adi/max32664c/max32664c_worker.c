/*
 * Background worker for the MAX32664C biometric sensor hub.
 *
 * Copyright (c) 2025, Daniel Kampert
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>

#include "max32664c.h"

LOG_MODULE_REGISTER(maxim_max32664c_worker, CONFIG_SENSOR_LOG_LEVEL);

/** @brief              Read the status from the sensor hub.
 *                      NOTE: Table 7 Sensor Hub Status Byte
 *  @param dev          Pointer to device
 *  @param status       Pointer to status byte
 *  @param i2c_error    Pointer to I2C error byte
 *  @return             0 when successful, otherwise an error code
 */
static int max32664c_get_hub_status(const struct device *dev, uint8_t *status, uint8_t *i2c_error)
{
	uint8_t tx[2] = {0x00, 0x00};
	uint8_t rx[2];

	if (max32664c_i2c_transmit(dev, tx, sizeof(tx), rx, sizeof(rx),
				   MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	*i2c_error = rx[0];
	*status = rx[1];

	return 0;
}

/** @brief      Read the FIFO sample count.
 *  @param dev  Pointer to device
 *  @param fifo Pointer to FIFO count
 */
static int max32664c_get_fifo_count(const struct device *dev, uint8_t *fifo)
{
	uint8_t tx[2] = {0x12, 0x00};
	uint8_t rx[2];

	if (max32664c_i2c_transmit(dev, tx, sizeof(tx), rx, sizeof(rx),
				   MAX32664C_DEFAULT_CMD_DELAY)) {
		return -EINVAL;
	}

	*fifo = rx[1];

	return rx[0];
}

/** @brief      Push a item into the message queue.
 *  @param msgq Pointer to message queue
 *  @param data Pointer to data to push
 */
static void max32664c_push_to_queue(struct k_msgq *msgq, const void *data)
{
	while (k_msgq_put(msgq, data, K_NO_WAIT) != 0) {
		k_msgq_purge(msgq);
	}
}

/** @brief          Process the buffer to get the raw data from the sensor hub.
 *  @param dev      Pointer to device
 */
static void max32664c_parse_and_push_raw(const struct device *dev)
{
	struct max32664c_data *data = dev->data;
	struct max32664c_raw_report_t report;

	report.PPG1 = ((uint32_t)(data->max32664_i2c_buffer[1]) << 16) |
		       ((uint32_t)(data->max32664_i2c_buffer[2]) << 8) |
		       data->max32664_i2c_buffer[3];
	report.PPG2 = ((uint32_t)(data->max32664_i2c_buffer[4]) << 16) |
		       ((uint32_t)(data->max32664_i2c_buffer[5]) << 8) |
		       data->max32664_i2c_buffer[6];
	report.PPG3 = ((uint32_t)(data->max32664_i2c_buffer[7]) << 16) |
		       ((uint32_t)(data->max32664_i2c_buffer[8]) << 8) |
		       data->max32664_i2c_buffer[9];

	/* PPG4 to 6 are used for PD2 */
	report.PPG4 = 0;
	report.PPG5 = 0;
	report.PPG6 = 0;

	report.acc.x =
		((int16_t)(data->max32664_i2c_buffer[19]) << 8) | data->max32664_i2c_buffer[20];
	report.acc.y =
		((int16_t)(data->max32664_i2c_buffer[21]) << 8) | data->max32664_i2c_buffer[22];
	report.acc.z =
		((int16_t)(data->max32664_i2c_buffer[23]) << 8) | data->max32664_i2c_buffer[24];

	max32664c_push_to_queue(&data->raw_report_queue, &report);
}

#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
/** @brief          Process the buffer to get the extended report data from the sensor hub.
 *  @param dev      Pointer to device
 */
static void max32664c_parse_and_push_ext_report(const struct device *dev)
{
	struct max32664c_data *data = dev->data;
	struct max32664c_ext_report_t report;

	report.op_mode = data->max32664_i2c_buffer[25];
	report.hr =
		(((uint16_t)(data->max32664_i2c_buffer[26]) << 8) | data->max32664_i2c_buffer[27]) /
		10;
	report.hr_confidence = data->max32664_i2c_buffer[28];
	report.rr =
		(((uint16_t)(data->max32664_i2c_buffer[29]) << 8) | data->max32664_i2c_buffer[30]) /
		10;
	report.rr_confidence = data->max32664_i2c_buffer[31];
	report.activity_class = data->max32664_i2c_buffer[32];
	report.total_walk_steps = data->max32664_i2c_buffer[33] |
				   ((uint32_t)(data->max32664_i2c_buffer[34]) << 8) |
				   ((uint32_t)(data->max32664_i2c_buffer[35]) << 16) |
				   ((uint32_t)(data->max32664_i2c_buffer[36]) << 24);
	report.total_run_steps = data->max32664_i2c_buffer[37] |
				  ((uint32_t)(data->max32664_i2c_buffer[38]) << 8) |
				  ((uint32_t)(data->max32664_i2c_buffer[39]) << 16) |
				  ((uint32_t)(data->max32664_i2c_buffer[40]) << 24);
	report.total_energy_kcal = data->max32664_i2c_buffer[41] |
				    ((uint32_t)(data->max32664_i2c_buffer[42]) << 8) |
				    ((uint32_t)(data->max32664_i2c_buffer[43]) << 16) |
				    ((uint32_t)(data->max32664_i2c_buffer[44]) << 24);
	report.total_amr_kcal = data->max32664_i2c_buffer[45] |
				 ((uint32_t)(data->max32664_i2c_buffer[46]) << 8) |
				 ((uint32_t)(data->max32664_i2c_buffer[47]) << 16) |
				 ((uint32_t)(data->max32664_i2c_buffer[48]) << 24);
	report.led_current_adj1.adj_flag = data->max32664_i2c_buffer[49];
	report.led_current_adj1.adj_val =
		(((uint16_t)(data->max32664_i2c_buffer[50]) << 8) | data->max32664_i2c_buffer[51]) /
		10;
	report.led_current_adj2.adj_flag = data->max32664_i2c_buffer[52];
	report.led_current_adj2.adj_val =
		(((uint16_t)(data->max32664_i2c_buffer[53]) << 8) | data->max32664_i2c_buffer[54]) /
		10;
	report.led_current_adj3.adj_flag = data->max32664_i2c_buffer[55];
	report.led_current_adj3.adj_val =
		(((uint16_t)(data->max32664_i2c_buffer[56]) << 8) | data->max32664_i2c_buffer[57]) /
		10;
	report.integration_time_adj_flag = data->max32664_i2c_buffer[58];
	report.requested_integration_time = data->max32664_i2c_buffer[59];
	report.sampling_rate_adj_flag = data->max32664_i2c_buffer[60];
	report.requested_sampling_rate = data->max32664_i2c_buffer[61];
	report.requested_sampling_average = data->max32664_i2c_buffer[62];
	report.hrm_afe_ctrl_state = data->max32664_i2c_buffer[63];
	report.is_high_motion_for_hrm = data->max32664_i2c_buffer[64];
	report.scd_state = data->max32664_i2c_buffer[65];
	report.r_value =
		(((uint16_t)(data->max32664_i2c_buffer[66]) << 8) | data->max32664_i2c_buffer[67]) /
		1000;
	report.spo2_meas.confidence = data->max32664_i2c_buffer[68];
	report.spo2_meas.value =
		(((uint16_t)(data->max32664_i2c_buffer[69]) << 8) | data->max32664_i2c_buffer[70]) /
		10;
	report.spo2_meas.valid_percent = data->max32664_i2c_buffer[71];
	report.spo2_meas.low_signal_flag = data->max32664_i2c_buffer[72];
	report.spo2_meas.motion_flag = data->max32664_i2c_buffer[73];
	report.spo2_meas.low_pi_flag = data->max32664_i2c_buffer[74];
	report.spo2_meas.unreliable_r_flag = data->max32664_i2c_buffer[75];
	report.spo2_meas.state = data->max32664_i2c_buffer[76];
	report.ibi_offset = data->max32664_i2c_buffer[77];
	report.unreliable_orientation_flag = data->max32664_i2c_buffer[78];
	report.reserved[0] = data->max32664_i2c_buffer[79];
	report.reserved[1] = data->max32664_i2c_buffer[80];

	max32664c_push_to_queue(&data->ext_report_queue, &report);
}
#else
/** @brief          Process the buffer to get the report data from the sensor hub.
 *  @param dev      Pointer to device
 */
static void max32664c_parse_and_push_report(const struct device *dev)
{
	struct max32664c_data *data = dev->data;
	struct max32664c_report_t report;

	report.op_mode = data->max32664_i2c_buffer[25];
	report.hr =
		(((uint16_t)(data->max32664_i2c_buffer[26]) << 8) | data->max32664_i2c_buffer[27]) /
		10;
	report.hr_confidence = data->max32664_i2c_buffer[28];
	report.rr =
		(((uint16_t)(data->max32664_i2c_buffer[29]) << 8) | data->max32664_i2c_buffer[30]) /
		10;
	report.rr_confidence = data->max32664_i2c_buffer[31];
	report.activity_class = data->max32664_i2c_buffer[32];
	report.r =
		(((uint16_t)(data->max32664_i2c_buffer[33]) << 8) | data->max32664_i2c_buffer[34]) /
		1000;
	report.spo2_meas.confidence = data->max32664_i2c_buffer[35];
	report.spo2_meas.value =
		(((uint16_t)(data->max32664_i2c_buffer[36]) << 8) | data->max32664_i2c_buffer[37]) /
		10;
	report.spo2_meas.complete = data->max32664_i2c_buffer[38];
	report.spo2_meas.low_signal_quality = data->max32664_i2c_buffer[39];
	report.spo2_meas.motion = data->max32664_i2c_buffer[40];
	report.spo2_meas.low_pi = data->max32664_i2c_buffer[41];
	report.spo2_meas.unreliable_r = data->max32664_i2c_buffer[42];
	report.spo2_meas.state = data->max32664_i2c_buffer[43];
	report.scd_state = data->max32664_i2c_buffer[44];

	max32664c_push_to_queue(&data->report_queue, &report);
}
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */

/** @brief      Worker thread to read the sensor hub.
 *              This thread does the following:
 *                  - It polls the sensor hub periodically for new results
 *                  - If new messages are available it reads the number of samples
 *                  - Then it reads all the samples to clear the FIFO.
 *                    It's necessary to clear the complete FIFO because the sensor hub
 *                    doesn´t support the reading of a single message and not clearing
 *                    the FIFO can cause a FIFO overrun.
 *                  - Extract the message data from the FIRST item from the FIFO and
 *                    copy them into the right message structure
 *                  - Put the message into a message queue
 *  @param dev  Pointer to device
 */
void max32664c_worker(const struct device *dev)
{
	int err;
	uint8_t fifo = 0;
	uint8_t status = 0;
	uint8_t i2c_error = 0;
	struct max32664c_data *data = dev->data;

	LOG_DBG("Starting worker thread for device: %s", dev->name);

	while (data->is_thread_running) {
		err = max32664c_get_hub_status(dev, &status, &i2c_error);
		if (err) {
			LOG_ERR("Failed to get hub status! Error: %d", err);
			continue;
		}

		if (!(status & (1 << MAX32664C_BIT_STATUS_DATA_RDY))) {
			LOG_WRN("No data ready! Status: 0x%X", status);
			k_msleep(100);
			continue;
		}

		err = max32664c_get_fifo_count(dev, &fifo);
		if (err) {
			LOG_ERR("Failed to get FIFO count! Error: %d", err);
			continue;
		}

		if (fifo == 0) {
			LOG_DBG("No data available in the FIFO.");
			continue;
		}
#ifdef CONFIG_MAX32664C_USE_STATIC_MEMORY
		else if (fifo > CONFIG_MAX32664C_SAMPLE_BUFFER_SIZE) {
			LOG_ERR("FIFO count %u exceeds maximum buffer size %u!",
				fifo, CONFIG_MAX32664C_SAMPLE_BUFFER_SIZE);

			/* TODO: Find a good way to clear the FIFO */
			continue;
		}
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */

#ifndef CONFIG_MAX32664C_USE_STATIC_MEMORY
#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
		size_t buffer_size = fifo * (sizeof(struct max32664c_raw_report_t) +
							sizeof(struct max32664c_ext_report_t)) +
						1;
#else
		size_t buffer_size = fifo * (sizeof(struct max32664c_raw_report_t) +
							sizeof(struct max32664c_report_t)) +
						1;
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */

		LOG_DBG("Allocating memory %u samples", fifo);
		LOG_DBG("Allocating memory for the I2C buffer with size: %u", buffer_size);
		data->max32664_i2c_buffer = (uint8_t *)k_malloc(buffer_size);

		if (data->max32664_i2c_buffer == NULL) {
			LOG_ERR("Can not allocate memory for the I2C buffer!");
			continue;
		}
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */

		uint8_t tx[2] = {0x12, 0x01};

		switch (data->op_mode) {
		case MAX32664C_OP_MODE_RAW: {
			/* Get all samples to clear the FIFO */
			max32664c_i2c_transmit(
				dev, tx, 2, data->max32664_i2c_buffer,
				(fifo * (sizeof(struct max32664c_raw_report_t))) +
					1,
				MAX32664C_DEFAULT_CMD_DELAY);

			if (data->max32664_i2c_buffer[0] != 0) {
				break;
			}

			max32664c_parse_and_push_raw(dev);

			break;
		}
#ifdef CONFIG_MAX32664C_USE_EXTENDED_REPORTS
		case MAX32664C_OP_MODE_ALGO_AEC_EXT:
		case MAX32664C_OP_MODE_ALGO_AGC_EXT: {

			/* Get all samples to clear the FIFO */
			max32664c_i2c_transmit(
				dev, tx, 2, data->max32664_i2c_buffer,
				(fifo * (sizeof(struct max32664c_raw_report_t) +
						sizeof(struct max32664c_ext_report_t))) +
					1,
				MAX32664C_DEFAULT_CMD_DELAY);

			if (data->max32664_i2c_buffer[0] != 0) {
				break;
			}

			max32664c_parse_and_push_raw(dev);
			max32664c_parse_and_push_ext_report(dev);

			break;
		}
#else
		case MAX32664C_OP_MODE_ALGO_AEC:
		case MAX32664C_OP_MODE_ALGO_AGC: {

			/* Get all samples to clear the FIFO */
			max32664c_i2c_transmit(
				dev, tx, 2, data->max32664_i2c_buffer,
				(fifo * (sizeof(struct max32664c_raw_report_t) +
						sizeof(struct max32664c_report_t))) +
					1,
				MAX32664C_DEFAULT_CMD_DELAY);

			if (data->max32664_i2c_buffer[0] != 0) {
				break;
			}

			max32664c_parse_and_push_raw(dev);
			max32664c_parse_and_push_report(dev);

			break;
		}
#endif /* CONFIG_MAX32664C_USE_EXTENDED_REPORTS */
		default: {
			break;
		}
		}

		if (data->max32664_i2c_buffer[0] != 0) {
			LOG_ERR("Can not read report! Status: 0x%X",
				data->max32664_i2c_buffer[0]);
		}

#ifndef CONFIG_MAX32664C_USE_STATIC_MEMORY
		k_free(data->max32664_i2c_buffer);
#endif /* CONFIG_MAX32664C_USE_STATIC_MEMORY */

		k_msleep(100);
	}
}
