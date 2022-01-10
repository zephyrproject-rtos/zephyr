/*
 * Copyright (c) 2022 Matija Tudan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <drivers/i2c.h>
#include <drivers/adc.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(adc_ads1015, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADS101X_RESOLUTION 12U

/* Default data rate (DR) is 1600 SPS */
#define ADS101X_DEFAULT_DR 4U

/* Register addresses */
enum {
	CONVERSION = 0x00,
	CONFIG     = 0x01,
};

/* Masks */
enum {
	DR	   = 0xE0,
	PGA	   = 0xE00,
	MUX	   = 0x7000,
	START_CONV = 0x8000,
};

/*
 * Translation from PGA bits to full-scale positive and negative input voltage
 * range in mV.
 */
static const uint16_t ads101x_fullscale_range[6] = {
	6144, /* ±6.144 V */
	4096, /* ±4.096 V */
	2048, /* ±2.048 V (default) */
	1024, /* ±1.024 V */
	512,  /* ±0.512 V */
	256,  /* ±0.256 V */
};

static const uint16_t ads101x_data_rate[7] = {
	128,  /* 128 SPS */
	250,  /* 250 SPS */
	490,  /* 490 SPS */
	920,  /* 920 SPS */
	1600, /* 1600 SPS (default) */
	2400, /* 2400 SPS */
	3300, /* 3300 SPS */
};

struct ads101x_config {
	const char *i2c_bus;
	uint8_t i2c_addr;
	uint8_t channels;
	uint16_t fsr;
};

struct ads101x_data {
	const struct device *dev;
	const struct device *i2c;
	struct adc_context ctx;
	int16_t *buffer;
	int16_t *repeat_buffer;
	uint8_t channels;
	struct k_thread thread;
	struct k_sem sem;
	uint16_t data_rate;

	K_KERNEL_STACK_MEMBER(stack,
			CONFIG_ADC_ADS101X_ACQUISITION_THREAD_STACK_SIZE);
};

static int ads101x_reg_read(const struct device *dev, uint8_t reg,
			      uint16_t *val)
{
	struct ads101x_data *data = dev->data;
	const struct ads101x_config *cfg = dev->config;

	if (i2c_burst_read(data->i2c, cfg->i2c_addr,
			   reg, (uint8_t *) val, 2) < 0) {
		LOG_ERR("I2C read failed");
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

static int ads101x_reg_write(const struct device *dev, uint8_t reg,
			       uint16_t val)
{
	struct ads101x_data *data = dev->data;
	const struct ads101x_config *cfg = dev->config;
	uint8_t buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write(data->i2c, buf, sizeof(buf), cfg->i2c_addr);
}

static int ads101x_set_dr(const struct device *dev, uint16_t acq_time)
{
	struct ads101x_data *data = dev->data;
	uint16_t cfg_reg;
	uint8_t dr;
	int err;

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		/* Set default 1600 SPS */
		dr = ADS101X_DEFAULT_DR;
		goto set_dr;
	}

	if (ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) {
		LOG_ERR("unsupported acquisition time unit");
		return -EINVAL;
	}

	/*
	 * Allow the caller to specify the data rate directly using
	 * ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, n) where n represents
	 * the index of ads101x_data_rate[] array.
	 */
	dr = ADC_ACQ_TIME_VALUE(acq_time);
	if (dr >= ARRAY_SIZE(ads101x_data_rate)) {
		LOG_ERR("ads101x_data_rate[] index out of range");
		return -EINVAL;
	}

set_dr:
	err = ads101x_reg_read(dev, CONFIG, &cfg_reg);
	if (err) {
		LOG_ERR("unable to read CONFIG reg");
		return -EIO;
	}

	cfg_reg &= ~DR;
	cfg_reg |= (dr << 5);

	err = ads101x_reg_write(dev, CONFIG, cfg_reg);
	if (err) {
		LOG_ERR("unable to write to CONFIG reg");
		return -EIO;
	}

	data->data_rate = ads101x_data_rate[dr];
	LOG_DBG("data rate set to %u SPS", data->data_rate);

	return 0;
}

static int ads101x_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct ads101x_config *config = dev->config;
	struct ads101x_data *data = dev->data;
	int err;

	if (channel_cfg->gain != ADC_GAIN_1_2) {
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("unsupported channel reference '%d'",
			channel_cfg->reference);
		return -ENOTSUP;
	}

	err = ads101x_set_dr(dev, channel_cfg->acquisition_time);
	if (err < 0) {
		LOG_ERR("unsupported channel acquisition_time '%d'",
			channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (channel_cfg->channel_id >= config->channels) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	return 0;
}

static int ads101x_validate_buffer_size(const struct device *dev,
					const struct adc_sequence *sequence)
{
	const struct ads101x_config *config = dev->config;
	uint8_t channels = 0;
	size_t needed;
	uint32_t mask;

	for (mask = BIT(config->channels - 1); mask != 0; mask >>= 1) {
		if (mask & sequence->channels) {
			channels++;
		}
	}

	needed = channels * sizeof(int16_t);
	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int ads101x_start_read(const struct device *dev,
			      const struct adc_sequence *sequence)
{
	const struct ads101x_config *config = dev->config;
	struct ads101x_data *data = dev->data;
	int err;

	if (sequence->resolution != ADS101X_RESOLUTION) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (find_msb_set(sequence->channels) > config->channels) {
		LOG_ERR("unsupported channels in mask: 0x%08x",
			sequence->channels);
		return -ENOTSUP;
	}

	err = ads101x_validate_buffer_size(dev, sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int ads101x_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct ads101x_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = ads101x_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static int ads101x_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	return ads101x_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads101x_data *data = CONTAINER_OF(ctx, struct ads101x_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct ads101x_data *data = CONTAINER_OF(ctx, struct ads101x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int ads101x_read_channel(const struct device *dev, uint8_t channel,
				int16_t *result)
{
	struct ads101x_data *data = dev->data;
	uint16_t cfg_reg, cfg_chn;
	int16_t conv;
	uint8_t delay;
	int err;

	err = ads101x_reg_read(dev, CONFIG, &cfg_reg);
	if (err) {
		LOG_ERR("unable to read CONFIG reg");
		return -EIO;
	}

	/* | MUX[2:0] | Input multiplexer configuration | */
	/* |----------|---------------------------------| */
	/* |   100    |   AINP = AIN0 and AINN = GND    | */
	/* |   101    |   AINP = AIN1 and AINN = GND    | */
	/* |   110    |   AINP = AIN2 and AINN = GND    | */
	/* |   111    |   AINP = AIN3 and AINN = GND    | */
	cfg_chn = channel + 4;

	cfg_chn <<= 12;
	cfg_reg &= ~MUX;
	cfg_reg |= cfg_chn;
	cfg_reg |= START_CONV;

	err = ads101x_reg_write(dev, CONFIG, cfg_reg);
	if (err) {
		LOG_ERR("unable to write to CONFIG reg");
		return -EIO;
	}

	/* The conversion time is equal to 1 / DR */
	delay = (1000 / data->data_rate) + 1;
	k_sleep(K_MSEC(delay));

	err = ads101x_reg_read(dev, CONVERSION, &conv);
	if (err) {
		LOG_ERR("unable to read CONVERSION reg");
		return -EIO;
	}

	/* | 15 14 13 12 11 10 9 8 7 6 5 4 | 3 2 1 0  | */
	/* |-------------------------------|----------| */
	/* |          DATA[11:0]           | Reserved | */
	conv >>= 4;

	*result = conv;

	return 0;
}

static void ads101x_acquisition_thread(struct ads101x_data *data)
{
	int16_t result = 0;
	uint8_t channel;
	int err;

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		while (data->channels) {
			channel = find_lsb_set(data->channels) - 1;

			LOG_DBG("reading channel %d", channel);

			err = ads101x_read_channel(data->dev, channel, &result);
			if (err) {
				LOG_ERR("failed to read channel %d (err %d)",
					channel, err);
				adc_context_complete(&data->ctx, err);
				break;
			}

			LOG_DBG("read channel %d, result = %d", channel,
				result);

			*data->buffer++ = result;
			WRITE_BIT(data->channels, channel, 0);
		}

		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static int ads101x_set_fsr(const struct device *dev)
{
	const struct ads101x_config *config = dev->config;
	bool fsr_supported = false;
	uint8_t pga = 0;
	uint16_t cfg_reg;
	int err;

	/* | PGA[2:0] |        FSR         | */
	/* |----------|--------------------| */
	/* |   000    | ±6.144 V           | */
	/* |   001    | ±4.096 V           | */
	/* |   010    | ±2.048 V (default) | */
	/* |   011    | ±1.024 V           | */
	/* |   100    | ±0.512 V           | */
	/* |   101    | ±0.256 V           | */
	for (pga = 0; pga < ARRAY_SIZE(ads101x_fullscale_range); pga++) {
		if (config->fsr == ads101x_fullscale_range[pga]) {
			fsr_supported = true;
			break;
		}
	}

	if (!fsr_supported) {
		LOG_ERR("unsupported fsr '%d'", config->fsr);
		return -ENOTSUP;
	}

	err = ads101x_reg_read(dev, CONFIG, &cfg_reg);
	if (err) {
		LOG_ERR("unable to read CONFIG reg");
		return -EIO;
	}

	cfg_reg &= ~PGA;
	cfg_reg |= (pga << 9);

	err = ads101x_reg_write(dev, CONFIG, cfg_reg);
	if (err) {
		LOG_ERR("unable to write to CONFIG reg");
		return -EIO;
	}

	LOG_DBG("full-scale range set to +-%u mV", ads101x_fullscale_range[pga]);

	return 0;
}

static int ads101x_init(const struct device *dev)
{
	const struct ads101x_config *config = dev->config;
	struct ads101x_data *data = dev->data;
	int err;

	data->dev = dev;
	k_sem_init(&data->sem, 0, 1);

	data->i2c = device_get_binding(config->i2c_bus);
	if (!data->i2c) {
		LOG_ERR("I2C device '%s' not found", config->i2c_bus);
		return -EINVAL;
	}

	err = ads101x_set_fsr(dev);
	if (err) {
		LOG_ERR("setting full-scale range failed");
		return err;
	}

	k_thread_create(&data->thread, data->stack,
			CONFIG_ADC_ADS101X_ACQUISITION_THREAD_STACK_SIZE,
			(k_thread_entry_t)ads101x_acquisition_thread,
			data, NULL, NULL,
			CONFIG_ADC_ADS101X_ACQUISITION_THREAD_PRIO,
			0, K_NO_WAIT);

	adc_context_unlock_unconditionally(&data->ctx);

	LOG_DBG("Init complete");

	return 0;
}

#define INST_DT_ADS101X(inst, t) DT_INST(inst, ti_ads##t)

#define ADS101X_DEVICE(t, n, ch) \
	static struct adc_driver_api ads##t##_adc_api_##n = { \
		.channel_setup = ads101x_channel_setup, \
		.read = ads101x_read, \
		.ref_internal = DT_PROP(INST_DT_ADS101X(n, t), fsr), \
		IF_ENABLED(CONFIG_ADC_ASYNC, \
			(.read_async = ads101x_read_async,)) \
	}; \
	static struct ads101x_data ads##t##_data_##n = { \
		ADC_CONTEXT_INIT_TIMER(ads##t##_data_##n, ctx), \
		ADC_CONTEXT_INIT_LOCK(ads##t##_data_##n, ctx), \
		ADC_CONTEXT_INIT_SYNC(ads##t##_data_##n, ctx), \
		.data_rate = ads101x_data_rate[ADS101X_DEFAULT_DR], \
	}; \
	static const struct ads101x_config ads##t##_config_##n = { \
		.i2c_bus = DT_BUS_LABEL(INST_DT_ADS101X(n, t)), \
		.i2c_addr = DT_REG_ADDR(INST_DT_ADS101X(n, t)), \
		.channels = ch, \
		.fsr = DT_PROP(INST_DT_ADS101X(n, t), fsr), \
	}; \
	DEVICE_DT_DEFINE(INST_DT_ADS101X(n, t), \
			 &ads101x_init, device_pm_control_nop, \
			 &ads##t##_data_##n, \
			 &ads##t##_config_##n, POST_KERNEL, \
			 CONFIG_ADC_ADS101X_INIT_PRIORITY, \
			 &ads##t##_adc_api_##n)

/*
 * ADS1014: 1 channel
 */
#define ADS1014_DEVICE(n) ADS101X_DEVICE(1014, n, 1)

/*
 * ADS1015: 4 channels
 */
#define ADS1015_DEVICE(n) ADS101X_DEVICE(1015, n, 4)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_ADS101X_FOREACH(t, inst_expr) \
	UTIL_LISTIFY(DT_NUM_INST_STATUS_OKAY(ti_ads##t), \
		     CALL_WITH_ARG, inst_expr)

INST_DT_ADS101X_FOREACH(1014, ADS1014_DEVICE);
INST_DT_ADS101X_FOREACH(1015, ADS1015_DEVICE);
