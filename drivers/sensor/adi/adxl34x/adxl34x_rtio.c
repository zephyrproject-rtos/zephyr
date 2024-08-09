/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

#include "adxl34x_reg.h"
#include "adxl34x_private.h"
#include "adxl34x_rtio.h"
#include "adxl34x_decoder.h"
#include "adxl34x_trigger.h"

LOG_MODULE_DECLARE(adxl34x, CONFIG_SENSOR_LOG_LEVEL);

/**
 * Fetch a single sample from the sensor
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] rx_buf Pointer to store the result
 * @param[in] buf_size Size of the @p rx_buf buffer in bytes
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_rtio_sample_fetch(const struct device *dev, uint8_t *rx_buf, uint8_t buf_size)
{
	const struct adxl34x_dev_config *config = dev->config;
	int rc;

	/* Read accel x, y and z values. */
	rc = config->bus_read_buf(dev, ADXL34X_REG_DATA, rx_buf, buf_size);
	if (rc) {
		LOG_ERR("Failed to read from device");
		return rc;
	}
	return 0;
}

/**
 * Find the trigger (if any) configured in the sensor read configuration
 *
 * @param[in] cfg Read configuration of this driver instance
 * @param[in] trig The trigger to lookup
 * @return The trigger if found, NULL otherwise
 */
static struct sensor_stream_trigger *
adxl34x_get_stream_trigger(const struct sensor_read_config *cfg, enum sensor_trigger_type trig)
{
	for (unsigned int i = 0; i < cfg->count; ++i) {
		if (cfg->triggers[i].trigger == trig) {
			return &cfg->triggers[i];
		}
	}
	return NULL;
}

/**
 * Flush all sensor data when indicated by the trigger
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] sensor_config Read configuration of this driver instance
 * @param[in] interrupted Indicate if an (specific) interrupt was detected
 * @param[in] trigger_type The type of trigger
 * @return 1 if the sensor data was dropped, 0 otherwise
 */
static int adxl34x_drop_data_on_trigger(const struct device *dev,
					const struct sensor_read_config *sensor_config,
					bool interrupted, enum sensor_trigger_type trigger_type)
{
	if (!interrupted) {
		return 0;
	}
	const struct sensor_stream_trigger *trigger =
		adxl34x_get_stream_trigger(sensor_config, trigger_type);
	if (trigger == NULL) {
		return 0;
	}
	if (trigger->opt == SENSOR_STREAM_DATA_NOP || trigger->opt == SENSOR_STREAM_DATA_DROP) {
		adxl34x_trigger_flush(dev); /* Clear the FIFO of the adxl34x. */
		return 1;
	}
	return 0;
}

/**
 * Submit a single packet to the RTIO stream
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] int_source The source(s) of the interrupt
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_submit_packet(const struct device *dev, struct adxl34x_int_source int_source)
{
	const struct adxl34x_dev_data *data = dev->data;
	const struct adxl34x_cfg *cfg = &data->cfg;
	const uint8_t nr_of_samples = cfg->fifo_ctl.samples;
	struct rtio_iodev_sqe *iodev_sqe = data->iodev_sqe;
	struct adxl34x_encoded_data *edata;
	const uint32_t min_buf_len = sizeof(struct adxl34x_encoded_data) +
				     sizeof(edata->fifo_data) * (nr_of_samples - 1);
	int rc;
	uint8_t *buf;
	uint32_t buf_len;
	uint8_t offset = 0;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context. */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0 || buf == NULL || buf_len < min_buf_len) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return -ENOBUFS;
	}

	/* Prepare response. */
	edata = (struct adxl34x_encoded_data *)buf;
	edata->header.entries = nr_of_samples;
	edata->header.range = cfg->data_format.range;
	edata->header.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	edata->header.trigger = int_source;

	/* Readout FIFO (x, y and z) data. */
	for (int i = 0; i < nr_of_samples; i++) {
		rc = adxl34x_rtio_sample_fetch(dev, edata->fifo_data + offset,
					       sizeof(edata->fifo_data));
		if (rc != 0) {
			LOG_ERR("Failed to get sensor samples");
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return rc;
		}
		offset += sizeof(edata->fifo_data);
	}

	rtio_iodev_sqe_ok(iodev_sqe, nr_of_samples);
	return 0;
}

/**
 * Handle both sensor data and trigger events
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_rtio_handle_motion_data(const struct device *dev)
{
	const struct adxl34x_dev_data *data = dev->data;
	const struct adxl34x_cfg *cfg = &data->cfg;
	struct rtio_iodev_sqe *iodev_sqe = data->iodev_sqe;
	int rc;

	if (iodev_sqe == NULL) {
		LOG_WRN("Not submitting packet, stream not started");
		return -ENOSTR;
	}

	const struct sensor_read_config *sensor_config = iodev_sqe->sqe.iodev->data;

	if (!sensor_config->is_streaming) {
		LOG_ERR("Failed to setup stream correctly");
		return -ENOSTR;
	}

	/* Read (and clear) any interrupts. */
	struct adxl34x_int_source int_source;

	rc = adxl34x_get_int_source(dev, &int_source);
	ARG_UNUSED(rc); /* Satisfy the static code analysers. */
	__ASSERT_NO_MSG(rc == 0);

	/* Handle motion related events as well (only if triggers are registered). */
	adxl34x_handle_motion_events(dev, int_source);

	if (int_source.overrun) {
		LOG_WRN("Lost accel samples, overrun detected");
	}
	/* Drop the data from the FIFO when the configured trigger indicates to do so. */
	if (adxl34x_drop_data_on_trigger(dev, sensor_config, int_source.overrun,
					 SENSOR_TRIG_FIFO_FULL) == 1) {
		return 0;
	}
	if (adxl34x_drop_data_on_trigger(dev, sensor_config, int_source.watermark,
					 SENSOR_TRIG_FIFO_WATERMARK) == 1) {
		return 0;
	}

	/* Check for spurious interrupts. */
	struct adxl34x_fifo_status fifo_status;

	rc = adxl34x_get_fifo_status(dev, &fifo_status);
	ARG_UNUSED(rc); /* Satisfy the static code analysers. */
	__ASSERT_NO_MSG(rc == 0);

	/* Check if the FIFO has enough data to create a packet. */
	const uint8_t nr_of_samples = cfg->fifo_ctl.samples;

	if (fifo_status.entries < nr_of_samples) {
		/* No (or not enough) samples to collect due to a spurious interrupt or motion
		 * event.
		 */
		return -ENODATA;
	}

	/* Create and send (submit) packet to user. */
	rc = adxl34x_submit_packet(dev, int_source);
	return rc;
}

/**
 * Start collecting streaming sensor data
 *
 * Streaming data is created when data ready interrupts arrive. This function only prepares the
 * driver to receive these interrupts, and makes sure the submission queue is available when data
 * arrives.
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] iodev_sqe IO device submission queue entry
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
#ifdef CONFIG_ADXL34X_TRIGGER
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	/* We only 'setup' the stream once to start the submitting of packages based on
	 * interrupts.
	 */
	if (data->iodev_sqe != NULL) {
		return 0;
	}
	data->iodev_sqe = iodev_sqe;

	/* Enable interrupts on both the MCU and ADXL34x side. */
	rc = adxl34x_trigger_init(dev);
	if (rc != 0) {
		LOG_ERR("Failed to enable the stream");
		return rc;
	}
	rc = adxl34x_trigger_reset(dev);
	if (rc != 0) {
		LOG_ERR("Failed to enable the stream");
		return rc;
	}
	return 0;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(iodev_sqe);
	return -ENOTSUP;
#endif /* CONFIG_ADXL34X_TRIGGER */
}

/**
 * Collect a single sample of data (x, y and z value) from the sensor
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] iodev_sqe IO device submission queue entry
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct adxl34x_dev_data *data = dev->data;
	const struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	const struct sensor_read_config *sensor_config = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = sensor_config->channels;
	const size_t num_channels = sensor_config->count;
	const uint32_t min_buf_len = sizeof(struct adxl34x_encoded_data);
	struct adxl34x_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context. */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return rc;
	}

	/* Determine what channels we need to fetch. */
	for (unsigned int i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_ALL:
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			break;
		default:
			rc = -ENOTSUP;
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return rc;
		}
	}

	/* Prepare response. */
	edata = (struct adxl34x_encoded_data *)buf;
	edata->header.entries = 1;
	edata->header.range = cfg->data_format.range;
	edata->header.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	edata->header.trigger = (struct adxl34x_int_source){.data_ready = 1};

	rc = adxl34x_rtio_sample_fetch(dev, edata->fifo_data, sizeof(edata->fifo_data));
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return rc;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
	return 0;
}

/**
 * Collect a single sample or a stream of samples from the sensor
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] iodev_sqe IO device submission queue entry
 */
void adxl34x_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *sensor_config = iodev_sqe->sqe.iodev->data;
	enum pm_device_state pm_state;
	int rc;

	rc = pm_device_state_get(dev, &pm_state);
	if (rc == 0 && pm_state != PM_DEVICE_STATE_ACTIVE) {
		LOG_DBG("Device is suspended, sensor is unavailable");
		return;
	}

	if (sensor_config->is_streaming) {
		adxl34x_submit_stream(dev, iodev_sqe);
	} else {
		adxl34x_submit_one_shot(dev, iodev_sqe);
	}
}
