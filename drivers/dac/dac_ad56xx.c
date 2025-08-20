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

LOG_MODULE_REGISTER(dac_ad56xx, CONFIG_DAC_LOG_LEVEL);

/*
 * These values are actually all way less than 1us, but we can only
 * wait with 1us precision.
 *
 * This should be checked when new types of this series are added to
 * this implementation.
 */
#define DAC_AD56XX_MINIMUM_PULSE_WIDTH_LOW_IN_US 1
#define DAC_AD56XX_PULSE_ACTIVATION_TIME_IN_US   1

enum ad56xx_command {
	AD56XX_CMD_WRITE_UPDATE_CHANNEL = 3,
	AD56XX_CMD_SOFTWARE_RESET = 6,
};

struct ad56xx_config {
	struct spi_dt_spec bus;
	const struct gpio_dt_spec gpio_reset;
	uint8_t resolution;

	const uint8_t *channel_addresses;
	size_t channel_count;
};

struct ad56xx_data {
};

static int ad56xx_write_command(const struct device *dev, enum ad56xx_command command,
				uint8_t address, uint16_t value)
{
	const struct ad56xx_config *config = dev->config;
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

	LOG_DBG("sending to DAC %s command 0x%02X, address 0x%02X and value 0x%04X", dev->name,
		command, address, value);
	int result = spi_transceive_dt(&config->bus, &tx, &rx);

	if (result != 0) {
		LOG_ERR("spi_transceive failed with error %i", result);
		return result;
	}

	return 0;
}

static int ad56xx_channel_setup(const struct device *dev, const struct dac_channel_cfg *channel_cfg)
{
	const struct ad56xx_config *config = dev->config;

	if (channel_cfg->channel_id >= config->channel_count) {
		LOG_ERR("invalid channel %i", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("invalid resolution %i", channel_cfg->resolution);
		return -EINVAL;
	}

	if (channel_cfg->internal) {
		LOG_ERR("Internal channels not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int ad56xx_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct ad56xx_config *config = dev->config;

	if (value > BIT(config->resolution) - 1) {
		LOG_ERR("invalid value %i", value);
		return -EINVAL;
	}

	if (channel >= config->channel_count) {
		LOG_ERR("invalid channel %i", channel);
		return -EINVAL;
	}

	return ad56xx_write_command(dev, AD56XX_CMD_WRITE_UPDATE_CHANNEL,
				    config->channel_addresses[channel], value);
}

static int ad56xx_init(const struct device *dev)
{
	const struct ad56xx_config *config = dev->config;
	int result;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (config->gpio_reset.port != NULL) {
		LOG_DBG("reset %s with GPIO", dev->name);
		result = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_ACTIVE);
		if (result != 0) {
			LOG_ERR("failed to initialize GPIO for reset");
			return result;
		}

		k_busy_wait(DAC_AD56XX_MINIMUM_PULSE_WIDTH_LOW_IN_US);
		gpio_pin_set_dt(&config->gpio_reset, 0);
	} else {
		LOG_DBG("reset %s with command", dev->name);
		result = ad56xx_write_command(dev, AD56XX_CMD_SOFTWARE_RESET, 0, 0);
		if (result != 0) {
			LOG_ERR("failed to send reset command");
			return result;
		}
	}

	/*
	 * The pulse activation time is actually defined to start together
	 * with the pulse start. To be on the safe side we add the wait time
	 * on top of the actual pulse.
	 */
	k_busy_wait(DAC_AD56XX_PULSE_ACTIVATION_TIME_IN_US);

	return 0;
}

static DEVICE_API(dac, ad56xx_driver_api) = {
	.channel_setup = ad56xx_channel_setup,
	.write_value = ad56xx_write_value,
};

BUILD_ASSERT(CONFIG_DAC_AD56XX_INIT_PRIORITY > CONFIG_SPI_INIT_PRIORITY,
	     "CONFIG_DAC_AD56XX_INIT_PRIORITY must be higher than CONFIG_SPI_INIT_PRIORITY");

#define DAC_AD56XX_INST_DEFINE(index, name, res, channels, channels_count)                         \
	static struct ad56xx_data data_##name##_##index;                                           \
	static const struct ad56xx_config config_##name##_##index = {                              \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			index, SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8), 0),           \
		.resolution = res,                                                                 \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(index, reset_gpios, {0}),                   \
		.channel_addresses = channels,                                                     \
		.channel_count = channels_count,                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, ad56xx_init, NULL, &data_##name##_##index,                    \
			      &config_##name##_##index, POST_KERNEL,                               \
			      CONFIG_DAC_AD56XX_INIT_PRIORITY, &ad56xx_driver_api);

#define DT_DRV_COMPAT adi_ad5628
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5628_channels[] = {
	0, 1, 2, 3, 4, 5, 6, 7,
};
#define DAC_AD5628_RESOLUTION    12
#define DAC_AD5628_CHANNELS      ad5628_channels
#define DAC_AD5628_CHANNEL_COUNT ARRAY_SIZE(ad5628_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5628_RESOLUTION,
				  DAC_AD5628_CHANNELS, DAC_AD5628_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5648
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5648_channels[] = {
	0, 1, 2, 3, 4, 5, 6, 7,
};
#define DAC_AD5648_RESOLUTION    14
#define DAC_AD5648_CHANNELS      ad5648_channels
#define DAC_AD5648_CHANNEL_COUNT ARRAY_SIZE(ad5648_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5648_RESOLUTION,
				  DAC_AD5648_CHANNELS, DAC_AD5648_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5668
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5668_channels[] = {
	0, 1, 2, 3, 4, 5, 6, 7,
};
#define DAC_AD5668_RESOLUTION    16
#define DAC_AD5668_CHANNELS      ad5668_channels
#define DAC_AD5668_CHANNEL_COUNT ARRAY_SIZE(ad5668_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5668_RESOLUTION,
				  DAC_AD5668_CHANNELS, DAC_AD5668_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5672
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5672_channels[] = {
	0, 1, 2, 3, 4, 5, 6, 7,
};
#define DAC_AD5672_RESOLUTION    12
#define DAC_AD5672_CHANNELS      ad5672_channels
#define DAC_AD5672_CHANNEL_COUNT ARRAY_SIZE(ad5672_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5672_RESOLUTION,
				  DAC_AD5672_CHANNELS, DAC_AD5672_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5674
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5674_channels[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};
#define DAC_AD5674_RESOLUTION    12
#define DAC_AD5674_CHANNELS      ad5674_channels
#define DAC_AD5674_CHANNEL_COUNT ARRAY_SIZE(ad5674_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5674_RESOLUTION,
				  DAC_AD5674_CHANNELS, DAC_AD5674_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5676
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5676_channels[] = {
	0, 1, 2, 3, 4, 5, 6, 7,
};
#define DAC_AD5676_RESOLUTION    16
#define DAC_AD5676_CHANNELS      ad5676_channels
#define DAC_AD5676_CHANNEL_COUNT ARRAY_SIZE(ad5676_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5676_RESOLUTION,
				  DAC_AD5676_CHANNELS, DAC_AD5676_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5679
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5679_channels[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};
#define DAC_AD5679_RESOLUTION    16
#define DAC_AD5679_CHANNELS      ad5679_channels
#define DAC_AD5679_CHANNEL_COUNT ARRAY_SIZE(ad5679_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5679_RESOLUTION,
				  DAC_AD5679_CHANNELS, DAC_AD5679_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5684
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5684_channels[] = {
	1,
	2,
	4,
	8,
};
#define DAC_AD5684_RESOLUTION    12
#define DAC_AD5684_CHANNELS      ad5684_channels
#define DAC_AD5684_CHANNEL_COUNT ARRAY_SIZE(ad5684_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5684_RESOLUTION,
				  DAC_AD5684_CHANNELS, DAC_AD5684_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5686
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5686_channels[] = {
	1,
	2,
	4,
	8,
	3,
	15,
};
#define DAC_AD5686_RESOLUTION    16
#define DAC_AD5686_CHANNELS      ad5686_channels
#define DAC_AD5686_CHANNEL_COUNT ARRAY_SIZE(ad5686_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5686_RESOLUTION,
				  DAC_AD5686_CHANNELS, DAC_AD5686_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5687
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5687_channels[] = {
	1,
	8,
};
#define DAC_AD5687_RESOLUTION    12
#define DAC_AD5687_CHANNELS      ad5687_channels
#define DAC_AD5687_CHANNEL_COUNT ARRAY_SIZE(ad5687_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5687_RESOLUTION,
				  DAC_AD5687_CHANNELS, DAC_AD5687_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5689
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static const uint8_t ad5689_channels[] = {
	1,
	8,
};
#define DAC_AD5689_RESOLUTION    16
#define DAC_AD5689_CHANNELS      ad5689_channels
#define DAC_AD5689_CHANNEL_COUNT ARRAY_SIZE(ad5689_channels)
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56XX_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5689_RESOLUTION,
				  DAC_AD5689_CHANNELS, DAC_AD5689_CHANNEL_COUNT)
#endif
#undef DT_DRV_COMPAT
