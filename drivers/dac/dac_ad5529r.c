/*
 * Copyright (c) 2026 The Zephyr Project Contributors
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

LOG_MODULE_REGISTER(dac_ad5529r, CONFIG_DAC_LOG_LEVEL);

/*
 * AD5529R is a 16-channel, 12-bit/16-bit voltage output DAC from Analog Devices.
 * https://www.analog.com/media/en/technical-documentation/data-sheets/ad5529r.pdf
 *
 * SPI Frame Format:
 * - 16-bit instruction phase: [R/W (1-bit), Device ID (3-bit), Address (12-bit)]
 * - 16-bit data phase
 *
 * The device supports multiple output voltage ranges:
 * 0-5V, 0-10V, 0-20V, 0-40V, +/-5V, +/-10V, +/-15V, +/-20V
 */
#define AD5529R_NUM_CHANNELS 16
#define AD5529R_RESOLUTION   16

#define AD5529R_PRODUCT_ID 0x4140
#define AD5529R_CHIP_TYPE  0x08

/* SPI instruction format */
#define AD5529R_INST_READ    BIT(15)
#define AD5529R_INST_WRITE   0
#define AD5529R_DEVICE_ID(x) (((x) & 0x7) << 12)
#define AD5529R_ADDR_MASK    0x0FFF

/* Register addresses */
#define AD5529R_REG_INTERFACE_CONFIG_A 0x000
#define AD5529R_REG_INTERFACE_CONFIG_B 0x001
#define AD5529R_REG_DEVICE_CONFIG      0x002
#define AD5529R_REG_CHIP_TYPE          0x003
#define AD5529R_REG_PRODUCT_ID_L       0x004
#define AD5529R_REG_PRODUCT_ID_H       0x005
#define AD5529R_REG_CHIP_GRADE         0x006
#define AD5529R_REG_SCRATCH_PAD        0x00A
#define AD5529R_REG_SPI_REVISION       0x00B
#define AD5529R_REG_VENDOR_L           0x00C
#define AD5529R_REG_VENDOR_H           0x00D
#define AD5529R_REG_STREAM_MODE        0x00E

/* Output operating mode register */
#define AD5529R_REG_OUT_OPERATING_MODE 0x03A

/* Output range registers (0x03C - 0x05A, one per channel) */
#define AD5529R_REG_OUT_RANGE_CH(n) (0x03C + ((n) * 2))

/* DAC input registers - used to update DAC value (0x148 - 0x166, one per channel) */
#define AD5529R_REG_DAC_INPUT_A_CH(n) (0x148 + ((n) * 2))

/* DAC output registers - actual output value (0x1A8 - 0x1C6) */
#define AD5529R_REG_DAC_OUTPUT_A_CH(n) (0x1A8 + ((n) * 2))

/* Interface config A bits */
#define AD5529R_SW_RESET_KEY   0x81
#define AD5529R_SDO_ACTIVE     BIT(4)
#define AD5529R_ADDR_ASCENSION BIT(5)

/* Output voltage range values */
enum ad5529r_output_range {
	AD5529R_RANGE_0_5V = 0x0,
	AD5529R_RANGE_0_10V = 0x1,
	AD5529R_RANGE_0_20V = 0x2,
	AD5529R_RANGE_0_40V = 0x3,
	AD5529R_RANGE_PM_5V = 0x5,
	AD5529R_RANGE_PM_10V = 0x6,
	AD5529R_RANGE_PM_15V = 0x7,
	AD5529R_RANGE_PM_20V = 0x8,
};

/* Timing constants */
#define AD5529R_RESET_PULSE_US 10
#define AD5529R_RESET_WAIT_US  100

struct ad5529r_config {
	struct spi_dt_spec bus;
	const struct gpio_dt_spec gpio_reset;
	const struct gpio_dt_spec gpio_ldac;
	const struct gpio_dt_spec gpio_clear;
	uint8_t device_id;
	uint8_t output_range;
};

struct ad5529r_data {
	uint16_t channel_setup_mask;
};

static int ad5529r_write_reg(const struct device *dev, uint16_t addr, uint16_t value)
{
	const struct ad5529r_config *config = dev->config;
	uint8_t buffer_tx[4];
	uint16_t instruction;
	int ret;

	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	/* Build instruction word: write bit + device ID + address */
	instruction = AD5529R_INST_WRITE | AD5529R_DEVICE_ID(config->device_id) |
		      (addr & AD5529R_ADDR_MASK);

	sys_put_be16(instruction, buffer_tx);
	sys_put_be16(value, buffer_tx + 2);

	LOG_DBG("write reg 0x%03X = 0x%04X (inst=0x%04X)", addr, value, instruction);

	ret = spi_write_dt(&config->bus, &tx);
	if (ret != 0) {
		LOG_ERR("SPI write failed: %d", ret);
		return ret;
	}

	return 0;
}

static int ad5529r_read_reg(const struct device *dev, uint16_t addr, uint16_t *value)
{
	const struct ad5529r_config *config = dev->config;
	uint8_t buffer_tx[4] = {0};
	uint8_t buffer_rx[4] = {0};
	uint16_t instruction;
	int ret;

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

	/* Build instruction word: read bit + device ID + address */
	instruction = AD5529R_INST_READ | AD5529R_DEVICE_ID(config->device_id) |
		      (addr & AD5529R_ADDR_MASK);

	sys_put_be16(instruction, buffer_tx);

	ret = spi_transceive_dt(&config->bus, &tx, &rx);
	if (ret != 0) {
		LOG_ERR("SPI transceive failed: %d", ret);
		return ret;
	}

	*value = sys_get_be16(buffer_rx + 2);

	LOG_DBG("read reg 0x%03X = 0x%04X", addr, *value);

	return 0;
}

static int ad5529r_software_reset(const struct device *dev)
{
	int ret;

	LOG_DBG("performing software reset");

	ret = ad5529r_write_reg(dev, AD5529R_REG_INTERFACE_CONFIG_A, AD5529R_SW_RESET_KEY);
	if (ret != 0) {
		LOG_ERR("software reset failed: %d", ret);
		return ret;
	}

	k_busy_wait(AD5529R_RESET_WAIT_US);

	return 0;
}

static int ad5529r_channel_setup(const struct device *dev,
				 const struct dac_channel_cfg *channel_cfg)
{
	const struct ad5529r_config *config = dev->config;
	struct ad5529r_data *data = dev->data;
	int ret;

	if (channel_cfg->channel_id >= AD5529R_NUM_CHANNELS) {
		LOG_ERR("invalid channel %u", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->resolution != AD5529R_RESOLUTION) {
		LOG_ERR("invalid resolution %u (expected %u)", channel_cfg->resolution,
			AD5529R_RESOLUTION);
		return -EINVAL;
	}

	if (channel_cfg->internal) {
		LOG_ERR("internal channels not supported");
		return -ENOTSUP;
	}

	/* Configure output range for this channel */
	ret = ad5529r_write_reg(dev, AD5529R_REG_OUT_RANGE_CH(channel_cfg->channel_id),
				config->output_range);
	if (ret != 0) {
		LOG_ERR("failed to set output range for channel %u: %d", channel_cfg->channel_id,
			ret);
		return ret;
	}

	data->channel_setup_mask |= BIT(channel_cfg->channel_id);

	LOG_DBG("channel %u setup complete (range=0x%02X)", channel_cfg->channel_id,
		config->output_range);

	return 0;
}

static int ad5529r_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	struct ad5529r_data *data = dev->data;
	int ret;

	if (channel >= AD5529R_NUM_CHANNELS) {
		LOG_ERR("invalid channel %u", channel);
		return -EINVAL;
	}

	if (!(data->channel_setup_mask & BIT(channel))) {
		LOG_ERR("channel %u not configured", channel);
		return -EINVAL;
	}

	if (value > UINT16_MAX) {
		LOG_ERR("invalid value %u (max %u)", value, UINT16_MAX);
		return -EINVAL;
	}

	ret = ad5529r_write_reg(dev, AD5529R_REG_DAC_INPUT_A_CH(channel), (uint16_t)value);
	if (ret != 0) {
		LOG_ERR("failed to write value to channel %u: %d", channel, ret);
		return ret;
	}

	LOG_DBG("channel %u set to %u", channel, value);

	return 0;
}

static int ad5529r_init(const struct device *dev)
{
	const struct ad5529r_config *config = dev->config;
	uint16_t product_id_low;
	uint16_t product_id;
	uint16_t chip_type;
	int ret;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	/* Configure reset GPIO if available */
	if (config->gpio_reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_reset)) {
			LOG_ERR("reset GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("failed to configure reset GPIO: %d", ret);
			return ret;
		}

		/* Perform hardware reset */
		LOG_DBG("performing hardware reset");
		gpio_pin_set_dt(&config->gpio_reset, 1);
		k_busy_wait(AD5529R_RESET_PULSE_US);
		gpio_pin_set_dt(&config->gpio_reset, 0);
		k_busy_wait(AD5529R_RESET_WAIT_US);
	} else {
		/* Perform software reset */
		ret = ad5529r_software_reset(dev);
		if (ret != 0) {
			return ret;
		}
	}

	/* Configure LDAC GPIO if available (active low) */
	if (config->gpio_ldac.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_ldac)) {
			LOG_ERR("LDAC GPIO not ready");
			return -ENODEV;
		}

		/* Keep LDAC low so DAC updates immediately on register write */
		ret = gpio_pin_configure_dt(&config->gpio_ldac, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			LOG_ERR("failed to configure LDAC GPIO: %d", ret);
			return ret;
		}
	}

	/* Configure CLEAR GPIO if available */
	if (config->gpio_clear.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_clear)) {
			LOG_ERR("CLEAR GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->gpio_clear, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("failed to configure CLEAR GPIO: %d", ret);
			return ret;
		}
	}

	/* Read product ID to verify communication */
	ret = ad5529r_read_reg(dev, AD5529R_REG_PRODUCT_ID_L, &product_id_low);
	if (ret != 0) {
		LOG_ERR("failed to read product ID: %d", ret);
		return ret;
	}

	ret = ad5529r_read_reg(dev, AD5529R_REG_PRODUCT_ID_H, &product_id);
	if (ret != 0) {
		LOG_ERR("failed to read product ID: %d", ret);
		return ret;
	}

	product_id <<= 8;
	product_id |= product_id_low;

	if (product_id != AD5529R_PRODUCT_ID) {
		LOG_ERR("unexpected product ID %X", product_id);
		return -EINVAL;
	}

	ret = ad5529r_read_reg(dev, AD5529R_REG_CHIP_TYPE, &chip_type);
	if (ret != 0) {
		LOG_ERR("failed to read chip type: %d", ret);
		return ret;
	}

	if (chip_type != AD5529R_CHIP_TYPE) {
		LOG_ERR("unexpected chip type %X", chip_type);
		return -EINVAL;
	}

	LOG_INF("AD5529R initialized (product_id=0x%04X, device_id=%u)", product_id,
		config->device_id);

	return 0;
}

static DEVICE_API(dac, ad5529r_driver_api) = {
	.channel_setup = ad5529r_channel_setup,
	.write_value = ad5529r_write_value,
};

BUILD_ASSERT(CONFIG_DAC_AD5529R_INIT_PRIORITY > CONFIG_SPI_INIT_PRIORITY,
	     "CONFIG_DAC_AD5529R_INIT_PRIORITY must be higher than CONFIG_SPI_INIT_PRIORITY");

#define AD5529R_OUTPUT_RANGE(inst)                                                                 \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst, output_range, range_0_5v),              \
		    (AD5529R_RANGE_0_5V),                                                \
		    (COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst, output_range, range_0_10v),\
		    (AD5529R_RANGE_0_10V),                                               \
		    (COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst, output_range, range_0_20v),\
		    (AD5529R_RANGE_0_20V),                                               \
		    (COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst, output_range, range_0_40v),\
		    (AD5529R_RANGE_0_40V),                                               \
		    (COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst, output_range, range_pm_5v),\
		    (AD5529R_RANGE_PM_5V),                                               \
		    (COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst, output_range, range_pm_10v),\
		    (AD5529R_RANGE_PM_10V),                                              \
		    (COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst, output_range, range_pm_15v),\
		    (AD5529R_RANGE_PM_15V),                                              \
		    (COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst, output_range, range_pm_20v),\
		    (AD5529R_RANGE_PM_20V),                                              \
		    (AD5529R_RANGE_0_10V))))))))))))))))

#define AD5529R_INST_DEFINE(inst)                                                                  \
	static struct ad5529r_data data_##inst;                                                    \
	static const struct ad5529r_config config_##inst = {                                       \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),         \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.gpio_ldac = GPIO_DT_SPEC_INST_GET_OR(inst, ldac_gpios, {0}),                      \
		.gpio_clear = GPIO_DT_SPEC_INST_GET_OR(inst, clear_gpios, {0}),                    \
		.device_id = DT_INST_PROP_OR(inst, device_id, 0),                                  \
		.output_range = AD5529R_OUTPUT_RANGE(inst),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ad5529r_init, NULL, &data_##inst, &config_##inst, POST_KERNEL, \
			      CONFIG_DAC_AD5529R_INIT_PRIORITY, &ad5529r_driver_api);

#define DT_DRV_COMPAT adi_ad5529r
DT_INST_FOREACH_STATUS_OKAY(AD5529R_INST_DEFINE)
