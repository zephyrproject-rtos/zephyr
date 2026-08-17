/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for the TI ADS7828 and ADS7830 I2C ADCs.
 *
 * The two parts share the same command byte and channel permutation, and
 * differ only in resolution: ADS7828 is 12-bit (two result bytes, right
 * aligned), ADS7830 is 8-bit (one byte).
 *
 * Datasheets:
 *	https://www.ti.com/lit/ds/symlink/ads7828.pdf
 *	https://www.ti.com/lit/ds/symlink/ads7830.pdf
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#include "adc_context.h"

LOG_MODULE_REGISTER(adc_ads7828, CONFIG_ADC_LOG_LEVEL);

#define ADS7828_CHANNEL_COUNT 8U

/* Command byte: SD C2 C1 C0 PD1 PD0 X X */
#define ADS7828_CMD_SINGLE_ENDED BIT(7)
#define ADS7828_CMD_PD_INTERNAL  (BIT(3) | BIT(2)) /* internal reference on, ADC on */
#define ADS7828_CMD_PD_EXTERNAL  BIT(2)            /* internal reference off, ADC on */

struct ads7828_config {
	struct i2c_dt_spec i2c;
	uint8_t resolution;
	uint8_t sample_bytes;
};

struct ads7828_data {
	struct adc_context ctx;
	struct k_sem acq_sem;
	uint16_t *buffer;
	uint16_t *buffer_ptr;
	uint32_t channels;
	bool internal_ref;
};

/* Single-ended MUX order is CH0, CH2, CH4, CH6, CH1, CH3, CH5, CH7. */
static uint8_t ads7828_command(const struct device *dev, uint8_t channel)
{
	struct ads7828_data *data = dev->data;
	uint8_t sel = (channel >> 1) | ((channel & 0x01U) << 2);
	uint8_t pd = data->internal_ref ? ADS7828_CMD_PD_INTERNAL : ADS7828_CMD_PD_EXTERNAL;

	return ADS7828_CMD_SINGLE_ENDED | pd | (sel << 4);
}

static int ads7828_read_channel(const struct device *dev, uint8_t channel, uint16_t *sample)
{
	const struct ads7828_config *config = dev->config;
	uint8_t cmd = ads7828_command(dev, channel);
	uint8_t rx[sizeof(uint16_t)];
	int err;

	err = i2c_write_read_dt(&config->i2c, &cmd, sizeof(cmd), rx, config->sample_bytes);
	if (err != 0) {
		LOG_ERR("Channel %u read failed (err %d)", channel, err);
		return err;
	}

	if (config->sample_bytes == 1U) {
		*sample = rx[0];
	} else {
		*sample = sys_get_be16(rx) & BIT_MASK(12);
	}

	return 0;
}

static int ads7828_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	struct ads7828_data *data = dev->data;

	if (channel_cfg->channel_id >= ADS7828_CHANNEL_COUNT) {
		LOG_ERR("Invalid channel id %u", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain %d", channel_cfg->gain);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid acquisition time 0x%x", channel_cfg->acquisition_time);
		return -EINVAL;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		data->internal_ref = true;
		break;
	case ADC_REF_EXTERNAL0:
		data->internal_ref = false;
		break;
	default:
		LOG_ERR("Invalid channel reference %d", channel_cfg->reference);
		return -EINVAL;
	}

	if (data->internal_ref) {
		uint16_t dummy;

		/* One read enables the internal reference so it can settle. */
		return ads7828_read_channel(dev, 0, &dummy);
	}

	return 0;
}

static int ads7828_validate_sequence(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct ads7828_config *config = dev->config;
	size_t needed;

	if (sequence->resolution != config->resolution) {
		LOG_ERR("Unsupported resolution %u", sequence->resolution);
		return -ENOTSUP;
	}

	if (sequence->channels == 0 || find_msb_set(sequence->channels) > ADS7828_CHANNEL_COUNT) {
		LOG_ERR("Unsupported channels 0x%x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->oversampling != 0) {
		LOG_ERR("Oversampling is not supported");
		return -ENOTSUP;
	}

	if (sequence->calibrate) {
		LOG_ERR("Calibration is not supported");
		return -ENOTSUP;
	}

	needed = POPCOUNT(sequence->channels) * sizeof(uint16_t);
	if (sequence->options != NULL) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		LOG_ERR("Insufficient buffer size %zu, %zu needed", sequence->buffer_size, needed);
		return -ENOMEM;
	}

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads7828_data *data = CONTAINER_OF(ctx, struct ads7828_data, ctx);

	data->channels = ctx->sequence.channels;
	data->buffer_ptr = data->buffer;
	k_sem_give(&data->acq_sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads7828_data *data = CONTAINER_OF(ctx, struct ads7828_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_ptr;
	}
}

static int ads7828_perform_read(const struct device *dev)
{
	struct ads7828_data *data = dev->data;
	uint32_t channels = data->channels;
	int err;

	k_sem_take(&data->acq_sem, K_FOREVER);

	while (channels != 0) {
		uint8_t channel = (uint8_t)(find_lsb_set(channels) - 1);

		err = ads7828_read_channel(dev, channel, data->buffer);
		if (err != 0) {
			return err;
		}

		data->buffer++;
		channels &= ~BIT(channel);
	}

	adc_context_on_sampling_done(&data->ctx, dev);

	return 0;
}

static int ads7828_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ads7828_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, false, NULL);

	err = ads7828_validate_sequence(dev, sequence);
	if (err == 0) {
		data->buffer = sequence->buffer;
		adc_context_start_read(&data->ctx, sequence);

		/*
		 * Reading a channel triggers its conversion, so samples are
		 * fetched in the caller's context. adc_context_complete() posts
		 * ctx.sync on both the normal and error path, ending this loop.
		 */
		while (k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
			err = ads7828_perform_read(dev);
			if (err != 0) {
				adc_context_complete(&data->ctx, err);
			}
		}

		err = data->ctx.status;
	}

	adc_context_release(&data->ctx, err);

	return err;
}

static DEVICE_API(adc, ads7828_api) = {
	.channel_setup = ads7828_channel_setup,
	.read = ads7828_read,
};

static int ads7828_init(const struct device *dev)
{
	const struct ads7828_config *config = dev->config;
	struct ads7828_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus %s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	k_sem_init(&data->acq_sem, 0, 1);
	adc_context_init(&data->ctx);
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define ADS7828_INIT(t, inst, res, bytes)                                                          \
	static struct ads7828_data ads7828_data_##t##_##inst;                                      \
                                                                                                   \
	static const struct ads7828_config ads7828_config_##t##_##inst = {                         \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.resolution = (res),                                                               \
		.sample_bytes = (bytes),                                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ads7828_init, NULL, &ads7828_data_##t##_##inst,                \
			      &ads7828_config_##t##_##inst, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, \
			      &ads7828_api);

#define DT_DRV_COMPAT            ti_ads7828
#define ADS7828_INIT_12BIT(inst) ADS7828_INIT(ads7828, inst, 12, 2)
DT_INST_FOREACH_STATUS_OKAY(ADS7828_INIT_12BIT)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT           ti_ads7830
#define ADS7830_INIT_8BIT(inst) ADS7828_INIT(ads7830, inst, 8, 1)
DT_INST_FOREACH_STATUS_OKAY(ADS7830_INIT_8BIT)
#undef DT_DRV_COMPAT
