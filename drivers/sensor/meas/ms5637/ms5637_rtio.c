/*
 * Copyright (c) 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT meas_ms5637

#include <stdint.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>

#include "ms5637.h"

LOG_MODULE_DECLARE(MS5637, CONFIG_SENSOR_LOG_LEVEL);

#define MS5637_TEMP_SHIFT  16
#define MS5637_PRESS_SHIFT 16

static void ms5637_start_transfer(struct rtio_iodev_sqe *iodev_sqe,
				  const struct device *dev);

/* ------------------------------------------------------------------ */
/* MPSC dispatcher                                                     */
/* ------------------------------------------------------------------ */

static void ms5637_start_next(const struct device *dev)
{
	struct ms5637_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->mpsc_lock);

	if (data->pending_sqe != NULL) {
		k_spin_unlock(&data->mpsc_lock, key);
		return;
	}

	struct mpsc_node *node = mpsc_pop(&data->io_q);

	if (node == NULL) {
		k_spin_unlock(&data->mpsc_lock, key);
		return;
	}

	struct rtio_iodev_sqe *next_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

	data->pending_sqe = next_sqe;
	k_spin_unlock(&data->mpsc_lock, key);

	ms5637_start_transfer(next_sqe, dev);
}

/* ------------------------------------------------------------------ */
/* Completion callback                                                 */
/* ------------------------------------------------------------------ */

static void ms5637_complete_cb(struct rtio *r, const struct rtio_sqe *sqe,
				int res, void *arg)
{
	const struct device *dev = arg;
	struct ms5637_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;
	struct ms5637_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	uint64_t cycles;
	k_spinlock_key_t key;
	int rc;

	ARG_UNUSED(r);

	if (res < 0) {
		LOG_ERR("RTIO read failed: %d", res);
		rtio_iodev_sqe_err(iodev_sqe, res);
		goto check_next;
	}

	rc = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata), &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("rx buf alloc failed: %d", rc);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		goto check_next;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		goto check_next;
	}

	edata = (struct ms5637_encoded_data *)buf;
	edata->header.base_timestamp_ns = sensor_clock_cycles_to_ns(cycles);
	edata->header.reading_count = 1U;
	edata->adc_pressure    = ((uint32_t)data->raw_buffer[0] << 16) |
				 ((uint32_t)data->raw_buffer[1] << 8) |
				 (uint32_t)data->raw_buffer[2];
	edata->adc_temperature = ((uint32_t)data->raw_buffer[3] << 16) |
				 ((uint32_t)data->raw_buffer[4] << 8) |
				 (uint32_t)data->raw_buffer[5];
	edata->calibration = data->cal;

	rtio_iodev_sqe_ok(iodev_sqe, 0);

check_next:
	{
		key = k_spin_lock(&data->mpsc_lock);
		data->pending_sqe = NULL;
		k_spin_unlock(&data->mpsc_lock, key);
	}
	ms5637_start_next(dev);
}

static void ms5637_prep_conv_chain(struct rtio_sqe *conv,
				   struct rtio_sqe *delay,
				   struct rtio_sqe *adc_cmd,
				   struct rtio_sqe *adc_rd,
				   struct rtio_iodev *iodev,
				   const uint8_t *conv_cmd,
				   uint8_t delay_ms,
				   uint8_t *raw_buf)
{
	rtio_sqe_prep_tiny_write(conv, iodev, RTIO_PRIO_NORM, conv_cmd, 1, NULL);
	conv->iodev_flags = RTIO_IODEV_I2C_STOP;
	conv->flags       = RTIO_SQE_CHAINED;

	rtio_sqe_prep_delay(delay, K_MSEC(delay_ms), NULL);
	delay->flags = RTIO_SQE_CHAINED;

	rtio_sqe_prep_tiny_write(adc_cmd, iodev, RTIO_PRIO_NORM,
				 (uint8_t[]){MS5637_CMD_READ_ADC}, 1, NULL);
	adc_cmd->flags = RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(adc_rd, iodev, RTIO_PRIO_NORM,
			   raw_buf, MS5637_ADC_READ_LEN, NULL);
	adc_rd->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	adc_rd->flags       = RTIO_SQE_CHAINED;
}

/* ------------------------------------------------------------------ */
/* Build and submit the full 9-SQE chain (no workqueue, no blocking)  */
/*                                                                     */
/* p_conv → delay_p → p_adc_cmd → p_adc_rd →                         */
/* t_conv → delay_t → t_adc_cmd → t_adc_rd → cb                      */
/* ------------------------------------------------------------------ */

static void ms5637_start_transfer(struct rtio_iodev_sqe *iodev_sqe,
				  const struct device *dev)
{
	struct ms5637_data *data = dev->data;
	const struct ms5637_config *cfg = dev->config;
	struct rtio_sqe *p_conv, *delay_p, *p_adc_cmd, *p_adc_rd;
	struct rtio_sqe *t_conv, *delay_t, *t_adc_cmd, *t_adc_rd, *cb;
	k_spinlock_key_t key;
	int rc;

	p_conv    = rtio_sqe_acquire(cfg->r);
	delay_p   = rtio_sqe_acquire(cfg->r);
	p_adc_cmd = rtio_sqe_acquire(cfg->r);
	p_adc_rd  = rtio_sqe_acquire(cfg->r);
	t_conv    = rtio_sqe_acquire(cfg->r);
	delay_t   = rtio_sqe_acquire(cfg->r);
	t_adc_cmd = rtio_sqe_acquire(cfg->r);
	t_adc_rd  = rtio_sqe_acquire(cfg->r);
	cb        = rtio_sqe_acquire(cfg->r);

	if (!p_conv || !delay_p || !p_adc_cmd || !p_adc_rd ||
	    !t_conv || !delay_t || !t_adc_cmd || !t_adc_rd || !cb) {
		rtio_sqe_drop_all(cfg->r);
		key = k_spin_lock(&data->mpsc_lock);
		data->pending_sqe = NULL;
		k_spin_unlock(&data->mpsc_lock, key);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		ms5637_start_next(dev);
		return;
	}

	/* Pressure: SQEs 1-4 */
	ms5637_prep_conv_chain(p_conv, delay_p, p_adc_cmd, p_adc_rd,
			       cfg->bus_iodev,
			       &data->pressure_conv_cmd,
			       data->pressure_conv_delay,
			       &data->raw_buffer[0]);

	/* Temperature: SQEs 5-8 */
	ms5637_prep_conv_chain(t_conv, delay_t, t_adc_cmd, t_adc_rd,
			       cfg->bus_iodev,
			       &data->temperature_conv_cmd,
			       data->temperature_conv_delay,
			       &data->raw_buffer[3]);

	/* SQE 9: completion callback */
	rtio_sqe_prep_callback_no_cqe(cb, ms5637_complete_cb, (void *)dev, iodev_sqe);

	rc = rtio_submit(cfg->r, 0);
	if (rc < 0) {
		key = k_spin_lock(&data->mpsc_lock);
		data->pending_sqe = NULL;
		k_spin_unlock(&data->mpsc_lock, key);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		ms5637_start_next(dev);
	}
}

/* ------------------------------------------------------------------ */
/* submit                                                              */
/* ------------------------------------------------------------------ */

static int ms5637_validate_request(const struct sensor_read_config *cfg)
{
	if (cfg->is_streaming) {
		return -ENOTSUP;
	}

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->channels[i].chan_idx != 0) {
			return -ENOTSUP;
		}

		switch (cfg->channels[i].chan_type) {
		case SENSOR_CHAN_ALL:
		case SENSOR_CHAN_PRESS:
		case SENSOR_CHAN_AMBIENT_TEMP:
			break;
		default:
			return -ENOTSUP;
		}
	}

	return 0;
}

void ms5637_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct ms5637_data *data = dev->data;
	int ret;

	ret = ms5637_validate_request(cfg);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	mpsc_push(&data->io_q, &iodev_sqe->q);

	ms5637_start_next(dev);
}

/* ------------------------------------------------------------------ */
/* Decoder                                                             */
/* ------------------------------------------------------------------ */

/*
 * Q31 fixed-point conversions.
 *
 * Temperature is in centidegrees C:  q31 = centideg * 2^(31-shift) / 100
 * Pressure is in Pascals → kPa:      q31 = Pa * 2^(31-shift) / 1000
 *
 * Shift is fixed at 16 (Q15.16), giving ±2^15 range with ~0.0015°C
 * and ~0.015 Pa resolution — sufficient for the MS5637's 24-bit ADC.
 */
static q31_t ms5637_centi_to_q31(int32_t val)
{
	return (q31_t)(((int64_t)val * (INT64_C(1) << (31 - MS5637_TEMP_SHIFT))) / 100);
}

static q31_t ms5637_pa_to_kpa_q31(int32_t val)
{
	return (q31_t)(((int64_t)val * (INT64_C(1) << (31 - MS5637_PRESS_SHIFT))) / 1000);
}

static int ms5637_decoder_get_frame_count(const uint8_t *buffer,
					  struct sensor_chan_spec chan_spec,
					  uint16_t *frame_count)
{
	ARG_UNUSED(buffer);

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_PRESS:
	case SENSOR_CHAN_AMBIENT_TEMP:
		*frame_count = 1;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int ms5637_decoder_get_size_info(struct sensor_chan_spec chan_spec,
					size_t *base_size, size_t *frame_size)
{
	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_PRESS:
	case SENSOR_CHAN_AMBIENT_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int ms5637_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec chan_spec,
				 uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct ms5637_encoded_data *edata = (const struct ms5637_encoded_data *)buffer;
	struct ms5637_reading reading;
	struct sensor_q31_data *out = data_out;

	if ((max_count == 0U) || (*fit != 0U)) {
		return 0;
	}
	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	ms5637_compensate(&edata->calibration, edata->adc_temperature,
			  edata->adc_pressure, &reading);

	out->header.base_timestamp_ns = edata->header.base_timestamp_ns;
	out->header.reading_count = 1U;
	out->readings[0].timestamp_delta = 0U;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_PRESS:
		out->shift = MS5637_PRESS_SHIFT;
		out->readings[0].pressure =
			ms5637_pa_to_kpa_q31(reading.pressure);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		out->shift = MS5637_TEMP_SHIFT;
		out->readings[0].temperature =
			ms5637_centi_to_q31(reading.temperature);
		break;
	default:
		return -ENOTSUP;
	}

	*fit = 1;
	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = ms5637_decoder_get_frame_count,
	.get_size_info   = ms5637_decoder_get_size_info,
	.decode          = ms5637_decoder_decode,
};

int ms5637_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
