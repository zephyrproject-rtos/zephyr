/*
 * Copyright (c) 2026 Vaisala Oyj
 *
 * Based on adc_ads1119.c, which are:
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright (c) 2020 Innoseis BV
 *
 * Based on adc_ads1x4s0x.c, which is:
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#include "adc_context.h"

#define DT_DRV_COMPAT ti_ads1220

LOG_MODULE_REGISTER(ADS1220, CONFIG_ADC_LOG_LEVEL);

#define ADS1220_CONFIG_VREF(x)  		(FIELD_PREP(BIT_MASK(2) << 6, x))
#define ADS1220_CONFIG_FIR(x)			(FIELD_PREP(BIT_MASK(2) << 4, x))
#define ADS1220_CONFIG_MUX(x)   		(FIELD_PREP(BIT_MASK(4) << 4, x))
#define ADS1220_CONFIG_CM(x)    		(FIELD_PREP(BIT(2), x))
#define ADS1220_CONFIG_DR(x)    		(FIELD_PREP(BIT_MASK(2) << 2, x))
#define ADS1220_CONFIG_GAIN(x)  		(FIELD_PREP(BIT_MASK(3) << 1, x))
#define ADS1220_CONFIG_IDAC1(x) 		(FIELD_PREP(BIT_MASK(3) << 5, x))
#define ADS1220_CONFIG_IDAC2(x)			(FIELD_PREP(BIT_MASK(3) << 2, x))
#define ADS1220_CONFIG_IDAC_CURRENT(x)	(FIELD_PREP(BIT_MASK(3), x))

#define ADS1220_REG_SHIFT 2

#define ADS1220_RESOLUTION   24
#define ADS1220_REF_INTERNAL 2048

// wait at least 50us + 32 * tCLK (256kHz)
#define ADS1220_RESET_DELAY 5

enum ads1220_cmd {
	ADS1220_CMD_RESET = 0x06,
	ADS1220_CMD_START_SYNC = 0x08,
	ADS1220_CMD_POWER_DOWN = 0x02,
	ADS1220_CMD_READ_DATA = 0x10,
	ADS1220_CMD_READ_REG = 0x20,
	ADS1220_CMD_WRITE_REG = 0x40,
};

enum ads1220_reg {
	ADS1220_REG_CONFIG_0 = 0,
	ADS1220_REG_CONFIG_1 = 1,
	ADS1220_REG_CONFIG_2 = 2,
	ADS1220_REG_CONFIG_3 = 3,
	ADS1220_REG_COUNT = 4,
};

enum {
	ADS1220_CONFIG_VREF_INTERNAL = 0,
	ADS1220_CONFIG_VREF_EXTERNAL_0 = 1,
	ADS1220_CONFIG_VREF_EXTERNAL_1 = 2,
	ADS1220_CONFIG_VREF_AVDD = 3,
};

enum {
	ADS1220_CONFIG_MUX_DIFF_0_1 = 0,
	ADS1220_CONFIG_MUX_DIFF_0_2 = 1,
	ADS1220_CONFIG_MUX_DIFF_0_3 = 2,
	ADS1220_CONFIG_MUX_DIFF_1_2 = 3,
	ADS1220_CONFIG_MUX_DIFF_1_3 = 4,
	ADS1220_CONFIG_MUX_DIFF_2_3 = 5,
	ADS1220_CONFIG_MUX_DIFF_1_0 = 6,
	ADS1220_CONFIG_MUX_DIFF_3_2 = 7,
	ADS1220_CONFIG_MUX_SINGLE_0 = 8,
	ADS1220_CONFIG_MUX_SINGLE_1 = 9,
	ADS1220_CONFIG_MUX_SINGLE_2 = 10,
	ADS1220_CONFIG_MUX_SINGLE_3 = 11,
	ADS1220_CONFIG_MUX_VREF_MONITOR = 12,
	ADS1220_CONFIG_MUX_VDD_MONITOR = 13,
	ADS1220_CONFIG_MUX_SHORTED = 14,
};

enum {
	ADS1220_CONFIG_DR_20 = 0,
	ADS1220_CONFIG_DR_45 = 1,
	ADS1220_CONFIG_DR_90 = 2,
	ADS1220_CONFIG_DR_175 = 3,
	ADS1220_CONFIG_DR_330 = 4,
	ADS1220_CONFIG_DR_600 = 5,
	ADS1220_CONFIG_DR_1000 = 6,
	ADS1220_CONFIG_DR_DEFAULT = ADS1220_CONFIG_DR_20,
};

enum {
	ADS1220_CONFIG_GAIN_1 = 0,
	ADS1220_CONFIG_GAIN_2 = 1,
	ADS1220_CONFIG_GAIN_4 = 2,
	ADS1220_CONFIG_GAIN_8 = 3,
	ADS1220_CONFIG_GAIN_16 = 4,
	ADS1220_CONFIG_GAIN_32 = 5,
	ADS1220_CONFIG_GAIN_64 = 6,
	ADS1220_CONFIG_GAIN_128 = 7,
};

enum {
	ADS1220_CONFIG_CM_SINGLE = 0,
	ADS1220_CONFIG_CM_CONTINUOUS = 1,
};

enum {
	ADS1220_CONFIG_PGA_ENABLED = 0,
	ADS1220_CONFIG_PGA_DISABLE = 1,
};

enum {
	ADS1220_CONFIG_BURN_OUT_CURRENT_SOURCES_OFF = 0,
	ADS1220_CONFIG_BURN_OUT_CURRENT_SOURCES_ON = 1,
};

enum {
	ADS1220_CONFIG_TEMPERATURE_SENSOR_OFF = 0,
	ADS1220_CONFIG_TEMPERATURE_SENSOR_ON = 1,
};

enum {
	ADS1220_CONFIG_OPERATING_MODE_NORMAL = 0,
	ADS1220_CONFIG_OPERATING_MODE_DUTY_CYCLE = 1,
	ADS1220_CONFIG_OPERATING_MODE_TURBO = 2,
};

enum {
	ADS1220_CONFIG_IDAC_OFF = 0,
	ADS1220_CONFIG_IDAC_10_UA = 1,
	ADS1220_CONFIG_IDAC_50_UA = 2,
	ADS1220_CONFIG_IDAC_100_UA = 3,
	ADS1220_CONFIG_IDAC_250_UA = 4,
	ADS1220_CONFIG_IDAC_500_UA = 5,
	ADS1220_CONFIG_IDAC_1000_UA = 6,
	ADS1220_CONFIG_IDAC_1500_UA = 7,
};

enum {
	ADS1220_CONFIG_POWER_SWITCH_OPEN = 0,
	ADS1220_CONFIG_POWER_SWITCH_AUTO = 1,
};

enum {
	ADS1220_CONFIG_FIR_DISABLED = 0,
	ADS1220_CONFIG_FIR_50_60_REJECT = 1,
	ADS1220_CONFIG_FIR_50_REJECT = 2,
	ADS1220_CONFIG_FIR_60_REJECT = 3,
};

enum {
	ADS1220_CONFIG_DRDY_ONLY = 0,
	ADS1220_CONFIG_DRDY_DOUT = 1,
};

enum {
	ADS1220_CONFIG_IDAC1_DISABLED = 0,
	ADS1220_CONFIG_IDAC1_AIN0 = 1,
	ADS1220_CONFIG_IDAC1_AIN1 = 2,
	ADS1220_CONFIG_IDAC1_AIN2 = 3,
	ADS1220_CONFIG_IDAC1_AIN3 = 4,
	ADS1220_CONFIG_IDAC1_REFP0 = 5,
	ADS1220_CONFIG_IDAC1_REFNO = 6,
	ADS1220_CONFIG_IDAC1_COUNT = 7,
};

enum {
	ADS1220_CONFIG_IDAC2_DISABLED = 0,
	ADS1220_CONFIG_IDAC2_AIN0 = 1,
	ADS1220_CONFIG_IDAC2_AIN1 = 2,
	ADS1220_CONFIG_IDAC2_AIN2 = 3,
	ADS1220_CONFIG_IDAC2_AIN3 = 4,
	ADS1220_CONFIG_IDAC2_REFP0 = 5,
	ADS1220_CONFIG_IDAC2_REFNO = 6,
	ADS1220_CONFIG_IDAC2_COUNT = 7,
};

struct ads1220_config {
	const struct spi_dt_spec spi;
	const struct gpio_dt_spec gpio_drdy;
	int idac_current;
#if CONFIG_ADC_ASYNC
	k_thread_stack_t *stack;
#endif
};

struct ads1220_data {
	struct adc_context ctx;
	struct k_sem acq_sem;
	struct k_sem drdy_sem;
	struct gpio_callback callback_drdy;
	int32_t *buffer;
	int32_t *buffer_ptr;
	bool differential;
#if CONFIG_ADC_ASYNC
	struct k_thread thread;
#endif
};

static int ads1220_write_regs(const struct device *dev, enum ads1220_reg start_addr,
			      const uint8_t *reg_vals, uint8_t num_regs)
{
	const struct ads1220_config *cfg = dev->config;
	uint8_t data[ADS1220_REG_COUNT + 1] = { 0 };

	if (start_addr >= ADS1220_REG_COUNT || start_addr + num_regs > ADS1220_REG_COUNT) {
		return -EINVAL;
	}

	data[0] = ADS1220_CMD_WRITE_REG | (start_addr << ADS1220_REG_SHIFT) | (num_regs - 1);
	memcpy(&data[1], reg_vals, num_regs);

	struct spi_buf tx_buf = {
		.buf = data,
		.len = num_regs + 1,
	};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(&cfg->spi, &tx);
}

static inline int ads1220_acq_time_to_dr(const struct device *dev, uint16_t acq_time)
{
	int odr = -EINVAL;
	uint16_t acq_value = ADC_ACQ_TIME_VALUE(acq_time);

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		return ADS1220_CONFIG_DR_DEFAULT;
	} else if (ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) {
		return -EINVAL;
	}

	switch (acq_value) {
	case ADS1220_CONFIG_DR_20:
		odr = ADS1220_CONFIG_DR_20;
		break;
	case ADS1220_CONFIG_DR_45:
		odr = ADS1220_CONFIG_DR_45;
		break;
	case ADS1220_CONFIG_DR_90:
		odr = ADS1220_CONFIG_DR_90;
		break;
	case ADS1220_CONFIG_DR_175:
		odr = ADS1220_CONFIG_DR_175;
		break;
	case ADS1220_CONFIG_DR_330:
		odr = ADS1220_CONFIG_DR_330;
		break;
	case ADS1220_CONFIG_DR_600:
		odr = ADS1220_CONFIG_DR_600;
		break;
	case ADS1220_CONFIG_DR_1000:
		odr = ADS1220_CONFIG_DR_1000;
		break;
	default:
		return -EINVAL;
	}

	return odr;
}

static int ads1220_send_start_read(const struct device *dev)
{
	const struct ads1220_config *cfg = dev->config;
	uint8_t cmd = ADS1220_CMD_START_SYNC;
	struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = 1,
	};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(&cfg->spi, &tx);
}

static int ads1220_wait_data_ready(const struct device *dev)
{
	struct ads1220_data *data = dev->data;

	return k_sem_take(&data->drdy_sem, ADC_CONTEXT_WAIT_FOR_COMPLETION_TIMEOUT);
}

static int ads1220_read_sample(const struct device *dev, int32_t *buff)
{
	int res;
	uint8_t rx_bytes[3];
	const struct ads1220_config *cfg = dev->config;
	struct spi_buf rx_buf = {
		.buf = &rx_bytes[0],
		.len = sizeof(rx_bytes),
	};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	res = spi_read_dt(&cfg->spi, &rx);

	if (res == 0) {
		*buff = sys_get_be24(&rx_bytes[0]);

		if (*buff & 0x800000) {
			*buff |= 0xFF000000;
		}
	}

	return res;
}

static int ads1220_differential_channel_setup(const struct adc_channel_cfg *channel_cfg,
					      uint8_t *config)
{
	if (channel_cfg->input_positive == 0) {
		if (channel_cfg->input_negative == 1) {
			*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_DIFF_0_1);
		} else if (channel_cfg->input_negative == 2) {
			*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_DIFF_0_2);
		} else if (channel_cfg->input_negative == 3) {
			*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_DIFF_0_3);
		} else {
			return -EINVAL;
		}
	} else if (channel_cfg->input_positive == 1) {
		if (channel_cfg->input_negative == 2) {
			*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_DIFF_1_2);
		} else if (channel_cfg->input_negative == 3) {
			*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_DIFF_1_3);
		} else if (channel_cfg->input_negative == 0) {
			*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_DIFF_1_0);
		} else {
			return -EINVAL;
		}
	} else if (channel_cfg->input_positive == 2) {
		if (channel_cfg->input_negative == 3) {
			*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_DIFF_2_3);
		} else {
			return -EINVAL;
		}
	} else if (channel_cfg->input_positive == 3) {
		if (channel_cfg->input_negative == 2) {
			*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_DIFF_3_2);
		} else {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int ads1220_single_channel_setup(const struct adc_channel_cfg *channel_cfg, uint8_t *config)
{
	if (channel_cfg->input_positive == 0) {
		*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_SINGLE_0);
	} else if (channel_cfg->input_positive == 1) {
		*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_SINGLE_1);
	} else if (channel_cfg->input_positive == 2) {
		*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_SINGLE_2);
	} else if (channel_cfg->input_positive == 3) {
		*config |= ADS1220_CONFIG_MUX(ADS1220_CONFIG_MUX_SINGLE_3);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int ads1220_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct ads1220_config *config = dev->config;
	struct ads1220_data *data = dev->data;
	uint8_t conf_bytes[4] = {0};
	int dr = 0;

	if (channel_cfg->channel_id != 0) {
		return -EINVAL;
	}

	conf_bytes[0] |= ADS1220_CONFIG_PGA_DISABLE;
	conf_bytes[1] |= ADS1220_CONFIG_CM(ADS1220_CONFIG_CM_CONTINUOUS);
	conf_bytes[2] |= ADS1220_CONFIG_FIR(ADS1220_CONFIG_FIR_50_60_REJECT);

	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
		conf_bytes[2] |= ADS1220_CONFIG_VREF(ADS1220_CONFIG_VREF_AVDD);
		break;
	case ADC_REF_EXTERNAL0:
		conf_bytes[2] |= ADS1220_CONFIG_VREF(ADS1220_CONFIG_VREF_EXTERNAL_0);
		break;
	case ADC_REF_EXTERNAL1:
		conf_bytes[2] |= ADS1220_CONFIG_VREF(ADS1220_CONFIG_VREF_EXTERNAL_1);
		break;
	case ADC_REF_INTERNAL:
		conf_bytes[2] |= ADS1220_CONFIG_VREF(ADS1220_CONFIG_VREF_INTERNAL);
		break;
	default:
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		if (ads1220_differential_channel_setup(channel_cfg, &conf_bytes[0]) != 0) {
			return -EINVAL;
		}
	} else {
		if (ads1220_single_channel_setup(channel_cfg, &conf_bytes[0]) != 0) {
			return -EINVAL;
		}
	}

	data->differential = channel_cfg->differential;

	dr = ads1220_acq_time_to_dr(dev, channel_cfg->acquisition_time);
	if (dr < 0) {
		return dr;
	}

	conf_bytes[1] |= ADS1220_CONFIG_DR(dr);

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		conf_bytes[0] |= ADS1220_CONFIG_GAIN(ADS1220_CONFIG_GAIN_1);
		break;
	case ADC_GAIN_2:
		conf_bytes[0] |= ADS1220_CONFIG_GAIN(ADS1220_CONFIG_GAIN_2);
		break;
	case ADC_GAIN_4:
		conf_bytes[0] |= ADS1220_CONFIG_GAIN(ADS1220_CONFIG_GAIN_4);
		break;
	case ADC_GAIN_8:
		conf_bytes[0] |= ADS1220_CONFIG_GAIN(ADS1220_CONFIG_GAIN_8);
		break;
	case ADC_GAIN_16:
		conf_bytes[0] |= ADS1220_CONFIG_GAIN(ADS1220_CONFIG_GAIN_16);
		break;
	case ADC_GAIN_32:
		conf_bytes[0] |= ADS1220_CONFIG_GAIN(ADS1220_CONFIG_GAIN_32);
		break;
	case ADC_GAIN_64:
		conf_bytes[0] |= ADS1220_CONFIG_GAIN(ADS1220_CONFIG_GAIN_64);
		break;
	case ADC_GAIN_128:
		conf_bytes[0] |= ADS1220_CONFIG_GAIN(ADS1220_CONFIG_GAIN_128);
		break;
	default:
		return -EINVAL;
	}

	switch (config->idac_current) {
	case 0:
		conf_bytes[2] |= ADS1220_CONFIG_IDAC_CURRENT(ADS1220_CONFIG_IDAC_OFF);
		break;
	case 10:
		conf_bytes[2] |= ADS1220_CONFIG_IDAC_CURRENT(ADS1220_CONFIG_IDAC_10_UA);
		break;
	case 50:
		conf_bytes[2] |= ADS1220_CONFIG_IDAC_CURRENT(ADS1220_CONFIG_IDAC_50_UA);
		break;
	case 100:
		conf_bytes[2] |= ADS1220_CONFIG_IDAC_CURRENT(ADS1220_CONFIG_IDAC_100_UA);
		break;
	case 250:
		conf_bytes[2] |= ADS1220_CONFIG_IDAC_CURRENT(ADS1220_CONFIG_IDAC_250_UA);
		break;
	case 500:
		conf_bytes[2] |= ADS1220_CONFIG_IDAC_CURRENT(ADS1220_CONFIG_IDAC_500_UA);
		break;
	case 1000:
		conf_bytes[2] |= ADS1220_CONFIG_IDAC_CURRENT(ADS1220_CONFIG_IDAC_1000_UA);
		break;
	case 1500:
		conf_bytes[2] |= ADS1220_CONFIG_IDAC_CURRENT(ADS1220_CONFIG_IDAC_1500_UA);
		break;
	default:
		LOG_ERR("%s: IDAC current %i not supported", dev->name, config->idac_current);
		return -EINVAL;
	}

	if (channel_cfg->current_source_pin_set) {
		if (channel_cfg->current_source_pin[0] > ADS1220_CONFIG_IDAC1_COUNT) {
			return -EINVAL;
		} else if (channel_cfg->current_source_pin[1] > ADS1220_CONFIG_IDAC2_COUNT) {
			return -EINVAL;
		}

		conf_bytes[3] |= ADS1220_CONFIG_IDAC1(channel_cfg->current_source_pin[0]);
		conf_bytes[3] |= ADS1220_CONFIG_IDAC2(channel_cfg->current_source_pin[1]);
	}

	return ads1220_write_regs(dev, ADS1220_REG_CONFIG_0, &conf_bytes[0], sizeof(conf_bytes));
}

static int ads1220_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(int32_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int ads1220_validate_sequence(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct ads1220_data *data = dev->data;
	const uint8_t resolution = data->differential ? ADS1220_RESOLUTION : ADS1220_RESOLUTION - 1;

	if (sequence->resolution != resolution) {
		return -EINVAL;
	}

	if (sequence->channels != BIT(0)) {
		return -EINVAL;
	}

	if (sequence->oversampling) {
		return -EINVAL;
	}

	return ads1220_validate_buffer_size(sequence);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads1220_data *data = CONTAINER_OF(ctx, struct ads1220_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_ptr;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads1220_data *data = CONTAINER_OF(ctx, struct ads1220_data, ctx);

	data->buffer_ptr = data->buffer;
	k_sem_give(&data->acq_sem);
}

static int ads1220_adc_start_read(const struct device *dev, const struct adc_sequence *sequence,
				  bool wait)
{
	int rc;
	struct ads1220_data *data = dev->data;

	rc = ads1220_validate_sequence(dev, sequence);
	if (rc != 0) {
		return rc;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	if (wait) {
		rc = adc_context_wait_for_completion(&data->ctx);
	}
	return rc;
}

static int ads1220_adc_perform_read(const struct device *dev)
{
	int rc;
	struct ads1220_data *data = dev->data;

	k_sem_take(&data->acq_sem, K_FOREVER);
	k_sem_reset(&data->drdy_sem);

	rc = ads1220_send_start_read(dev);
	if (rc) {
		adc_context_complete(&data->ctx, rc);
		return rc;
	}

	rc = ads1220_wait_data_ready(dev);
	if (rc != 0) {
		adc_context_complete(&data->ctx, rc);
		return rc;
	}

	rc = ads1220_read_sample(dev, data->buffer);
	if (rc != 0) {
		adc_context_complete(&data->ctx, rc);
		return rc;
	}
	data->buffer++;

	adc_context_on_sampling_done(&data->ctx, dev);

	return rc;
}

#if CONFIG_ADC_ASYNC
static int ads1220_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				  struct k_poll_signal *async)
{
	int rc;
	struct ads1220_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	rc = ads1220_adc_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, rc);

	return rc;
}

static int ads1220_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int rc;
	struct ads1220_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	rc = ads1220_adc_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, rc);

	return rc;
}

static void ads1220_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;

	while (true) {
		ads1220_adc_perform_read(dev);
	}
}
#else
static int ads1220_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int rc;
	struct ads1220_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	rc = ads1220_adc_start_read(dev, sequence, false);

	while (rc == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		rc = ads1220_adc_perform_read(dev);
	}

	adc_context_release(&data->ctx, rc);
	return rc;
}
#endif

static void ads1220_data_ready_handler(const struct device *dev, struct gpio_callback *gpio_cb,
				       uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct ads1220_data *data = CONTAINER_OF(gpio_cb, struct ads1220_data, callback_drdy);

	k_sem_give(&data->drdy_sem);
}

static int ads1220_configure_gpio(const struct device *dev)
{
	int ret;
	const struct ads1220_config *cfg = dev->config;
	struct ads1220_data *data = dev->data;

	ret = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&data->callback_drdy, ads1220_data_ready_handler,
			   BIT(cfg->gpio_drdy.pin));

	return gpio_add_callback(cfg->gpio_drdy.port, &data->callback_drdy);
}

static int ads1220_device_reset(const struct device *dev)
{
	const struct ads1220_config *cfg = dev->config;
	uint8_t cmd = ADS1220_CMD_RESET;

	struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = 1,
	};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	int res = spi_write_dt(&cfg->spi, &tx);

	k_msleep(ADS1220_RESET_DELAY);

	return res;
}

#if defined CONFIG_PM_DEVICE
static int ads1220_pm(const struct device *dev, uint16_t cmd)
{
	const struct ads1220_config *cfg = dev->config;
	struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = sizeof(cmd),
	};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(&cfg->spi, &tx);
}

static int ads1220_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return ads1220_pm(dev, ADS1220_CMD_START_SYNC);
	case PM_DEVICE_ACTION_SUSPEND:
		return ads1220_pm(dev, ADS1220_CMD_POWER_DOWN);
	default:
		return -EINVAL;
	}
}
#endif /* CONFIG_PM_DEVICE */

static int ads1220_init(const struct device *dev)
{
	int rc;
	const struct ads1220_config *config = dev->config;
	struct ads1220_data *data = dev->data;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("ADS1220 is not ready");
		return -ENODEV;
	}

	adc_context_init(&data->ctx);

	k_sem_init(&data->acq_sem, 0, 1);
	k_sem_init(&data->drdy_sem, 0, 1);

	rc = ads1220_configure_gpio(dev);
	if (rc != 0) {
		LOG_ERR("GPIO config failed %d", rc);
		return rc;
	}

	rc = ads1220_device_reset(dev);
	if (rc != 0) {
		LOG_WRN("Device is not reset");
	}

#if CONFIG_ADC_ASYNC
	k_tid_t tid = k_thread_create(&data->thread, config->stack,
				      CONFIG_ADC_ADS1220_ACQUISITION_THREAD_STACK_SIZE,
				      ads1220_acquisition_thread, (void *)dev, NULL, NULL,
				      CONFIG_ADC_ADS1220_ASYNC_THREAD_INIT_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_ads1220");
#endif
	adc_context_unlock_unconditionally(&data->ctx);

#if defined CONFIG_PM_DEVICE
	ret = ads1220_pm(dev, ADS1220_CMD_POWER_DOWN);
	if (ret != 0) {
		return ret;
	}

	pm_device_init_suspended(dev);
#endif /* CONFIG_PM_DEVICE */

	return rc;
}

static DEVICE_API(adc, api) = {
	.channel_setup = ads1220_channel_setup,
	.read = ads1220_read,
	.ref_internal = ADS1220_REF_INTERNAL,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ads1220_adc_read_async,
#endif
};

#define ADC_ADS1220_SPI_CFG SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define ADC_ADS1220_INST_DEFINE(n)                                                                 \
	PM_DEVICE_DT_INST_DEFINE(n, ads1220_pm_action);                                            \
	IF_ENABLED(										   \
		CONFIG_ADC_ASYNC,								   \
		(static K_KERNEL_STACK_DEFINE(							   \
			 thread_stack_##n,							   \
			 CONFIG_ADC_ADS1220_ACQUISITION_THREAD_STACK_SIZE);)			   \
	)                                                                  \
	static const struct ads1220_config config_##n = {                                          \
		.spi = SPI_DT_SPEC_INST_GET(n, ADC_ADS1220_SPI_CFG),                               \
		.gpio_drdy = GPIO_DT_SPEC_INST_GET(n, drdy_gpios),				   \
		.idac_current = DT_INST_PROP(n, idac_current),					   \
		IF_ENABLED(CONFIG_ADC_ASYNC,							   \
			(.stack = thread_stack_##n)) };                          \
	static struct ads1220_data data_##n;                                                       \
	DEVICE_DT_INST_DEFINE(n, ads1220_init, PM_DEVICE_DT_INST_GET(n), &data_##n, &config_##n,   \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(ADC_ADS1220_INST_DEFINE);
