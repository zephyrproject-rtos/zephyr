/*
 * Copyright (c) 2024 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Texas Instruments DACx0501 series
 *
 * Device driver for the  Texas Instruments DACx0501 series of devices: DAC60501, DAC70501 and
 * DAC80501: Digital to Analog Converters with a single channel output and with 12, 14 and 16
 * bits resolution respectively. Data sheet can be found here:
 * https://www.ti.com/lit/ds/symlink/dac80501.pdf
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(dac_dacx0501, CONFIG_DAC_LOG_LEVEL);

#define DACX0501_REG_DEVICE_ID 0x01U
#define DACX0501_REG_SYNC      0x02U
#define DACX0501_REG_CONFIG    0x03U
#define DACX0501_REG_GAIN      0x04U
#define DACX0501_REG_TRIGGER   0x05U
#define DACX0501_REG_STATUS    0x07U
#define DACX0501_REG_DAC       0x08U

#define DACX0501_MASK_DEVICE_ID_RES      GENMASK(14, 12)
#define DACX0501_MASK_CONFIG_REF_PWDWN   BIT(8)
#define DACX0501_MASK_CONFIG_DAC_PWDWN   BIT(0)
#define DACX0501_MASK_GAIN_BUFF_GAIN     BIT(0)
#define DACX0501_MASK_GAIN_REFDIV_EN     BIT(8)
#define DACX0501_MASK_TRIGGER_SOFT_RESET (BIT(1) | BIT(3))
#define DACX0501_MASK_STATUS_REF_ALM     BIT(0)

/* Specifies the source of the reference voltage. */
enum voltage_reference_source {
	REF_INTERNAL, /* Internal 2.5V voltage reference. */
	REF_EXTERNAL, /* External pin voltage reference. */
};

/* Specifies the reference voltage multiplier. */
enum output_gain {
	VM_MUL2, /* Multiplies by 2. */
	VM_MUL1, /* Multiplies by 1. */
	VM_DIV2, /* Multiplies by 0.5 */
};

struct dacx0501_config {
	struct i2c_dt_spec i2c_spec;
	enum voltage_reference_source voltage_reference;
	enum output_gain output_gain;
};

struct dacx0501_data {
	/* Number of bits in the DAC register: Either 12, 14 or 16. */
	uint8_t resolution;
};

static int dacx0501_reg_read(const struct device *dev, const uint8_t addr, uint16_t *data)
{
	const struct dacx0501_config *config = dev->config;
	uint8_t raw_data[2];
	int status;

	status = i2c_write_read_dt(&config->i2c_spec, &addr, sizeof(addr), raw_data,
				   sizeof(raw_data));
	if (status != 0) {
		return status;
	}

	/* DAC is big endian. */
	*data = sys_get_be16(raw_data);
	return 0;
}

static int dacx0501_reg_write(const struct device *dev, uint8_t addr, uint16_t data)
{
	const struct dacx0501_config *config = dev->config;
	uint8_t write_cmd[3] = {addr};

	/* DAC is big endian. */
	sys_put_be16(data, write_cmd + 1);

	return i2c_write_dt(&config->i2c_spec, write_cmd, sizeof(write_cmd));
}

static int dacx0501_channel_setup(const struct device *dev,
				  const struct dac_channel_cfg *channel_cfg)
{
	struct dacx0501_data *data = dev->data;

	/* DACx0501 series only has a single output channel. */
	if (channel_cfg->channel_id != 0) {
		LOG_ERR("Unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != data->resolution) {
		LOG_ERR("Unsupported resolution %d. Actual: %d", channel_cfg->resolution,
			data->resolution);
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("Internal channels not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int dacx0501_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	struct dacx0501_data *data = dev->data;

	if (channel != 0) {
		LOG_ERR("dacx0501: Unsupported channel %d", channel);
		return -ENOTSUP;
	}

	if (value >= (1 << data->resolution)) {
		LOG_ERR("dacx0501: Value %d out of range", value);
		return -EINVAL;
	}

	value <<= (16 - data->resolution);

	return dacx0501_reg_write(dev, DACX0501_REG_DAC, value);
}

static int dacx0501_init(const struct device *dev)
{
	const struct dacx0501_config *config = dev->config;
	struct dacx0501_data *data = dev->data;
	uint16_t device_id;
	int status;

	if (!i2c_is_ready_dt(&config->i2c_spec)) {
		LOG_ERR("I2C bus %s not ready", config->i2c_spec.bus->name);
		return -ENODEV;
	}

	status = dacx0501_reg_read(dev, DACX0501_REG_DEVICE_ID, &device_id);
	if (status != 0) {
		LOG_ERR("read DEVICE_ID register failed");
		return status;
	}

	/* See DEVICE_ID register RES field in the data sheet. */
	data->resolution = 16 - 2 * FIELD_GET(DACX0501_MASK_DEVICE_ID_RES, device_id);

	status = dacx0501_reg_write(dev, DACX0501_REG_CONFIG,
				    FIELD_PREP(DACX0501_MASK_CONFIG_REF_PWDWN,
					       config->voltage_reference == REF_EXTERNAL));
	if (status != 0) {
		LOG_ERR("write CONFIG register failed");
		return status;
	}

	status = dacx0501_reg_write(
		dev, DACX0501_REG_GAIN,
		FIELD_PREP(DACX0501_MASK_GAIN_REFDIV_EN, config->output_gain == VM_DIV2) |
			FIELD_PREP(DACX0501_MASK_GAIN_BUFF_GAIN, config->output_gain == VM_MUL2));
	if (status != 0) {
		LOG_ERR("GAIN Register update failed");
		return status;
	}

	return 0;
}

static const struct dac_driver_api dacx0501_driver_api = {
	.channel_setup = dacx0501_channel_setup,
	.write_value = dacx0501_write_value,
};

#define DT_DRV_COMPAT ti_dacx0501

#define DACX0501_DEFINE(n)                                                                         \
	static struct dacx0501_data dacx0501_data_##n = {};                                        \
	static const struct dacx0501_config dacx0501_config_##n = {                                \
		.i2c_spec = I2C_DT_SPEC_INST_GET(n),                                               \
		.voltage_reference =                                                               \
			_CONCAT(REF_, DT_STRING_UPPER_TOKEN(DT_DRV_INST(n), voltage_reference)),   \
		.output_gain = _CONCAT(VM_, DT_STRING_UPPER_TOKEN(DT_DRV_INST(n), output_gain)),   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &dacx0501_init, NULL, &dacx0501_data_##n, &dacx0501_config_##n,   \
			      POST_KERNEL, CONFIG_DAC_DACX0501_INIT_PRIORITY,                      \
			      &dacx0501_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DACX0501_DEFINE)
