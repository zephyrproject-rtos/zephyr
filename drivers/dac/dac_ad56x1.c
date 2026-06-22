/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(dac_ad56x1, CONFIG_DAC_LOG_LEVEL);

/*
 * https://www.analog.com/media/en/technical-documentation/data-sheets/AD5601_5611_5621.pdf
 *
 * The Analog Device AD5601, AD5611, and AD5621 are 8, 10, and 12 bits single channel SPI DACs.
 */

/*
 * The AD56x1 devices support a NORMAL mode where the output is connected to the output amplifier
 * driven by a resistor string. These devices also support 3 power down modes in which the output is
 * disconnected from the amplifier and is either:
 * - connected to GND through a 1 kΩ resistor
 * - connected to GND through a 100 kΩ resistor
 * - Disconnected (three-state)
 *
 * This driver only support the normal mode to stick to the regular DAC API.
 */
#define DAC_AD56X1_MODE_NORMAL                 0x0000
#define DAC_AD56X1_MODE_POWER_DOWN_1K          0x4000
#define DAC_AD56X1_MODE_POWER_DOWN_100K        0x8000
#define DAC_AD56X1_MODE_POWER_DOWN_THREE_STATE 0xC000

struct ad56x1_config {
	struct spi_dt_spec bus;
	uint8_t resolution;
};

static int ad56x1_channel_setup(const struct device *dev, const struct dac_channel_cfg *channel_cfg)
{
	const struct ad56x1_config *config = dev->config;

	if (channel_cfg->channel_id != 0) {
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

static int ad56x1_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct ad56x1_config *config = dev->config;
	uint8_t buffer_tx[2];
	uint16_t command = DAC_AD56X1_MODE_NORMAL;
	int result;

	if (value > BIT(config->resolution) - 1) {
		LOG_ERR("invalid value %i", value);
		return -EINVAL;
	}

	if (channel != 0) {
		LOG_ERR("invalid channel %i", channel);
		return -EINVAL;
	}

	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	command |= value << (14 - config->resolution);
	sys_put_be16(command, buffer_tx);

	LOG_DBG("sending to DAC %s command 0x%02X, (value 0x%04X, normal mode)", dev->name, command,
		value);
	result = spi_write_dt(&config->bus, &tx);

	if (result != 0) {
		LOG_ERR("spi_transceive failed with error %i", result);
		return result;
	}

	return 0;
}

static int ad56x1_init(const struct device *dev)
{
	const struct ad56x1_config *config = dev->config;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(dac, ad56x1_driver_api) = {
	.channel_setup = ad56x1_channel_setup,
	.write_value = ad56x1_write_value,
};

BUILD_ASSERT(CONFIG_DAC_AD56X1_INIT_PRIORITY > CONFIG_SPI_INIT_PRIORITY,
	     "CONFIG_DAC_AD56X1_INIT_PRIORITY must be higher than CONFIG_SPI_INIT_PRIORITY");

#define DAC_AD56X1_INST_DEFINE(index, name, res)                                                   \
	static const struct ad56x1_config config_##name##_##index = {                              \
		.bus = SPI_DT_SPEC_INST_GET(index,                                                 \
					    SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8)), \
		.resolution = res};                                                                \
	DEVICE_DT_INST_DEFINE(index, ad56x1_init, NULL, NULL, &config_##name##_##index,            \
			      POST_KERNEL, CONFIG_DAC_AD56X1_INIT_PRIORITY, &ad56x1_driver_api);

#define DT_DRV_COMPAT adi_ad5601
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define DAC_AD5601_RESOLUTION 8
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56X1_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5601_RESOLUTION)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5611
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define DAC_AD5611_RESOLUTION 10
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56X1_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5611_RESOLUTION)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5621
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define DAC_AD5621_RESOLUTION 12
DT_INST_FOREACH_STATUS_OKAY_VARGS(DAC_AD56X1_INST_DEFINE, DT_DRV_COMPAT, DAC_AD5621_RESOLUTION)
#endif
#undef DT_DRV_COMPAT
