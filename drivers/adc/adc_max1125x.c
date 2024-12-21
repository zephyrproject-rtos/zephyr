/*
 * Copyright (c) 2023 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(ADC_MAX1125X, CONFIG_ADC_LOG_LEVEL);

#define MAX1125X_CONFIG_PGA(x)     BIT(x)
#define MAX1125X_CONFIG_CHANNEL(x) ((x) << 5)
#define MAX1125X_CONFIG_CHMAP(x)   ((x << 2) | BIT(1))
#define MAX1125X_REG_DATA(x)       (MAX1125X_REG_DATA0 + (x << 1))

#define MAX1125X_CMD_READ        0xC1
#define MAX1125X_CMD_WRITE       0xC0
#define MAX1125X_CMD_CONV        0x80
#define MAX1125X_CMD_CALIBRATION 0x20
#define MAX1125X_CMD_SEQUENCER   0x30

enum max1125x_mode {
	MAX1125X_MODE_POWERDOWN = 0x01,
	MAX1125X_MODE_CALIBRATION = 0x02,
	MAX1125X_MODE_SEQUENCER = 0x03,
};

enum {
	MAX1125X_CONFIG_RATE_1_9 = 0x00,
	MAX1125X_CONFIG_RATE_3_9 = 0x01,
	MAX1125X_CONFIG_RATE_7_8 = 0x02,
	MAX1125X_CONFIG_RATE_15_6 = 0x03,
	MAX1125X_CONFIG_RATE_31_2 = 0x04,
	MAX1125X_CONFIG_RATE_62_5 = 0x05,
	MAX1125X_CONFIG_RATE_125 = 0x06,
	MAX1125X_CONFIG_RATE_250 = 0x07,
	MAX1125X_CONFIG_RATE_500 = 0x08,
	MAX1125X_CONFIG_RATE_1000 = 0x09,
	MAX1125X_CONFIG_RATE_2000 = 0x0A,
	MAX1125X_CONFIG_RATE_4000 = 0x0B,
	MAX1125X_CONFIG_RATE_8000 = 0x0C,
	MAX1125X_CONFIG_RATE_16000 = 0x0D,
	MAX1125X_CONFIG_RATE_32000 = 0x0E,
	MAX1125X_CONFIG_RATE_64000 = 0x0F,
};

enum max1125x_reg {
	MAX1125X_REG_STAT = 0x00,
	MAX1125X_REG_CTRL1 = 0x02,
	MAX1125X_REG_CTRL2 = 0x04,
	MAX1125X_REG_CTRL3 = 0x06,
	MAX1125X_REG_GPIO_CTRL = 0x08,
	MAX1125X_REG_DELAY = 0x0A,
	MAX1125X_REG_CHMAP1 = 0x0C,
	MAX1125X_REG_CHMAP0 = 0x0E,
	MAX1125X_REG_SEQ = 0x10,
	MAX1125X_REG_GPO_DIR = 0x12,
	MAX1125X_REG_SOC = 0x14,
	MAX1125X_REG_SGC = 0x16,
	MAX1125X_REG_SCOC = 0x18,
	MAX1125X_REG_SCGC = 0x1A,
	MAX1125X_REG_DATA0 = 0x1C,
	MAX1125X_REG_DATA1 = 0x1E,
	MAX1125X_REG_DATA2 = 0x20,
	MAX1125X_REG_DATA3 = 0x22,
	MAX1125X_REG_DATA4 = 0x24,
	MAX1125X_REG_DATA5 = 0x26,
};

enum {
	MAX1125X_REG_STAT_LEN = 3,
	MAX1125X_REG_CTRL1_LEN = 1,
	MAX1125X_REG_CTRL2_LEN = 1,
	MAX1125X_REG_CTRL3_LEN = 1,
	MAX1125X_REG_GPIO_CTRL_LEN = 1,
	MAX1125X_REG_DELAY_LEN = 2,
	MAX1125X_REG_CHMAP1_LEN = 3,
	MAX1125X_REG_CHMAP0_LEN = 3,
	MAX1125X_REG_SEQ_LEN = 1,
	MAX1125X_REG_GPO_DIR_LEN = 1,
	MAX1125X_REG_SOC_LEN = 3,
	MAX1125X_REG_SGC_LEN = 3,
	MAX1125X_REG_SCOC_LEN = 3,
	MAX1125X_REG_SCGC_LEN = 3,
};

enum {
	MAX1125X_CTRL1_CAL_SELF = 0,
	MAX1125X_CTRL1_CAL_OFFSET = 1,
	MAX1125X_CTRL1_CAL_FULLSCALE = 2,
};

enum {
	MAX1125X_CTRL1_PD_NOP = 0,
	MAX1125X_CTRL1_DP_SLEEP = 1,
	MAX1125X_CTRL1_DP_STANDBY = 2,
	MAX1125X_CTRL1_DP_RESET = 3,
};

enum {
	MAX1125X_CTRL1_CONTSC = 0,
	MAX1125X_CTRL1_SCYCLE = 1,
	MAX1125X_CTRL1_FORMAT = 2,
	MAX1125X_CTRL1_UBPOLAR = 3,
};

enum {
	MAX1125X_CTRL2_PGA_GAIN_1 = 0,
	MAX1125X_CTRL2_PGA_GAIN_2 = 1,
	MAX1125X_CTRL2_PGA_GAIN_4 = 2,
	MAX1125X_CTRL2_PGA_GAIN_8 = 3,
	MAX1125X_CTRL2_PGA_GAIN_16 = 4,
	MAX1125X_CTRL2_PGA_GAIN_32 = 5,
	MAX1125X_CTRL2_PGA_GAIN_64 = 6,
	MAX1125X_CTRL2_PGA_GAIN_128 = 7,
};

enum {
	MAX1125X_CTRL2_PGAEN = 3,
	MAX1125X_CTRL2_LPMODE = 4,
	MAX1125X_CTRL2_LDOEN = 5,
	MAX1125X_CTRL2_CSSEN = 6,
	MAX1125X_CTRL2_EXTCLK = 7,
};

enum {
	MAX1125X_CTRL3_NOSCO = 0,
	MAX1125X_CTRL3_NOSCG = 1,
	MAX1125X_CTRL3_NOSYSO = 2,
	MAX1125X_CTRL3_NOSYSG = 3,
	MAX1125X_CTRL3_CALREGSEL = 4,
	MAX1125X_CTRL3_SYNC_MODE = 5,
	MAX1125X_CTRL3_GPO_MODE = 6,
};

enum {
	MAX1125X_GPIO_CTRL_DIO0 = 0,
	MAX1125X_GPIO_CTRL_DIO1 = 1,
	MAX1125X_GPIO_CTRL_DIRO = 3,
	MAX1125X_GPIO_CTRL_DIR1 = 4,
	MAX1125X_GPIO_CTRL_GPIO0_EN = 6,
	MAX1125X_GPIO_CTRL_GPIO1_EN = 7,
};

enum {
	MAX1125X_SEQ_RDYBEN = 0,
	MAX1125X_SEQ_MDREN = 1,
	MAX1125X_SEQ_GPODREN = 2,
	MAX1125X_SEQ_MODE0 = 3,
	MAX1125X_SEQ_MODE1 = 4,
	MAX1125X_SEQ_MUX0 = 5,
	MAX1125X_SEQ_MUX1 = 6,
	MAX1125X_SEQ_MUX2 = 7,
};

enum {
	MAX1125X_GPO_DIR_GPO0 = 0,
	MAX1125X_GPO_DIR_GPO1 = 1,
};

enum {
	MAX1125X_CMD_RATE0 = 0,
	MAX1125X_CMD_RATE1 = 1,
	MAX1125X_CMD_RATE2 = 2,
	MAX1125X_CMD_RATE3 = 3,
};

enum {
	MAX1125X_CHANNEL_0 = 0x0,
	MAX1125X_CHANNEL_1 = 0x1,
	MAX1125X_CHANNEL_2 = 0x2,
	MAX1125X_CHANNEL_3 = 0x3,
	MAX1125X_CHANNEL_4 = 0x4,
	MAX1125X_CHANNEL_5 = 0x5,
};

enum {
	MAX1125X_CMD_MODE0 = 4,
	MAX1125X_CMD_MODE1 = 5,
};

struct max1125x_gpio_ctrl {
	bool gpio0_enable;
	bool gpio1_enable;
	bool gpio0_direction;
	bool gpio1_direction;
};

struct max1125x_gpo_ctrl {
	bool gpo0_enable;
	bool gpo1_enable;
};

struct max1125x_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec drdy_gpio;
	const uint32_t odr_delay[16];
	uint8_t resolution;
	bool multiplexer;
	bool pga;
	bool self_calibration;
	struct max1125x_gpio_ctrl gpio;
	struct max1125x_gpo_ctrl gpo;
};

struct max1125x_data {
	const struct device *dev;
	struct adc_context ctx;
	uint8_t rate;
	struct gpio_callback callback_data_ready;
	struct k_sem acq_sem;
	struct k_sem data_ready_signal;
	int32_t *buffer;
	int32_t *repeat_buffer;
	struct k_thread thread;
	bool differential;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_MAX1125X_ACQUISITION_THREAD_STACK_SIZE);
};

static void max1125x_data_ready_handler(const struct device *dev, struct gpio_callback *gpio_cb,
					uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct max1125x_data *data =
		CONTAINER_OF(gpio_cb, struct max1125x_data, callback_data_ready);

	k_sem_give(&data->data_ready_signal);
}

static int max1125x_read_reg(const struct device *dev, enum max1125x_reg reg_addr, uint8_t *buffer,
			     size_t reg_size)
{
	int ret;
	const struct max1125x_config *config = dev->config;
	uint8_t buffer_tx[3];
	uint8_t buffer_rx[ARRAY_SIZE(buffer_tx)];
	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};
	buffer_tx[0] = MAX1125X_CMD_READ | reg_addr;
	/* read one register */
	buffer_tx[1] = 0x00;

	ret = spi_transceive_dt(&config->bus, &tx, &rx);
	if (ret != 0) {
		LOG_ERR("MAX1125X: error writing register 0x%X (%d)", reg_addr, ret);
		return ret;
	}
	*buffer = buffer_rx[1];
	LOG_DBG("read from register 0x%02X value 0x%02X", reg_addr, *buffer);

	return 0;
}

static int max1125x_write_reg(const struct device *dev, enum max1125x_reg reg_addr,
			      uint8_t *reg_val, size_t reg_size)
{
	int ret;
	const struct max1125x_config *config = dev->config;
	uint8_t command = MAX1125X_CMD_WRITE | reg_addr;

	const struct spi_buf spi_buf[2] = {{.buf = &command, .len = sizeof(command)},
					   {.buf = reg_val, .len = reg_size}};
	const struct spi_buf_set tx = {.buffers = spi_buf, .count = ARRAY_SIZE(spi_buf)};

	ret = spi_write_dt(&config->bus, &tx);
	if (ret != 0) {
		LOG_ERR("MAX1125X: error writing register 0x%X (%d)", reg_addr, ret);
		return ret;
	}

	return 0;
}

static int max1125x_send_command(const struct device *dev, enum max1125x_mode mode, uint8_t rate)
{
	int ret;
	const struct max1125x_config *config = dev->config;
	uint8_t command = MAX1125X_CMD_CONV | mode | rate;
	const struct spi_buf spi_buf = {.buf = &command, .len = sizeof(command)};
	const struct spi_buf_set tx = {.buffers = &spi_buf, .count = 1};

	ret = spi_write_dt(&config->bus, &tx);
	if (ret != 0) {
		LOG_ERR("MAX1125X: error writing register 0x%X (%d)", rate, ret);
		return ret;
	}

	return 0;
}

static int max1125x_start_conversion(const struct device *dev)
{
	const struct max1125x_data *data = dev->data;

	return max1125x_send_command(dev, MAX1125X_CMD_SEQUENCER, data->rate);
}

static inline int max1125x_acq_time_to_dr(const struct device *dev, uint16_t acq_time)
{
	struct max1125x_data *data = dev->data;
	const struct max1125x_config *config = dev->config;
	const uint32_t *odr_delay = config->odr_delay;
	uint32_t odr_delay_us = 0;
	uint16_t acq_value = ADC_ACQ_TIME_VALUE(acq_time);
	int odr = -EINVAL;

	if (acq_time != ADC_ACQ_TIME_DEFAULT && ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) {
		LOG_ERR("MAX1125X: invalid acq time value (%d)", acq_time);
		return -EINVAL;
	}

	if (acq_value < MAX1125X_CONFIG_RATE_1_9 || acq_value > MAX1125X_CONFIG_RATE_64000) {
		LOG_ERR("MAX1125X: invalid acq value (%d)", acq_value);
		return -EINVAL;
	}

	odr = acq_value;
	odr_delay_us = odr_delay[acq_value];

	data->rate = odr;

	return odr;
}

static int max1125x_wait_data_ready(const struct device *dev)
{
	struct max1125x_data *data = dev->data;

	return k_sem_take(&data->data_ready_signal, ADC_CONTEXT_WAIT_FOR_COMPLETION_TIMEOUT);
}

static int max1125x_read_sample(const struct device *dev)
{
	const struct max1125x_config *config = dev->config;
	struct max1125x_data *data = dev->data;
	bool is_positive;
	uint8_t buffer_tx[(config->resolution / 8) + 1];
	uint8_t buffer_rx[ARRAY_SIZE(buffer_tx)];
	uint8_t current_channel = find_msb_set(data->ctx.sequence.channels) - 1;
	int rc;

	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	buffer_tx[0] = MAX1125X_CMD_READ | MAX1125X_REG_DATA(current_channel);

	rc = spi_transceive_dt(&config->bus, &tx, &rx);
	if (rc != 0) {
		LOG_ERR("spi_transceive failed with error %i", rc);
		return rc;
	}

	/* The data format while in unipolar mode is always offset binary.
	 * In offset binary format the most negative value is 0x000000,
	 * the midscale value is 0x800000 and the most positive value is
	 * 0xFFFFFF. In bipolar mode if the FORMAT bit = ‘1’ then the
	 * data format is offset binary. If the FORMAT bit = ‘0’, then
	 * the data format is two’s complement. In two’s complement the
	 * negative full-scale value is 0x800000, the midscale is 0x000000
	 * and the positive full scale is 0x7FFFFF. Any input exceeding
	 * the available input range is limited to the minimum or maximum
	 * data value.
	 */
	is_positive = buffer_rx[(config->resolution / 8)] >> 7;
	if (is_positive) {
		*data->buffer++ = sys_get_be24(buffer_rx) - (1 << (config->resolution - 1));
	} else {
		*data->buffer++ = sys_get_be24(buffer_rx + 1);
	}

	adc_context_on_sampling_done(&data->ctx, dev);

	return rc;
}

static int max1125x_configure_chmap(const struct device *dev, const uint8_t channel_id)
{
	uint8_t last_order = 0;
	uint8_t chmap1_register[3] = {0};
	uint8_t chmap0_register[3] = {0};

	if (channel_id > 6) {
		LOG_ERR("MAX1125X: invalid channel (%u)", channel_id);
		return -EINVAL;
	}

	max1125x_read_reg(dev, MAX1125X_REG_CHMAP1, chmap1_register, MAX1125X_REG_CHMAP1_LEN);
	for (int index = 0; index < 3; index++) {
		if ((chmap1_register[index] >> 2) >= last_order) {
			last_order = chmap1_register[index] >> 2;
		} else {
			continue;
		}
	}

	max1125x_read_reg(dev, MAX1125X_REG_CHMAP0, chmap0_register, MAX1125X_REG_CHMAP0_LEN);
	for (int index = 0; index < 3; index++) {
		if ((chmap0_register[index] >> 2) >= last_order) {
			last_order = chmap0_register[index] >> 2;
		} else {
			continue;
		}
	}

	last_order++;

	switch (channel_id) {
	case MAX1125X_CHANNEL_0:
		chmap0_register[2] = MAX1125X_CONFIG_CHMAP(last_order);
		break;
	case MAX1125X_CHANNEL_1:
		chmap0_register[1] = MAX1125X_CONFIG_CHMAP(last_order);
		break;
	case MAX1125X_CHANNEL_2:
		chmap0_register[0] = MAX1125X_CONFIG_CHMAP(last_order);
		break;
	case MAX1125X_CHANNEL_3:
		chmap1_register[2] = MAX1125X_CONFIG_CHMAP(last_order);
		break;
	case MAX1125X_CHANNEL_4:
		chmap1_register[1] = MAX1125X_CONFIG_CHMAP(last_order);
		break;
	case MAX1125X_CHANNEL_5:
		chmap1_register[0] = MAX1125X_CONFIG_CHMAP(last_order);
		break;
	default:
		break;
	}

	if (channel_id > 3) {
		/* CHMAP 1 register configuration */
		max1125x_write_reg(dev, MAX1125X_REG_CHMAP1, chmap1_register,
				   MAX1125X_REG_CHMAP1_LEN);
	} else {
		/* CHMAP 0 register configuration */
		max1125x_write_reg(dev, MAX1125X_REG_CHMAP0, chmap0_register,
				   MAX1125X_REG_CHMAP0_LEN);
	}

	return 0;
}

static int max1125x_self_calibration(const struct device *dev)
{
	uint8_t seq_register = 0;
	uint8_t ctrl1_register = BIT(MAX1125X_CTRL1_SCYCLE);

	max1125x_write_reg(dev, MAX1125X_REG_SEQ, &seq_register, MAX1125X_REG_SEQ_LEN);
	max1125x_write_reg(dev, MAX1125X_REG_CTRL1, &ctrl1_register, MAX1125X_REG_CTRL1_LEN);
	max1125x_send_command(dev, MAX1125X_CMD_CALIBRATION, 0x00);

	k_sleep(K_MSEC(200));
	return 0;
}

static int max1125x_channel_setup(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	const struct max1125x_config *max_config = dev->config;
	uint8_t seq_register = 0;
	uint8_t ctrl2_register = 0;
	uint8_t gpio_reg = 0;
	uint8_t gpo_reg = 0;

	/* sequencer register configuration */
	max1125x_read_reg(dev, MAX1125X_REG_SEQ, &seq_register, MAX1125X_REG_SEQ_LEN);
	seq_register |= BIT(MAX1125X_SEQ_MDREN);
	seq_register |= BIT(MAX1125X_SEQ_MODE0);
	max1125x_write_reg(dev, MAX1125X_REG_SEQ, &seq_register, MAX1125X_REG_SEQ_LEN);

	/* configuration multiplexer */
	if (max_config->multiplexer) {
		if (!channel_cfg->differential) {
			LOG_ERR("6 channel fully supported only supported differential "
				"differemtial option %i",
				channel_cfg->differential);
			return -ENOTSUP;
		}
	}

	max1125x_acq_time_to_dr(dev, channel_cfg->acquisition_time);

	/* ctrl2 register configuration */
	if (max_config->pga) {
		/* programmable gain amplifier support */
		ctrl2_register |= MAX1125X_CONFIG_PGA(MAX1125X_CTRL2_PGAEN);
		switch (channel_cfg->gain) {
		case ADC_GAIN_1:
			ctrl2_register |= MAX1125X_CTRL2_PGA_GAIN_1;
			break;
		case ADC_GAIN_2:
			ctrl2_register |= MAX1125X_CTRL2_PGA_GAIN_2;
			break;
		case ADC_GAIN_4:
			ctrl2_register |= MAX1125X_CTRL2_PGA_GAIN_4;
			break;
		case ADC_GAIN_8:
			ctrl2_register |= MAX1125X_CTRL2_PGA_GAIN_8;
			break;
		case ADC_GAIN_16:
			ctrl2_register |= MAX1125X_CTRL2_PGA_GAIN_16;
			break;
		case ADC_GAIN_32:
			ctrl2_register |= MAX1125X_CTRL2_PGA_GAIN_32;
			break;
		case ADC_GAIN_64:
			ctrl2_register |= MAX1125X_CTRL2_PGA_GAIN_64;
			break;
		case ADC_GAIN_128:
			ctrl2_register |= MAX1125X_CTRL2_PGA_GAIN_128;
			break;
		default:
			LOG_ERR("MAX1125X: unsupported channel gain '%d'", channel_cfg->gain);
			return -ENOTSUP;
		}
	}

	if (channel_cfg->reference == ADC_REF_INTERNAL) {
		ctrl2_register |= BIT(MAX1125X_CTRL2_LDOEN);

	} else if (channel_cfg->reference == ADC_REF_EXTERNAL1) {
		ctrl2_register &= ~BIT(MAX1125X_CTRL2_LDOEN);
	} else {
		LOG_ERR("MAX1125X: unsupported channel reference type '%d'",
			channel_cfg->reference);
		return -ENOTSUP;
	}
	max1125x_write_reg(dev, MAX1125X_REG_CTRL2, &ctrl2_register, MAX1125X_REG_CTRL2_LEN);

	/* GPIO_CTRL register configuration */
	gpio_reg |= max_config->gpio.gpio0_enable << MAX1125X_GPIO_CTRL_GPIO0_EN;
	gpio_reg |= max_config->gpio.gpio1_enable << MAX1125X_GPIO_CTRL_GPIO1_EN;
	gpio_reg |= max_config->gpio.gpio0_direction << MAX1125X_GPIO_CTRL_DIRO;
	gpio_reg |= max_config->gpio.gpio1_direction << MAX1125X_GPIO_CTRL_DIR1;
	max1125x_write_reg(dev, MAX1125X_REG_GPIO_CTRL, &gpio_reg, MAX1125X_REG_GPIO_CTRL_LEN);

	/* GPO_DIR register configuration */
	gpo_reg |= max_config->gpo.gpo0_enable << MAX1125X_GPO_DIR_GPO0;
	gpo_reg |= max_config->gpo.gpo1_enable << MAX1125X_GPO_DIR_GPO1;
	max1125x_write_reg(dev, MAX1125X_REG_GPO_DIR, &gpo_reg, MAX1125X_REG_GPO_DIR_LEN);

	/* configuration of channel order */
	max1125x_configure_chmap(dev, channel_cfg->channel_id);

	return 0;
}

static int max1125x_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(uint8_t) * (sequence->resolution / 8);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int max1125x_validate_sequence(const struct device *dev, const struct adc_sequence *sequence)
{
	int err;

	if (sequence->oversampling) {
		LOG_ERR("MAX1125X: oversampling not supported");
		return -ENOTSUP;
	}

	err = max1125x_validate_buffer_size(sequence);
	if (err) {
		LOG_ERR("MAX1125X: buffer size too small");
		return -ENOTSUP;
	}

	return 0;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct max1125x_data *data = CONTAINER_OF(ctx, struct max1125x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct max1125x_data *data = CONTAINER_OF(ctx, struct max1125x_data, ctx);

	data->repeat_buffer = data->buffer;

	max1125x_start_conversion(data->dev);

	k_sem_give(&data->acq_sem);
}

static int max1125x_adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int rc;
	struct max1125x_data *data = dev->data;

	rc = max1125x_validate_sequence(dev, sequence);
	if (rc != 0) {
		return rc;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int max1125x_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	int rc;
	struct max1125x_data *data = dev->data;

	adc_context_lock(&data->ctx, async ? true : false, async);
	rc = max1125x_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, rc);

	return rc;
}

static int max1125x_adc_perform_read(const struct device *dev)
{
	struct max1125x_data *data = dev->data;
	int rc;

	rc = max1125x_read_sample(dev);
	if (rc != 0) {
		LOG_ERR("reading sample failed (err %d)", rc);
		adc_context_complete(&data->ctx, rc);
		return rc;
	}

	return rc;
}

static int max1125x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return max1125x_adc_read_async(dev, sequence, NULL);
}

static void max1125x_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct max1125x_data *data = dev->data;
	int rc;

	while (true) {
		k_sem_take(&data->acq_sem, K_FOREVER);

		rc = max1125x_wait_data_ready(dev);
		if (rc != 0) {
			LOG_ERR("MAX1125X: failed to get ready status (err %d)", rc);
			adc_context_complete(&data->ctx, rc);
			break;
		}

		max1125x_adc_perform_read(dev);
	}
}

static int max1125x_init(const struct device *dev)
{
	int err;
	const struct max1125x_config *config = dev->config;
	struct max1125x_data *data = dev->data;

	data->dev = dev;

	k_sem_init(&data->acq_sem, 0, 1);
	k_sem_init(&data->data_ready_signal, 0, 1);

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("spi bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (config->self_calibration) {
		LOG_INF("performing self calibration process");
		max1125x_self_calibration(dev);
	}

	err = gpio_pin_configure_dt(&config->drdy_gpio, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("failed to initialize GPIO for data ready (err %d)", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		LOG_ERR("failed to configure data ready interrupt (err %d)", err);
		return -EIO;
	}

	gpio_init_callback(&data->callback_data_ready, max1125x_data_ready_handler,
			   BIT(config->drdy_gpio.pin));
	err = gpio_add_callback(config->drdy_gpio.port, &data->callback_data_ready);
	if (err != 0) {
		LOG_ERR("failed to add data ready callback (err %d)", err);
		return -EIO;
	}

	k_tid_t tid = k_thread_create(
		&data->thread, data->stack, K_KERNEL_STACK_SIZEOF(data->stack),
		max1125x_acquisition_thread, (void *)dev, NULL, NULL,
		CONFIG_ADC_MAX1125X_ACQUISITION_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_max1125x");

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, max1125x_api) = {
	.channel_setup = max1125x_channel_setup,
	.read = max1125x_read,
	.ref_internal = 2048,
#ifdef CONFIG_ADC_ASYNC
	.read_async = max1125x_adc_read_async,
#endif
};

#define DT_INST_MAX1125X(inst, t) DT_INST(inst, maxim_max##t)

#define MAX1125X_INIT(t, n, odr_delay_us, res, mux, pgab)                                          \
	static const struct max1125x_config max##t##_cfg_##n = {                                   \
		.bus = SPI_DT_SPEC_GET(DT_INST_MAX1125X(n, t),                                     \
				       SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,    \
				       1),                                                         \
		.odr_delay = odr_delay_us,                                                         \
		.resolution = res,                                                                 \
		.multiplexer = mux,                                                                \
		.pga = pgab,                                                                       \
		.drdy_gpio = GPIO_DT_SPEC_GET_OR(DT_INST_MAX1125X(n, t), drdy_gpios, {0}),         \
		.self_calibration = DT_PROP_OR(DT_INST_MAX1125X(n, t), self_calibration, 0),       \
		.gpio.gpio0_enable = DT_PROP_OR(DT_INST_MAX1125X(n, t), gpio0_enable, 1),          \
		.gpio.gpio1_enable = DT_PROP_OR(DT_INST_MAX1125X(n, t), gpio1_enable, 0),          \
		.gpio.gpio0_direction = DT_PROP_OR(DT_INST_MAX1125X(n, t), gpio0_direction, 0),    \
		.gpio.gpio1_direction = DT_PROP_OR(DT_INST_MAX1125X(n, t), gpio1_direction, 0),    \
		.gpo.gpo0_enable = DT_PROP_OR(DT_INST_MAX1125X(n, t), gpo1_enable, 0),             \
		.gpo.gpo1_enable = DT_PROP_OR(DT_INST_MAX1125X(n, t), gpo1_enable, 0),             \
	};                                                                                         \
	static struct max1125x_data max##t##_data_##n = {                                          \
		ADC_CONTEXT_INIT_LOCK(max##t##_data_##n, ctx),                                     \
		ADC_CONTEXT_INIT_TIMER(max##t##_data_##n, ctx),                                    \
		ADC_CONTEXT_INIT_SYNC(max##t##_data_##n, ctx),                                     \
	};                                                                                         \
	DEVICE_DT_DEFINE(DT_INST_MAX1125X(n, t), max1125x_init, NULL, &max##t##_data_##n,          \
			 &max##t##_cfg_##n, POST_KERNEL, CONFIG_ADC_MAX1125X_INIT_PRIORITY,        \
			 &max1125x_api);

/* Each data register is a 16-bit read-only register. Any attempt to write
 * data to this location will have no effect. The data read from these
 * registers is clocked out MSB first. The result is stored in a format
 * according to the FORMAT bit in the CTRL1 register. The data format
 * while in unipolar mode is always offset binary. In offset binary
 * format the most negative value is 0x0000, the midscale value is 0x8000 and
 *  the most positive value is 0xFFFF. In bipolar mode if the FORMAT
 * bit = ‘1’ then the data format is offset binary. If the FORMAT
 * bit= ‘0’, then the data format is two’s complement. In two’s
 * complement the negative full-scale value is 0x8000, the midscale is 0x0000
 * and the positive full scale is 0x7FFF. Any input exceeding the available
 * input range is limited to the minimum or maximum data value.
 */

#define MAX11253_RESOLUTION 16

/* Each data register is a 24-bit read-only register. Any attempt to write
 * data to this location will have no effect. The data read from these
 * registers is clocked out MSB first. The result is stored in a format
 * according to the FORMAT bit in the CTRL1 register. The data format
 * while in unipolar mode is always offset binary. In offset binary format
 * the most negative value is 0x000000, the midscale value is 0x800000 and
 * the most positive value is 0xFFFFFF. In bipolar mode if the FORMAT
 * bit = ‘1’ then the data format is offset binary. If the FORMAT bit = ‘0’,
 * then the data format is two’s complement. In two’s complement the negative
 * full-scale value is 0x800000, the midscale is 0x000000 and the positive
 * full scale is 0x7FFFFF. Any input exceeding the available input range is
 * limited to the minimum or maximum data value.
 */

#define MAX11254_RESOLUTION 24

/*
 * Approximated MAX1125X acquisition times in microseconds. These are
 * used for the initial delay when polling for data ready.
 *
 * {1.9 SPS, 3.9 SPS, 7.8 SPS, 15.6 SPS, 31.2 SPS, 62.5 SPS, 125 SPS, 250 SPS, 500 SPS,
 * 1000 SPS, 2000 SPS, 4000 SPS, 8000 SPS, 16000 SPS, 32000 SPS, 64000 SPS}
 */
#define MAX1125X_ODR_DELAY_US                                                                      \
	{                                                                                          \
		526315, 256410, 128205, 64102, 32051, 16000, 8000, 4000, 2000, 1000, 500, 250,     \
			125, 62, 31, 15                                                            \
	}

/*
 * MAX11253: 16 bit, 6-channel, programmable gain amplifier, delta-sigma
 */
#define MAX11253_INIT(n)                                                                           \
	MAX1125X_INIT(11253, n, MAX1125X_ODR_DELAY_US, MAX11253_RESOLUTION, false, true)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT maxim_max11253
DT_INST_FOREACH_STATUS_OKAY(MAX11253_INIT)

/*
 * MAX1125X: 24 bit, 6-channel, programmable gain amplifier, delta-sigma
 */

#define MAX11254_INIT(n)                                                                           \
	MAX1125X_INIT(11254, n, MAX1125X_ODR_DELAY_US, MAX11254_RESOLUTION, false, true)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT maxim_max11254
DT_INST_FOREACH_STATUS_OKAY(MAX11254_INIT)
