/* TI ADS1X1X ADC
 *
 * Copyright (c) 2021 Facebook, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(ADS1X1X, CONFIG_ADC_LOG_LEVEL);

#define ADS1X1X_CONFIG_OS BIT(15)
#define ADS1X1X_CONFIG_MUX(x) ((x) << 12)
#define ADS1X1X_CONFIG_PGA(x) ((x) << 9)
#define ADS1X1X_CONFIG_MODE BIT(8)
#define ADS1X1X_CONFIG_DR(x) ((x) << 5)
#define ADS1X1X_CONFIG_COMP_MODE BIT(4)
#define ADS1X1X_CONFIG_COMP_POL BIT(3)
#define ADS1X1X_CONFIG_COMP_LAT BIT(2)
#define ADS1X1X_CONFIG_COMP_QUE(x) (x)

enum ads1x1x_reg {
	ADS1X1X_REG_CONV = 0x00,
	ADS1X1X_REG_CONFIG = 0x01,
	ADS1X1X_REG_LO_THRESH = 0x02,
	ADS1X1X_REG_HI_THRESH = 0x03,
};

enum {
	ADS1X15_CONFIG_MUX_DIFF_0_1 = 0,
	ADS1X15_CONFIG_MUX_DIFF_0_3 = 1,
	ADS1X15_CONFIG_MUX_DIFF_1_3 = 2,
	ADS1X15_CONFIG_MUX_DIFF_2_3 = 3,
	ADS1X15_CONFIG_MUX_SINGLE_0 = 4,
	ADS1X15_CONFIG_MUX_SINGLE_1 = 5,
	ADS1X15_CONFIG_MUX_SINGLE_2 = 6,
	ADS1X15_CONFIG_MUX_SINGLE_3 = 7,
};

enum {
	/* ADS111X, ADS101X samples per second */
	/* 8, 128 samples per second */
	ADS1X1X_CONFIG_DR_8_128 = 0,
	/* 16, 250 samples per second */
	ADS1X1X_CONFIG_DR_16_250 = 1,
	/* 32, 490 samples per second */
	ADS1X1X_CONFIG_DR_32_490 = 2,
	/* 64, 920 samples per second */
	ADS1X1X_CONFIG_DR_64_920 = 3,
	/* 128, 1600 samples per second (default) */
	ADS1X1X_CONFIG_DR_128_1600 = 4,
	/* 250, 2400 samples per second */
	ADS1X1X_CONFIG_DR_250_2400 = 5,
	/* 475, 3300 samples per second */
	ADS1X1X_CONFIG_DR_475_3300 = 6,
	/* 860, 3300 samples per second */
	ADS1X1X_CONFIG_DR_860_3300 = 7,
	/* Default data rate */
	ADS1X1X_CONFIG_DR_DEFAULT = ADS1X1X_CONFIG_DR_128_1600
};

enum {
	/* +/-6.144V range = Gain 2/3 */
	ADS1X1X_CONFIG_PGA_6144 = 0,
	/* +/-4.096V range = Gain 1 */
	ADS1X1X_CONFIG_PGA_4096 = 1,
	/* +/-2.048V range = Gain 2 (default) */
	ADS1X1X_CONFIG_PGA_2048 = 2,
	/* +/-1.024V range = Gain 4 */
	ADS1X1X_CONFIG_PGA_1024 = 3,
	/* +/-0.512V range = Gain 8 */
	ADS1X1X_CONFIG_PGA_512 = 4,
	/* +/-0.256V range = Gain 16 */
	ADS1X1X_CONFIG_PGA_256 = 5
};

enum {
	ADS1X1X_CONFIG_MODE_CONTINUOUS = 0,
	ADS1X1X_CONFIG_MODE_SINGLE_SHOT = 1,
};

enum {
	/* Traditional comparator with hysteresis (default) */
	ADS1X1X_CONFIG_COMP_MODE_TRADITIONAL = 0,
	/* Window comparator */
	ADS1X1X_CONFIG_COMP_MODE_WINDOW = 1
};

enum {
	/* ALERT/RDY pin is low when active (default) */
	ADS1X1X_CONFIG_COMP_POLARITY_ACTIVE_LO = 0,
	/* ALERT/RDY pin is high when active */
	ADS1X1X_CONFIG_COMP_POLARITY_ACTIVE_HI = 1
};

enum {
	/* Non-latching comparator (default) */
	ADS1X1X_CONFIG_COMP_NON_LATCHING = 0,
	/* Latching comparator */
	ADS1X1X_CONFIG_COMP_LATCHING = 1
};

enum {
	/* Assert ALERT/RDY after one conversions */
	ADS1X1X_CONFIG_COMP_QUEUE_1 = 0,
	/* Assert ALERT/RDY after two conversions */
	ADS1X1X_CONFIG_COMP_QUEUE_2 = 1,
	/* Assert ALERT/RDY after four conversions */
	ADS1X1X_CONFIG_COMP_QUEUE_4 = 2,
	/* Disable the comparator and put ALERT/RDY in high state (default) */
	ADS1X1X_CONFIG_COMP_QUEUE_NONE = 3
};

struct ads1x1x_config {
	struct i2c_dt_spec bus;
	const uint32_t odr_delay[8];
	uint8_t resolution;
	bool multiplexer;
	bool pga;
};

struct ads1x1x_data {
	const struct device *dev;
	struct adc_context ctx;
	k_timeout_t ready_time;
	struct k_sem acq_sem;
	int16_t *buffer;
	int16_t *repeat_buffer;
	struct k_thread thread;
	bool differential;

	K_THREAD_STACK_MEMBER(stack, CONFIG_ADC_ADS1X1X_ACQUISITION_THREAD_STACK_SIZE);
};

static int ads1x1x_read_reg(const struct device *dev, enum ads1x1x_reg reg_addr, uint16_t *buf)
{
	const struct ads1x1x_config *config = dev->config;
	uint16_t reg_val;
	int ret;

	ret = i2c_burst_read_dt(&config->bus, reg_addr, (uint8_t *)&reg_val, sizeof(reg_val));
	if (ret != 0) {
		LOG_ERR("ADS1X1X[0x%X]: error reading register 0x%X (%d)", config->bus.addr,
			reg_addr, ret);
		return ret;
	}

	*buf = sys_be16_to_cpu(reg_val);

	return 0;
}

static int ads1x1x_write_reg(const struct device *dev, enum ads1x1x_reg reg_addr, uint16_t reg_val)
{
	const struct ads1x1x_config *config = dev->config;
	uint8_t buf[3];
	int ret;

	buf[0] = reg_addr;
	sys_put_be16(reg_val, &buf[1]);

	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));

	if (ret != 0) {
		LOG_ERR("ADS1X1X[0x%X]: error writing register 0x%X (%d)", config->bus.addr,
			reg_addr, ret);
		return ret;
	}

	return 0;
}

static int ads1x1x_start_conversion(const struct device *dev)
{
	/* send start sampling command */
	uint16_t config;

	ads1x1x_read_reg(dev, ADS1X1X_REG_CONFIG, &config);
	config |= ADS1X1X_CONFIG_OS;
	ads1x1x_write_reg(dev, ADS1X1X_REG_CONFIG, config);

	return 0;
}

static inline int ads1x1x_acq_time_to_dr(const struct device *dev, uint16_t acq_time)
{
	struct ads1x1x_data *data = dev->data;
	const struct ads1x1x_config *ads_config = dev->config;
	const uint32_t *odr_delay = ads_config->odr_delay;
	uint32_t odr_delay_us = 0;
	int odr = -EINVAL;
	uint16_t acq_value = ADC_ACQ_TIME_VALUE(acq_time);

	/* The ADS1x1x uses samples per seconds units with the lowest being 8SPS
	 * and with acquisition_time only having 14b for time, this will not fit
	 * within here for microsecond units. Use Tick units and allow the user to
	 * specify the ODR directly.
	 */
	if (acq_time != ADC_ACQ_TIME_DEFAULT && ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) {
		return -EINVAL;
	}

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		odr = ADS1X1X_CONFIG_DR_DEFAULT;
		odr_delay_us = odr_delay[ADS1X1X_CONFIG_DR_DEFAULT];
	} else {
		switch (acq_value) {
		case ADS1X1X_CONFIG_DR_8_128:
			odr = ADS1X1X_CONFIG_DR_8_128;
			odr_delay_us = odr_delay[ADS1X1X_CONFIG_DR_8_128];
			break;
		case ADS1X1X_CONFIG_DR_16_250:
			odr = ADS1X1X_CONFIG_DR_16_250;
			odr_delay_us = odr_delay[ADS1X1X_CONFIG_DR_16_250];
			break;
		case ADS1X1X_CONFIG_DR_32_490:
			odr = ADS1X1X_CONFIG_DR_32_490;
			odr_delay_us = odr_delay[ADS1X1X_CONFIG_DR_32_490];
			break;
		case ADS1X1X_CONFIG_DR_64_920:
			odr = ADS1X1X_CONFIG_DR_64_920;
			odr_delay_us = odr_delay[ADS1X1X_CONFIG_DR_64_920];
			break;
		case ADS1X1X_CONFIG_DR_128_1600:
			odr = ADS1X1X_CONFIG_DR_128_1600;
			odr_delay_us = odr_delay[ADS1X1X_CONFIG_DR_128_1600];
			break;
		case ADS1X1X_CONFIG_DR_250_2400:
			odr = ADS1X1X_CONFIG_DR_250_2400;
			odr_delay_us = odr_delay[ADS1X1X_CONFIG_DR_250_2400];
			break;
		case ADS1X1X_CONFIG_DR_475_3300:
			odr = ADS1X1X_CONFIG_DR_475_3300;
			odr_delay_us = odr_delay[ADS1X1X_CONFIG_DR_475_3300];
			break;
		case ADS1X1X_CONFIG_DR_860_3300:
			odr = ADS1X1X_CONFIG_DR_860_3300;
			odr_delay_us = odr_delay[ADS1X1X_CONFIG_DR_860_3300];
			break;
		default:
			break;
		}
	}

	/* As per the datasheet, 25us is needed to wake-up from power down mode
	 */
	odr_delay_us += 25;
	data->ready_time = K_USEC(odr_delay_us);

	return odr;
}

static int ads1x1x_wait_data_ready(const struct device *dev)
{
	int rc;
	struct ads1x1x_data *data = dev->data;

	k_sleep(data->ready_time);
	uint16_t status = 0;

	rc = ads1x1x_read_reg(dev, ADS1X1X_REG_CONFIG, &status);
	if (rc != 0) {
		return rc;
	}

	while (!(status & ADS1X1X_CONFIG_OS)) {
		k_sleep(K_USEC(100));
		rc = ads1x1x_read_reg(dev, ADS1X1X_REG_CONFIG, &status);
		if (rc != 0) {
			return rc;
		}
	}

	return rc;
}

static int ads1x1x_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct ads1x1x_config *ads_config = dev->config;
	struct ads1x1x_data *data = dev->data;
	uint16_t config = 0;
	int dr = 0;

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("unsupported channel reference type '%d'", channel_cfg->reference);
		return -ENOTSUP;
	}

	if (ads_config->multiplexer) {
		/* the device has an input multiplexer */
		if (channel_cfg->differential) {
			if (channel_cfg->input_positive == 0 && channel_cfg->input_negative == 1) {
				config |= ADS1X1X_CONFIG_MUX(ADS1X15_CONFIG_MUX_DIFF_0_1);
			} else if (channel_cfg->input_positive == 0 &&
				   channel_cfg->input_negative == 3) {
				config |= ADS1X1X_CONFIG_MUX(ADS1X15_CONFIG_MUX_DIFF_0_3);
			} else if (channel_cfg->input_positive == 1 &&
				   channel_cfg->input_negative == 3) {
				config |= ADS1X1X_CONFIG_MUX(ADS1X15_CONFIG_MUX_DIFF_1_3);
			} else if (channel_cfg->input_positive == 2 &&
				   channel_cfg->input_negative == 3) {
				config |= ADS1X1X_CONFIG_MUX(ADS1X15_CONFIG_MUX_DIFF_2_3);
			} else {
				LOG_ERR("unsupported input positive '%d' and input negative '%d'",
					channel_cfg->input_positive, channel_cfg->input_negative);
				return -ENOTSUP;
			}
		} else {
			if (channel_cfg->input_positive == 0) {
				config |= ADS1X1X_CONFIG_MUX(ADS1X15_CONFIG_MUX_SINGLE_0);
			} else if (channel_cfg->input_positive == 1) {
				config |= ADS1X1X_CONFIG_MUX(ADS1X15_CONFIG_MUX_SINGLE_1);
			} else if (channel_cfg->input_positive == 2) {
				config |= ADS1X1X_CONFIG_MUX(ADS1X15_CONFIG_MUX_SINGLE_2);
			} else if (channel_cfg->input_positive == 3) {
				config |= ADS1X1X_CONFIG_MUX(ADS1X15_CONFIG_MUX_SINGLE_3);
			} else {
				LOG_ERR("unsupported input positive '%d'",
					channel_cfg->input_positive);
				return -ENOTSUP;
			}
		}
	} else {
		/* only differential supported without multiplexer */
		if (!((channel_cfg->differential) &&
		      (channel_cfg->input_positive == 0 && channel_cfg->input_negative == 1))) {
			LOG_ERR("unsupported input positive '%d' and input negative '%d'",
				channel_cfg->input_positive, channel_cfg->input_negative);
			return -ENOTSUP;
		}
	}
	/* store differential mode to determine supported resolution */
	data->differential = channel_cfg->differential;

	dr = ads1x1x_acq_time_to_dr(dev, channel_cfg->acquisition_time);
	if (dr < 0) {
		LOG_ERR("unsupported channel acquisition time 0x%02x",
			channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	config |= ADS1X1X_CONFIG_DR(dr);

	if (ads_config->pga) {
		/* programmable gain amplifier support */
		switch (channel_cfg->gain) {
		case ADC_GAIN_1_3:
			config |= ADS1X1X_CONFIG_PGA(ADS1X1X_CONFIG_PGA_6144);
			break;
		case ADC_GAIN_1_2:
			config |= ADS1X1X_CONFIG_PGA(ADS1X1X_CONFIG_PGA_4096);
			break;
		case ADC_GAIN_1:
			config |= ADS1X1X_CONFIG_PGA(ADS1X1X_CONFIG_PGA_2048);
			break;
		case ADC_GAIN_2:
			config |= ADS1X1X_CONFIG_PGA(ADS1X1X_CONFIG_PGA_1024);
			break;
		case ADC_GAIN_4:
			config |= ADS1X1X_CONFIG_PGA(ADS1X1X_CONFIG_PGA_512);
			break;
		case ADC_GAIN_8:
			config |= ADS1X1X_CONFIG_PGA(ADS1X1X_CONFIG_PGA_256);
			break;
		default:
			LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
			return -ENOTSUP;
		}
	} else {
		/* no programmable gain amplifier, so only allow ADC_GAIN_1 */
		if (channel_cfg->gain != ADC_GAIN_1) {
			LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
			return -ENOTSUP;
		}
	}

	/* Only single shot supported */
	config |= ADS1X1X_CONFIG_MODE;

	/* disable comparator */
	config |= ADS1X1X_CONFIG_COMP_MODE;

	return ads1x1x_write_reg(dev, ADS1X1X_REG_CONFIG, config);
}

static int ads1x1x_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(int16_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int ads1x1x_validate_sequence(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct ads1x1x_config *config = dev->config;
	struct ads1x1x_data *data = dev->data;
	uint8_t resolution = data->differential ? config->resolution : config->resolution - 1;
	int err;

	if (sequence->resolution != resolution) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (sequence->channels != BIT(0)) {
		LOG_ERR("only channel 0 supported");
		return -ENOTSUP;
	}

	if (sequence->oversampling) {
		LOG_ERR("oversampling not supported");
		return -ENOTSUP;
	}

	err = ads1x1x_validate_buffer_size(sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return -ENOTSUP;
	}

	return 0;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads1x1x_data *data = CONTAINER_OF(ctx, struct ads1x1x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads1x1x_data *data = CONTAINER_OF(ctx, struct ads1x1x_data, ctx);

	data->repeat_buffer = data->buffer;

	ads1x1x_start_conversion(data->dev);

	k_sem_give(&data->acq_sem);
}

static int ads1x1x_adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int rc;
	struct ads1x1x_data *data = dev->data;

	rc = ads1x1x_validate_sequence(dev, sequence);
	if (rc != 0) {
		return rc;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int ads1x1x_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				  struct k_poll_signal *async)
{
	int rc;
	struct ads1x1x_data *data = dev->data;

	adc_context_lock(&data->ctx, async ? true : false, async);
	rc = ads1x1x_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, rc);

	return rc;
}

static int ads1x1x_adc_perform_read(const struct device *dev)
{
	int rc;
	struct ads1x1x_data *data = dev->data;
	const struct ads1x1x_config *config = dev->config;
	int16_t buf;

	rc = ads1x1x_read_reg(dev, ADS1X1X_REG_CONV, &buf);
	if (rc != 0) {
		adc_context_complete(&data->ctx, rc);
		return rc;
	}
	/* The ads101x stores it's 12b data in the upper part
	 * while the ads111x uses all 16b in the register, so
	 * shift down. Data is also signed, so perform
	 * division rather than shifting
	 */
	*data->buffer++ = buf / (1 << (16 - config->resolution));

	adc_context_on_sampling_done(&data->ctx, dev);

	return rc;
}

static int ads1x1x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return ads1x1x_adc_read_async(dev, sequence, NULL);
}

static void ads1x1x_acquisition_thread(const struct device *dev)
{
	struct ads1x1x_data *data = dev->data;
	int rc;

	while (true) {
		k_sem_take(&data->acq_sem, K_FOREVER);

		rc = ads1x1x_wait_data_ready(dev);
		if (rc != 0) {
			LOG_ERR("failed to get ready status (err %d)", rc);
			adc_context_complete(&data->ctx, rc);
			break;
		}

		ads1x1x_adc_perform_read(dev);
	}
}

static int ads1x1x_init(const struct device *dev)
{
	const struct ads1x1x_config *config = dev->config;
	struct ads1x1x_data *data = dev->data;

	data->dev = dev;

	k_sem_init(&data->acq_sem, 0, 1);

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	const k_tid_t tid =
		k_thread_create(&data->thread, data->stack, K_THREAD_STACK_SIZEOF(data->stack),
				(k_thread_entry_t)ads1x1x_acquisition_thread, (void *)dev, NULL,
				NULL, CONFIG_ADC_ADS1X1X_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_ads1x1x");

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api ads1x1x_api = {
	.channel_setup = ads1x1x_channel_setup,
	.read = ads1x1x_read,
	.ref_internal = 2048,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ads1x1x_adc_read_async,
#endif
};

#define DT_INST_ADS1X1X(inst, t) DT_INST(inst, ti_ads##t)

#define ADS1X1X_INIT(t, n, odr_delay_us, res, mux, pgab)                                           \
	static const struct ads1x1x_config ads##t##_config_##n = {                                 \
		.bus = I2C_DT_SPEC_GET(DT_INST_ADS1X1X(n, t)),                                     \
		.odr_delay = odr_delay_us,                                                         \
		.resolution = res,                                                                 \
		.multiplexer = mux,                                                                \
		.pga = pgab,                                                                       \
	};                                                                                         \
	static struct ads1x1x_data ads##t##_data_##n = {                                           \
		ADC_CONTEXT_INIT_LOCK(ads##t##_data_##n, ctx),                                     \
		ADC_CONTEXT_INIT_TIMER(ads##t##_data_##n, ctx),                                    \
		ADC_CONTEXT_INIT_SYNC(ads##t##_data_##n, ctx),                                     \
	};                                                                                         \
	DEVICE_DT_DEFINE(DT_INST_ADS1X1X(n, t), ads1x1x_init, NULL, &ads##t##_data_##n,            \
			 &ads##t##_config_##n, POST_KERNEL, CONFIG_ADC_ADS1X1X_INIT_PRIORITY,      \
			 &ads1x1x_api);

/* The ADS111X provides 16 bits of data in binary two's complement format
 * A positive full-scale (+FS) input produces an output code of 7FFFh and a
 * negative full-scale (–FS) input produces an output code of 8000h. Single
 * ended signal measurements only only use the positive code range from
 * 0000h to 7FFFh
 */
#define ADS111X_RESOLUTION 16

/*
 * Approximated ADS111x acquisition times in microseconds. These are
 * used for the initial delay when polling for data ready.
 * {8 SPS, 16 SPS, 32 SPS, 64 SPS, 128 SPS (default), 250 SPS, 475 SPS, 860 SPS}
 */
#define ADS111X_ODR_DELAY_US                                                                       \
	{                                                                                          \
		125000, 62500, 31250, 15625, 7813, 4000, 2105, 1163                                \
	}

/*
 * ADS1115: 16 bit, multiplexer, programmable gain amplifier
 */
#define ADS1115_INIT(n) ADS1X1X_INIT(1115, n, ADS111X_ODR_DELAY_US, ADS111X_RESOLUTION, true, true)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ads1115
DT_INST_FOREACH_STATUS_OKAY(ADS1115_INIT)

/*
 * ADS1114: 16 bit, no multiplexer, programmable gain amplifier
 */
#define ADS1114_INIT(n) ADS1X1X_INIT(1114, n, ADS111X_ODR_DELAY_US, ADS111X_RESOLUTION, false, true)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ads1114
DT_INST_FOREACH_STATUS_OKAY(ADS1114_INIT)

/*
 * ADS1113: 16 bit, no multiplexer, no programmable gain amplifier
 */
#define ADS1113_INIT(n)                                                                            \
	ADS1X1X_INIT(1113, n, ADS111X_ODR_DELAY_US, ADS111X_RESOLUTION, false, false)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ads1113
DT_INST_FOREACH_STATUS_OKAY(ADS1113_INIT)

/* The ADS101X provides 12 bits of data in binary two's complement format
 * A positive full-scale (+FS) input produces an output code of 7FFh and a
 * negative full-scale (–FS) input produces an output code of 800h. Single
 * ended signal measurements only only use the positive code range from
 * 000h to 7FFh
 */
#define ADS101X_RESOLUTION 12

/*
 * Approximated ADS101x acquisition times in microseconds. These are
 * used for the initial delay when polling for data ready.
 * {128 SPS, 250 SPS, 490 SPS, 920 SPS, 1600 SPS (default), 2400 SPS, 3300 SPS, 3300 SPS}
 */
#define ADS101X_ODR_DELAY_US                                                                       \
	{                                                                                          \
		7813, 4000, 2041, 1087, 625, 417, 303, 303                                         \
	}

/*
 * ADS1015: 12 bit, multiplexer, programmable gain amplifier
 */
#define ADS1015_INIT(n) ADS1X1X_INIT(1015, n, ADS101X_ODR_DELAY_US, ADS101X_RESOLUTION, true, true)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ads1015
DT_INST_FOREACH_STATUS_OKAY(ADS1015_INIT)

/*
 * ADS1014: 12 bit, no multiplexer, programmable gain amplifier
 */
#define ADS1014_INIT(n) ADS1X1X_INIT(1014, n, ADS101X_ODR_DELAY_US, ADS101X_RESOLUTION, false, true)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ads1014
DT_INST_FOREACH_STATUS_OKAY(ADS1014_INIT)

/*
 * ADS1013: 12 bit, no multiplexer, no programmable gain amplifier
 */
#define ADS1013_INIT(n)                                                                            \
	ADS1X1X_INIT(1013, n, ADS101X_ODR_DELAY_US, ADS101X_RESOLUTION, false, false)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ads1013
DT_INST_FOREACH_STATUS_OKAY(ADS1013_INIT)
