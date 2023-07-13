/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(dac_ad568x, CONFIG_DAC_LOG_LEVEL);

/*
 * The wait times are actually only 30ns, but as we can wait
 * only a minimum amount of 1us this value is getting used.
 * This will ensure that the device has been properly resetted.
 */
#define DAC_AD568X_CHANNEL_COUNT         2
#define DAC_AD568X_RESET_LOW_TIME_IN_US  1
#define DAC_AD568X_RESET_WAIT_TIME_IN_US 1

enum ad568x_command {
	AD586XCOMMAND_WRITEANDUPDATECHANNEL = 0b0011,
	AD586XCOMMAND_SOFTWARERESET = 0b0110,
};

enum ad568x_address {
	AD568XADDRESS_DACA = 0b0001,
	AD568XADDRESS_DACB = 0b1000,
	AD568XADDRESS_DACAANDDACB = 0b1001,
};

struct ad568x_config {
	struct spi_dt_spec bus;
	const struct gpio_dt_spec gpio_reset;
	uint8_t resolution;
};

struct ad568x_data {
};

static int ad568x_write_command(const struct device *dev, enum ad568x_command command,
				enum ad568x_address address, uint16_t value)
{
	const struct ad568x_config *config = dev->config;
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

	buffer_tx[0] = (command << 4) | address;
	value = value << (16 - config->resolution);
	sys_put_be16(value, buffer_tx + 1);

	int result = spi_transceive_dt(&config->bus, &tx, &rx);

	if (result != 0) {
		LOG_ERR("spi_transceive failed with error %i", result);
		return result;
	}

	return 0;
}

static int ad568x_channel_setup(const struct device *dev,
				const struct dac_channel_cfg *channel_cfg)
{
	const struct ad568x_config *config = dev->config;

	if (channel_cfg->channel_id >= DAC_AD568X_CHANNEL_COUNT) {
		LOG_ERR("invalid channel %i", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("invalid resolution %i", channel_cfg->resolution);
		return -EINVAL;
	}

	return 0;
}

static int ad568x_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	enum ad568x_address address;

	if (value > UINT16_MAX) {
		LOG_ERR("invalid value %i", value);
		return -EINVAL;
	}

	switch (channel) {
	case 0:
		address = AD568XADDRESS_DACA;
		break;
	case 1:
		address = AD568XADDRESS_DACB;
		break;
	default:
		LOG_ERR("invalid channel %i", channel);
		return -EINVAL;
	}

	return ad568x_write_command(dev, AD586XCOMMAND_WRITEANDUPDATECHANNEL,
				    AD568XADDRESS_DACAANDDACB, value);
}

static int ad568x_init(const struct device *dev)
{
	const struct ad568x_config *config = dev->config;
	int result;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (config->gpio_reset.port != NULL) {
		result = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_ACTIVE);
		if (result != 0) {
			LOG_ERR("failed to initialize GPIO for reset");
			return result;
		}

		k_busy_wait(DAC_AD568X_RESET_LOW_TIME_IN_US);
		gpio_pin_set_dt(&config->gpio_reset, 0);
	} else {
		result = ad568x_write_command(dev, AD586XCOMMAND_SOFTWARERESET,
					      AD568XADDRESS_DACAANDDACB, 0);
	}

	k_busy_wait(DAC_AD568X_RESET_WAIT_TIME_IN_US);

	return 0;
}

static const struct dac_driver_api ad568x_driver_api = {
	.channel_setup = ad568x_channel_setup,
	.write_value = ad568x_write_value,
};

BUILD_ASSERT(CONFIG_DAC_AD568X_INIT_PRIORITY > CONFIG_SPI_INIT_PRIORITY,
	     "CONFIG_DAC_AD568X_INIT_PRIORITY must be higher than CONFIG_SPI_INIT_PRIORITY");

#define DAC_AD568X_INST_DEFINE(index, name, res)                                                   \
	static struct ad568x_data data_##name##_##index;                                           \
	static const struct ad568x_config config_##name##_##index = {                              \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			index, SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8), 0),           \
		.resolution = res,                                                                 \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, ad568x_init, NULL, &data_##name##_##index,                    \
			      &config_##name##_##index, POST_KERNEL,                               \
			      CONFIG_DAC_AD568X_INIT_PRIORITY, &ad568x_driver_api);

#define DT_DRV_COMPAT         adi_ad5687
#define DAC_AD5687_RESOLUTION 12
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD568X_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5687_RESOLUTION)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT         adi_ad5689
#define DAC_AD5689_RESOLUTION 16
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD568X_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5689_RESOLUTION)
#undef DT_DRV_COMPAT
