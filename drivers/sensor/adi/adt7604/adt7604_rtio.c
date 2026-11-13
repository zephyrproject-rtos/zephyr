/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/work.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "adt7604.h"
#include "adt7604_bus.h"

LOG_MODULE_DECLARE(adt7604, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Read sensor result based on measurement type
 *
 * @param dev Pointer to the device structure
 * @param chan_num Hardware channel number
 * @param meas_type Type of measurement to read
 * @param result Pointer to store the result
 * @return 0 on success, negative error code otherwise
 */
static int read_channel_result(const struct device *dev, uint8_t chan_num,
			       enum adt7604_measurement_type meas_type, int32_t *result)
{
	if (meas_type == ADT7604_MEASUREMENT_TEMPERATURE) {
		return adt7604_read_temperature_result(dev, chan_num, result);
	} else if (meas_type == ADT7604_MEASUREMENT_RESISTANCE) {
		return adt7604_read_resistance_result(dev, chan_num, result);
	}
	return -ENOTSUP;
}

/**
 * @brief Get instance index for measurement type and increment counter
 *
 * @param meas_type Type of measurement
 * @param temp_instance Pointer to temperature instance counter
 * @param res_instance Pointer to resistance instance counter
 * @return Instance index, or -1 if measurement type should be skipped
 */
static int get_type_instance(enum adt7604_measurement_type meas_type, uint8_t *temp_instance,
			     uint8_t *res_instance)
{
	switch (meas_type) {
	case ADT7604_MEASUREMENT_TEMPERATURE:
		return (*temp_instance)++;
	case ADT7604_MEASUREMENT_RESISTANCE:
		return (*res_instance)++;
	default:
		return -1; /* Skip RSENSE/unassigned */
	}
}

/*
 * Returns true if this hardware channel (identified by sensor_type) should be
 * measured for the current read request. type_instance is the zero-based index
 * of this hardware channel among all channels of the same logical type seen so
 * far in the cfg->channels[] loop — the caller increments it after each call.
 */
static bool channel_requested(uint8_t sensor_type, const struct sensor_chan_spec *requested,
			      size_t num_requested, uint8_t type_instance)
{
	enum adt7604_measurement_type meas_type = adt7604_get_measurement_type(sensor_type);

	/* RSENSE and unassigned channels are never requested */
	if (meas_type == ADT7604_MEASUREMENT_NONE) {
		return false;
	}

	for (size_t i = 0; i < num_requested; i++) {
		enum sensor_channel ch = requested[i].chan_type;

		if (ch == SENSOR_CHAN_ALL) {
			return true; /* All measurement types match SENSOR_CHAN_ALL */
		}

		if (ch == SENSOR_CHAN_AMBIENT_TEMP &&
		    meas_type == ADT7604_MEASUREMENT_TEMPERATURE &&
		    requested[i].chan_idx == type_instance) {
			return true;
		}

		if (ch == SENSOR_CHAN_RESISTANCE && meas_type == ADT7604_MEASUREMENT_RESISTANCE &&
		    requested[i].chan_idx == type_instance) {
			return true;
		}
	}
	return false;
}

static void adt7604_submit_fetch(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = read_cfg->sensor;
	const struct adt7604_config *cfg = dev->config;
	struct adt7604_data *data = dev->data;
	uint32_t min_buf = sizeof(struct adt7604_encoded_data);
	uint8_t *buf;
	uint32_t buf_len;
	int rc;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf, min_buf, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	struct adt7604_encoded_data *edata = (struct adt7604_encoded_data *)buf;

	edata->timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	edata->temp_unit_celsius = data->temp_unit_celsius;

	uint8_t temp_instance = 0;
	uint8_t res_instance = 0;
	uint8_t n = 0;

	for (int i = 0; i < cfg->num_channels; i++) {
		uint8_t ch_num = cfg->channels[i].chan_num;
		uint8_t stype = cfg->channels[i].sensor_type;
		enum adt7604_measurement_type meas_type = adt7604_get_measurement_type(stype);

		int type_instance = get_type_instance(meas_type, &temp_instance, &res_instance);

		if (type_instance < 0 ||
		    !channel_requested(stype, read_cfg->channels, read_cfg->count, type_instance)) {
			continue;
		}

		rc = adt7604_start_conversion(dev, ch_num);
		if (rc < 0) {
			LOG_ERR("Failed to start conversion on channel %d", ch_num);
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return;
		}

		rc = adt7604_wait_status_done(dev);
		if (rc < 0) {
			LOG_ERR("Conversion timeout on channel %d", ch_num);
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return;
		}

		edata->readings[n].chan_num = ch_num;
		edata->readings[n].sensor_type = stype;
		edata->readings[n].raw = 0;

		int32_t result = 0;

		rc = read_channel_result(dev, ch_num, meas_type, &result);
		if (rc < 0) {
			LOG_ERR("Failed to read channel %d result", ch_num);
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return;
		}

		edata->readings[n].raw = result;
		n++;
	}

	edata->num_channels = n;
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void adt7604_submit(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe)
{
	ARG_UNUSED(sensor);

	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider increasing "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, adt7604_submit_fetch);
}
