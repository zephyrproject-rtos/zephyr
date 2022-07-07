/*
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT microchip_mcp4725

#include <zephyr/zephyr.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_mcp4725, CONFIG_DAC_LOG_LEVEL);

/* Information in this file comes from MCP4725 datasheet revision D
 * found at https://ww1.microchip.com/downloads/en/DeviceDoc/22039d.pdf
 */

/* Defines for field values in MCP4725 DAC register */
#define MCP4725_DAC_MAX_VAL			0xFFF

#define MCP4725_FAST_MODE_POWER_DOWN_POS	4U
#define MCP4725_FAST_MODE_DAC_UPPER_VAL_POS	8U
#define MCP4725_FAST_MODE_DAC_UPPER_VAL_MASK	0xF
#define MCP4725_FAST_MODE_DAC_LOWER_VAL_MASK	0xFF

#define MCP4725_READ_RDY_POS			7U
#define MCP4725_READ_RDY_MASK			(0x1 << MCP4725_READ_RDY_POS)

/* After writing eeprom, the MCP4725 can be in a busy state for 25 - 50ms
 * See section 1.0 of MCP4725 datasheet, 'Electrical Characteristics'
 */
#define MCP4725_BUSY_TIMEOUT_MS			60U

struct mcp4725_config {
	const struct device *i2c_dev;
	uint16_t i2c_addr;
};

/* Read mcp4725 and check RDY status bit */
static int mcp4725_wait_until_ready(const struct device *dev, uint16_t i2c_addr)
{
	uint8_t rx_data[5];
	bool mcp4725_ready = false;
	int ret;
	int32_t timeout = k_uptime_get_32() + MCP4725_BUSY_TIMEOUT_MS;

	/* Wait until RDY bit is set or return error if timer exceeds MCP4725_BUSY_TIMEOUT_MS */
	while (!mcp4725_ready) {
		ret = i2c_read(dev, rx_data, sizeof(rx_data), i2c_addr);

		if (ret == 0) {
			mcp4725_ready = rx_data[0] & MCP4725_READ_RDY_MASK;
		} else {
			/* I2C error */
			return ret;
		}

		if (k_uptime_get_32() > timeout) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

/* MCP4725 is a single channel 12 bit DAC */
static int mcp4725_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	if (channel_cfg->channel_id != 0) {
		return -EINVAL;
	}

	if (channel_cfg->resolution != 12) {
		return -ENOTSUP;
	}

	return 0;
}

static int mcp4725_write_value(const struct device *dev, uint8_t channel,
				uint32_t value)
{
	const struct mcp4725_config *config = dev->config;
	uint8_t tx_data[2];
	int ret;

	if (channel != 0) {
		return -EINVAL;
	}

	/* Check value isn't over 12 bits */
	if (value > MCP4725_DAC_MAX_VAL) {
		return -ENOTSUP;
	}

	/* WRITE_MODE_FAST message format (2 bytes):
	 *
	 * ||     15 14     |        13 12        |    11 10 9 8    || 7 6 5 4 3 2 1 0 ||
	 * || Fast mode (0) | Power-down bits (0) | DAC value[11:8] || DAC value[7:0]  ||
	 */
	tx_data[0] = ((value >> MCP4725_FAST_MODE_DAC_UPPER_VAL_POS) &
		MCP4725_FAST_MODE_DAC_UPPER_VAL_MASK);
	tx_data[1] = (value & MCP4725_FAST_MODE_DAC_LOWER_VAL_MASK);
	ret = i2c_write(config->i2c_dev, tx_data, sizeof(tx_data),
		config->i2c_addr);

	return ret;
}

static int dac_mcp4725_init(const struct device *dev)
{
	const struct mcp4725_config *config = dev->config;

	if (!device_is_ready(config->i2c_dev)) {
		LOG_ERR("I2C device not found");
		return -EINVAL;
	}

	/* Check we can read a 'RDY' bit from this device */
	if (mcp4725_wait_until_ready(config->i2c_dev, config->i2c_addr)) {
		return -EBUSY;
	}

	return 0;
}

static const struct dac_driver_api mcp4725_driver_api = {
	.channel_setup = mcp4725_channel_setup,
	.write_value = mcp4725_write_value,
};


#define INST_DT_MCP4725(index)						\
	static const struct mcp4725_config mcp4725_config_##index = {	\
		.i2c_dev = DEVICE_DT_GET(DT_INST_BUS(index)),		\
		.i2c_addr = DT_INST_REG_ADDR(index)			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(index, dac_mcp4725_init,			\
			    NULL,					\
			    NULL,					\
			    &mcp4725_config_##index, POST_KERNEL,	\
			    CONFIG_DAC_INIT_PRIORITY,			\
			    &mcp4725_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INST_DT_MCP4725);
