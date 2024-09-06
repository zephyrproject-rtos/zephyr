/*
 * Copyright (c) 2023 Caspar Friedrich <c.s.w.friedrich@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER

/*
 * This requires to be included _after_  `#define ADC_CONTEXT_USES_KERNEL_TIMER`
 */
#include "adc_context.h"

LOG_MODULE_REGISTER(tla2021, CONFIG_ADC_LOG_LEVEL);

#define ACQ_THREAD_PRIORITY   CONFIG_ADC_TLA202X_ACQUISITION_THREAD_PRIORITY
#define ACQ_THREAD_STACK_SIZE CONFIG_ADC_TLA202X_ACQUISITION_THREAD_STACK_SIZE

#define MAX_CHANNELS   4
#define ADC_RESOLUTION 12

#define WAKEUP_TIME_US     25
#define CONVERSION_TIME_US 625 /* DR is 1600 SPS */

/*
 * Conversion Data Register (RP = 00h) [reset = 0000h]
 */
#define REG_DATA     0x00
#define REG_DATA_pos 4

/*
 * Configuration Register (RP = 01h) [reset = 8583h]
 */
#define REG_CONFIG          0x01
#define REG_CONFIG_DEFAULT  0x8583
#define REG_CONFIG_DR_pos   5
#define REG_CONFIG_MODE_pos 8
#define REG_CONFIG_PGA_pos  9               /* TLA2024 Only */
#define REG_CONFIG_PGA_msk  GENMASK(11, 9)  /* TLA2022 and TLA2024 Only */
#define REG_CONFIG_MUX_pos  12              /* TLA2024 Only */
#define REG_CONFIG_MUX_msk  GENMASK(14, 12) /* TLA2024 Only */
#define REG_CONFIG_OS_pos   15
#define REG_CONFIG_OS_msk   (BIT_MASK(1) << REG_CONFIG_OS_pos)

enum {
	MUX_DIFF_0_1 = 0,
	MUX_DIFF_0_3 = 1,
	MUX_DIFF_1_3 = 2,
	MUX_DIFF_2_3 = 3,
	MUX_SINGLE_0 = 4,
	MUX_SINGLE_1 = 5,
	MUX_SINGLE_2 = 6,
	MUX_SINGLE_3 = 7,
};

enum {
	/* Gain 1/3 */
	PGA_6144 = 0,
	/* Gain 1/2 */
	PGA_4096 = 1,
	/* Gain 1 (Default) */
	PGA_2048 = 2,
	/* Gain 2 */
	PGA_1024 = 3,
	/* Gain 4 */
	PGA_512 = 4,
	/* Gain 8 */
	PGA_256 = 5
};

typedef int16_t tla202x_reg_data_t;
typedef uint16_t tla202x_reg_config_t;

struct tla202x_config {
	const struct i2c_dt_spec bus;
	bool has_pga;
	uint8_t channel_count;
};

struct tla202x_data {
	const struct device *dev;
	struct adc_context ctx;
#ifdef CONFIG_ADC_ASYNC
	struct k_sem acq_lock;
#endif
	tla202x_reg_data_t *buffer;
	tla202x_reg_data_t *repeat_buffer;
	uint8_t channels;

	/*
	 * Shadow register
	 */
	tla202x_reg_config_t reg_config[MAX_CHANNELS];
};

static int tla202x_read_register(const struct device *dev, uint8_t reg, uint16_t *value)
{
	int ret;

	const struct tla202x_config *config = dev->config;
	uint8_t tmp[2];

	ret = i2c_write_read_dt(&config->bus, &reg, sizeof(reg), tmp, sizeof(tmp));
	if (ret) {
		return ret;
	}

	*value = sys_get_be16(tmp);

	return 0;
}

static int tla202x_write_register(const struct device *dev, uint8_t reg, uint16_t value)
{
	int ret;

	const struct tla202x_config *config = dev->config;
	uint8_t tmp[3] = {reg};

	sys_put_be16(value, &tmp[1]);

	ret = i2c_write_dt(&config->bus, tmp, sizeof(tmp));
	if (ret) {
		return ret;
	}

	return 0;
}

static int tla202x_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	const struct tla202x_config *config = dev->config;
	struct tla202x_data *data = dev->data;

	if (cfg->channel_id >= config->channel_count) {
		LOG_ERR("invalid channel selection %u", cfg->channel_id);
		return -EINVAL;
	}
	tla202x_reg_config_t *reg_config = &data->reg_config[cfg->channel_id];

	if (config->has_pga) {
		*reg_config &= ~REG_CONFIG_PGA_msk;
		switch (cfg->gain) {
		case ADC_GAIN_1_3:
			*reg_config |= PGA_6144 << REG_CONFIG_PGA_pos;
			break;
		case ADC_GAIN_1_2:
			*reg_config |= PGA_4096 << REG_CONFIG_PGA_pos;
			break;
		case ADC_GAIN_1:
			*reg_config |= PGA_2048 << REG_CONFIG_PGA_pos;
			break;
		case ADC_GAIN_2:
			*reg_config |= PGA_1024 << REG_CONFIG_PGA_pos;
			break;
		case ADC_GAIN_4:
			*reg_config |= PGA_512 << REG_CONFIG_PGA_pos;
			break;
		case ADC_GAIN_8:
			*reg_config |= PGA_256 << REG_CONFIG_PGA_pos;
			break;
		default:
			LOG_ERR("Invalid gain");
			return -EINVAL;
		}
	} else {
		if (cfg->gain != ADC_GAIN_1) {
			LOG_ERR("Invalid gain");
			return -EINVAL;
		}
	}

	/*
	 * Only adc with more than one channels has a mux
	 */
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	if (config->channel_count > 1) {
		*reg_config &= ~REG_CONFIG_MUX_msk;

		if (cfg->differential) {
			if (cfg->input_positive == 0 && cfg->input_negative == 1) {
				*reg_config |= MUX_DIFF_0_1 << REG_CONFIG_MUX_pos;
			} else if (cfg->input_positive == 0 && cfg->input_negative == 3) {
				*reg_config |= MUX_DIFF_0_3 << REG_CONFIG_MUX_pos;
			} else if (cfg->input_positive == 1 && cfg->input_negative == 3) {
				*reg_config |= MUX_DIFF_1_3 << REG_CONFIG_MUX_pos;
			} else if (cfg->input_positive == 2 && cfg->input_negative == 3) {
				*reg_config |= MUX_DIFF_2_3 << REG_CONFIG_MUX_pos;
			} else {
				LOG_ERR("Invalid channel config");
				return -EINVAL;
			}
		} else {
			if (cfg->input_positive == 0) {
				*reg_config |= MUX_SINGLE_0 << REG_CONFIG_MUX_pos;
			} else if (cfg->input_positive == 1) {
				*reg_config |= MUX_SINGLE_1 << REG_CONFIG_MUX_pos;
			} else if (cfg->input_positive == 2) {
				*reg_config |= MUX_SINGLE_2 << REG_CONFIG_MUX_pos;
			} else if (cfg->input_positive == 3) {
				*reg_config |= MUX_SINGLE_3 << REG_CONFIG_MUX_pos;
			} else {
				LOG_ERR("Invalid channel config");
				return -EINVAL;
			}
		}
	}
#endif

	if (cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid reference");
		return -EINVAL;
	}

	if (cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid acquisition time");
		return -EINVAL;
	}

	return 0;
}

static int tla202x_start_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct tla202x_data *data = dev->data;
	const struct tla202x_config *config = dev->config;

	const size_t num_extra_samples = seq->options ? seq->options->extra_samplings : 0;
	const size_t num_samples = (1 + num_extra_samples) * POPCOUNT(seq->channels);

	if (find_msb_set(seq->channels) > config->channel_count) {
		LOG_ERR("Selected channel(s) not supported: %x", seq->channels);
		return -EINVAL;
	}

	if (seq->resolution != ADC_RESOLUTION) {
		LOG_ERR("Selected resolution not supported: %d", seq->resolution);
		return -EINVAL;
	}

	if (seq->oversampling) {
		LOG_ERR("Oversampling is not supported");
		return -EINVAL;
	}

	if (seq->calibrate) {
		LOG_ERR("Calibration is not supported");
		return -EINVAL;
	}

	if (!seq->buffer) {
		LOG_ERR("Buffer invalid");
		return -EINVAL;
	}

	if (seq->buffer_size < (num_samples * sizeof(tla202x_reg_data_t))) {
		LOG_ERR("buffer size too small");
		return -EINVAL;
	}

	data->buffer = seq->buffer;

	adc_context_start_read(&data->ctx, seq);

	return adc_context_wait_for_completion(&data->ctx);
}

static int tla202x_read_async(const struct device *dev, const struct adc_sequence *seq,
			      struct k_poll_signal *async)
{
	int ret;

	struct tla202x_data *data = dev->data;

	adc_context_lock(&data->ctx, async ? true : false, async);
	ret = tla202x_start_read(dev, seq);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int tla202x_read(const struct device *dev, const struct adc_sequence *seq)
{
	return tla202x_read_async(dev, seq, NULL);
}

static void tla202x_perform_read(const struct device *dev)
{
	struct tla202x_data *data = dev->data;
	tla202x_reg_config_t reg;
	tla202x_reg_data_t res;
	uint8_t ch;
	int ret;

	while (data->channels) {
		/*
		 * Select correct channel
		 */
		ch = find_lsb_set(data->channels) - 1;
		reg = data->reg_config[ch];
		LOG_DBG("reg: %x", reg);

		/*
		 * Start single-shot conversion
		 */
		WRITE_BIT(reg, REG_CONFIG_MODE_pos, 1);
		WRITE_BIT(reg, REG_CONFIG_OS_pos, 1);
		ret = tla202x_write_register(dev, REG_CONFIG, reg);
		if (ret) {
			LOG_WRN("Failed to start conversion");
		}

		/*
		 * Wait until sampling is done
		 */
		k_usleep(WAKEUP_TIME_US + CONVERSION_TIME_US);
		do {
			k_yield();
			ret = tla202x_read_register(dev, REG_CONFIG, &reg);
			if (ret < 0) {
				adc_context_complete(&data->ctx, ret);
			}
		} while (!(reg & REG_CONFIG_OS_msk));

		/*
		 * Read result
		 */
		ret = tla202x_read_register(dev, REG_DATA, &res);
		if (ret) {
			adc_context_complete(&data->ctx, ret);
		}

		/*
		 * ADC data is stored in the upper 12 bits
		 */
		res >>= REG_DATA_pos;
		*data->buffer++ = res;

		LOG_DBG("read channel %d, result = %d", ch, res);
		WRITE_BIT(data->channels, ch, 0);
	}

	adc_context_on_sampling_done(&data->ctx, data->dev);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct tla202x_data *data = CONTAINER_OF(ctx, struct tla202x_data, ctx);
	const struct device *dev = data->dev;

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

#ifdef CONFIG_ADC_ASYNC
	k_sem_give(&data->acq_lock);
#else
	tla202x_perform_read(dev);
#endif
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct tla202x_data *data = CONTAINER_OF(ctx, struct tla202x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

#ifdef CONFIG_ADC_ASYNC
static void tla202x_acq_thread_fn(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct tla202x_data *data = dev->data;

	while (true) {
		k_sem_take(&data->acq_lock, K_FOREVER);

		tla202x_perform_read(dev);
	}
}
#endif

static int tla202x_init(const struct device *dev)
{
	int ret;

	const struct tla202x_config *config = dev->config;
	struct tla202x_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("Bus not ready");
		return -EINVAL;
	}

	for (uint8_t ch = 0; ch < MAX_CHANNELS; ch++) {
		data->reg_config[ch] = REG_CONFIG_DEFAULT;
	}

	ret = tla202x_write_register(dev, REG_CONFIG, data->reg_config[0]);
	if (ret) {
		LOG_ERR("Device reset failed: %d", ret);
		return ret;
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, tla202x_driver_api) = {
	.channel_setup = tla202x_channel_setup,
	.read = tla202x_read,
	.ref_internal = 4096,
#ifdef CONFIG_ADC_ASYNC
	.read_async = tla202x_read_async,
#endif
};

#define TLA202X_THREAD_INIT(t, n)                                                                  \
	K_THREAD_DEFINE(adc_##t##_##n##_thread, ACQ_THREAD_STACK_SIZE, tla202x_acq_thread_fn,      \
			DEVICE_DT_INST_GET(n), NULL, NULL, ACQ_THREAD_PRIORITY, 0, 0);

#define TLA202X_INIT(n, t, pga, channels)                                                          \
	IF_ENABLED(CONFIG_ADC_ASYNC, (TLA202X_THREAD_INIT(t, n)))                                  \
	static const struct tla202x_config inst_##t##_##n##_config = {                             \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.has_pga = pga,                                                                    \
		.channel_count = channels,                                                         \
	};                                                                                         \
	static struct tla202x_data inst_##t##_##n##_data = {                                       \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
		ADC_CONTEXT_INIT_LOCK(inst_##t##_##n##_data, ctx),                                 \
		ADC_CONTEXT_INIT_TIMER(inst_##t##_##n##_data, ctx),                                \
		ADC_CONTEXT_INIT_SYNC(inst_##t##_##n##_data, ctx),                                 \
		IF_ENABLED(CONFIG_ADC_ASYNC,                                                       \
			   (.acq_lock = Z_SEM_INITIALIZER(inst_##t##_##n##_data.acq_lock, 0, 1),)) \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &tla202x_init, NULL, &inst_##t##_##n##_data,                      \
			      &inst_##t##_##n##_config, POST_KERNEL,                               \
			      CONFIG_ADC_TLA202X_INIT_PRIORITY, &tla202x_driver_api);

#define DT_DRV_COMPAT        ti_tla2021
#define ADC_TLA2021_CHANNELS 1
#define ADC_TLA2021_PGA      false
DT_INST_FOREACH_STATUS_OKAY_VARGS(TLA202X_INIT, tla2021, ADC_TLA2021_PGA, ADC_TLA2021_CHANNELS)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT        ti_tla2022
#define ADC_TLA2022_CHANNELS 1
#define ADC_TLA2022_PGA      true
DT_INST_FOREACH_STATUS_OKAY_VARGS(TLA202X_INIT, tla2022, ADC_TLA2022_PGA, ADC_TLA2022_CHANNELS)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT        ti_tla2024
#define ADC_TLA2024_CHANNELS 4
#define ADC_TLA2024_PGA      true
DT_INST_FOREACH_STATUS_OKAY_VARGS(TLA202X_INIT, tla2024, ADC_TLA2024_PGA, ADC_TLA2024_CHANNELS)
#undef DT_DRV_COMPAT

BUILD_ASSERT(CONFIG_I2C_INIT_PRIORITY < CONFIG_ADC_TLA202X_INIT_PRIORITY);
