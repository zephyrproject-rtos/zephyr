/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/analog_switch.h>

LOG_MODULE_REGISTER(adgm3121, CONFIG_ANALOG_SWITCH_LOG_LEVEL);

#define ADGM3121_NUM_CHANNELS		4

#define ADGM3121_REG_SWITCH_DATA	0x20

#define ADGM3121_SWITCH_MSK		GENMASK(3, 0)

#define ADGM3121_SPI_READ		BIT(15)
#define ADGM3121_SPI_ADDR_MSK		GENMASK(14, 8)

#define ADGM3121_SWITCHING_TIME_US	200
#define ADGM3121_POWER_UP_TIME_MS	45

enum adgm3121_mode {
	ADGM3121_MODE_SPI,
	ADGM3121_MODE_GPIO,
};

struct adgm3121_config {
	enum adgm3121_mode mode;
	union {
		struct spi_dt_spec spi;
		struct gpio_dt_spec gpios[ADGM3121_NUM_CHANNELS];
	};
};

struct adgm3121_data {
	struct k_mutex lock;
	uint8_t switch_states;
};

static int adgm3121_spi_write_reg(const struct device *dev, uint8_t reg_addr,
				  uint8_t data)
{
	const struct adgm3121_config *cfg = dev->config;
	uint16_t cmd;
	uint8_t buf[2];
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 2,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	cmd = FIELD_PREP(ADGM3121_SPI_ADDR_MSK, reg_addr) | data;
	buf[0] = cmd >> 8;
	buf[1] = cmd & 0xFF;

	return spi_write_dt(&cfg->spi, &tx);
}

static int adgm3121_spi_read_reg(const struct device *dev, uint8_t reg_addr,
				 uint8_t *data)
{
	const struct adgm3121_config *cfg = dev->config;
	uint16_t cmd;
	uint8_t tx_data[2];
	uint8_t rx_data[2];
	const struct spi_buf tx_buf = {
		.buf = tx_data,
		.len = 2,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_buf = {
		.buf = rx_data,
		.len = 2,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};
	int ret;

	cmd = ADGM3121_SPI_READ | FIELD_PREP(ADGM3121_SPI_ADDR_MSK, reg_addr);
	tx_data[0] = cmd >> 8;
	tx_data[1] = cmd & 0xFF;

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret) {
		return ret;
	}

	*data = rx_data[1];

	return 0;
}

static int adgm3121_set(const struct device *dev, uint8_t channel,
			uint8_t state)
{
	const struct adgm3121_config *cfg = dev->config;
	struct adgm3121_data *data = dev->data;
	uint8_t new_states;
	int ret;

	if (channel >= ADGM3121_NUM_CHANNELS || state > 1) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (cfg->mode == ADGM3121_MODE_GPIO) {
		ret = gpio_pin_set_dt(&cfg->gpios[channel], state);
		if (ret) {
			goto unlock;
		}
		if (state) {
			data->switch_states |= BIT(channel);
		} else {
			data->switch_states &= ~BIT(channel);
		}
	} else {
		if (state) {
			new_states = data->switch_states | BIT(channel);
		} else {
			new_states = data->switch_states & ~BIT(channel);
		}

		ret = adgm3121_spi_write_reg(dev, ADGM3121_REG_SWITCH_DATA,
					     new_states);
		if (ret) {
			goto unlock;
		}

		data->switch_states = new_states;
		k_usleep(ADGM3121_SWITCHING_TIME_US);
	}

	ret = 0;

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int adgm3121_get(const struct device *dev, uint8_t channel,
			uint8_t *state)
{
	const struct adgm3121_config *cfg = dev->config;
	struct adgm3121_data *data = dev->data;
	int ret;

	if (channel >= ADGM3121_NUM_CHANNELS || state == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (cfg->mode == ADGM3121_MODE_GPIO) {
		ret = gpio_pin_get_dt(&cfg->gpios[channel]);
		if (ret < 0) {
			goto unlock;
		}
		*state = ret ? 1 : 0;
		ret = 0;
	} else {
		uint8_t reg_data;

		ret = adgm3121_spi_read_reg(dev, ADGM3121_REG_SWITCH_DATA,
					    &reg_data);
		if (ret) {
			goto unlock;
		}

		data->switch_states = reg_data & ADGM3121_SWITCH_MSK;
		*state = (reg_data & BIT(channel)) ? 1 : 0;
	}

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int adgm3121_set_all(const struct device *dev, uint32_t mask)
{
	const struct adgm3121_config *cfg = dev->config;
	struct adgm3121_data *data = dev->data;
	uint8_t sw_mask = mask & ADGM3121_SWITCH_MSK;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (cfg->mode == ADGM3121_MODE_GPIO) {
		for (int i = 0; i < ADGM3121_NUM_CHANNELS; i++) {
			ret = gpio_pin_set_dt(&cfg->gpios[i],
					      (sw_mask & BIT(i)) ? 1 : 0);
			if (ret) {
				goto unlock;
			}
		}
		data->switch_states = sw_mask;
	} else {
		ret = adgm3121_spi_write_reg(dev, ADGM3121_REG_SWITCH_DATA,
					     sw_mask);
		if (ret) {
			goto unlock;
		}

		data->switch_states = sw_mask;
		k_usleep(ADGM3121_SWITCHING_TIME_US);
	}

	ret = 0;

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int adgm3121_get_all(const struct device *dev, uint32_t *mask)
{
	const struct adgm3121_config *cfg = dev->config;
	struct adgm3121_data *data = dev->data;
	int ret;

	if (mask == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (cfg->mode == ADGM3121_MODE_GPIO) {
		*mask = 0;
		for (int i = 0; i < ADGM3121_NUM_CHANNELS; i++) {
			ret = gpio_pin_get_dt(&cfg->gpios[i]);
			if (ret < 0) {
				goto unlock;
			}
			if (ret) {
				*mask |= BIT(i);
			}
		}
		data->switch_states = *mask;
		ret = 0;
	} else {
		uint8_t reg_data;

		ret = adgm3121_spi_read_reg(dev, ADGM3121_REG_SWITCH_DATA,
					    &reg_data);
		if (ret) {
			goto unlock;
		}

		*mask = reg_data & ADGM3121_SWITCH_MSK;
		data->switch_states = *mask;
	}

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int adgm3121_reset(const struct device *dev)
{
	return adgm3121_set_all(dev, 0);
}

static DEVICE_API(analog_switch, adgm3121_api) = {
	.set = adgm3121_set,
	.get = adgm3121_get,
	.set_all = adgm3121_set_all,
	.get_all = adgm3121_get_all,
	.reset = adgm3121_reset,
	.num_channels = ADGM3121_NUM_CHANNELS,
};

static int adgm3121_init(const struct device *dev)
{
	const struct adgm3121_config *cfg = dev->config;
	struct adgm3121_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);
	data->switch_states = 0;

	if (cfg->mode == ADGM3121_MODE_SPI) {
		if (!spi_is_ready_dt(&cfg->spi)) {
			LOG_ERR("SPI bus not ready");
			return -ENODEV;
		}
	} else {
		for (int i = 0; i < ADGM3121_NUM_CHANNELS; i++) {
			if (!gpio_is_ready_dt(&cfg->gpios[i])) {
				LOG_ERR("GPIO %d not ready", i);
				return -ENODEV;
			}

			ret = gpio_pin_configure_dt(&cfg->gpios[i],
						    GPIO_OUTPUT_INACTIVE);
			if (ret) {
				LOG_ERR("Failed to configure GPIO %d: %d",
					i, ret);
				return ret;
			}
		}
	}

	k_msleep(ADGM3121_POWER_UP_TIME_MS);

	ret = adgm3121_reset(dev);
	if (ret) {
		LOG_ERR("Failed to reset switches: %d", ret);
		return ret;
	}

	return 0;
}

#define ADGM3121_SPI_DEFINE(inst)					\
	static struct adgm3121_data adgm3121_data_spi_##inst;		\
	static const struct adgm3121_config				\
		adgm3121_config_spi_##inst = {				\
		.mode = ADGM3121_MODE_SPI,				\
		.spi = SPI_DT_SPEC_INST_GET(inst,			\
			SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |	\
			SPI_WORD_SET(8), 0),				\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
			      adgm3121_init,				\
			      NULL,					\
			      &adgm3121_data_spi_##inst,		\
			      &adgm3121_config_spi_##inst,		\
			      POST_KERNEL,				\
			      CONFIG_ANALOG_SWITCH_INIT_PRIORITY,	\
			      &adgm3121_api);

#define ADGM3121_GPIO_DEFINE(inst)					\
	static struct adgm3121_data adgm3121_data_gpio_##inst;		\
	static const struct adgm3121_config				\
		adgm3121_config_gpio_##inst = {				\
		.mode = ADGM3121_MODE_GPIO,				\
		.gpios = {						\
			GPIO_DT_SPEC_INST_GET(inst, in1_gpios),		\
			GPIO_DT_SPEC_INST_GET(inst, in2_gpios),		\
			GPIO_DT_SPEC_INST_GET(inst, in3_gpios),		\
			GPIO_DT_SPEC_INST_GET(inst, in4_gpios),		\
		},							\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
			      adgm3121_init,				\
			      NULL,					\
			      &adgm3121_data_gpio_##inst,		\
			      &adgm3121_config_gpio_##inst,		\
			      POST_KERNEL,				\
			      CONFIG_ANALOG_SWITCH_INIT_PRIORITY,	\
			      &adgm3121_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_adgm3121
DT_INST_FOREACH_STATUS_OKAY(ADGM3121_SPI_DEFINE)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_adgm3121_gpio
DT_INST_FOREACH_STATUS_OKAY(ADGM3121_GPIO_DEFINE)
