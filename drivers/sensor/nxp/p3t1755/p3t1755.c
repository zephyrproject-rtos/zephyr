/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_p3t1755

#include "p3t1755.h"
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(P3T1755, CONFIG_SENSOR_LOG_LEVEL);


#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
static int p3t1755_i3c_read_reg(const struct device *dev, uint8_t reg, uint8_t *value, uint8_t len)
{
	struct p3t1755_data *data = dev->data;

	return i3c_burst_read(data->i3c_dev, reg, value, len);
}

static int p3t1755_i3c_write_reg(const struct device *dev, uint8_t reg, uint8_t *byte, uint8_t len)
{
	struct p3t1755_data *data = dev->data;

	return i3c_burst_write(data->i3c_dev, reg, byte, len);
}
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
int p3t1755_i2c_read_reg(const struct device *dev, uint8_t reg, uint8_t *value, uint8_t len)
{
	const struct p3t1755_config *config = dev->config;

	return i2c_burst_read_dt(&config->bus_cfg.i2c, reg, value, len);
}

int p3t1755_i2c_write_reg(const struct device *dev, uint8_t reg, uint8_t *byte, uint8_t len)
{
	const struct p3t1755_config *config = dev->config;

	return i2c_burst_write_dt(&config->bus_cfg.i2c, reg, byte, len);
}
#endif

static int p3t1755_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct p3t1755_config *config = dev->config;
	struct p3t1755_data *data = dev->data;
	uint8_t raw_temp[2];

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Invalid channel provided");
		return -ENOTSUP;
	}

	if (config->oneshot_mode) {
		data->config_reg |= P3T1755_CONFIG_REG_OS;
		config->ops.write(dev, P3T1755_CONFIG_REG, &data->config_reg, 1);
		/* Maximum one-shot conversion time per specification is 12ms */
		k_sleep(K_MSEC(12));
	}

	config->ops.read(dev, P3T1755_TEMPERATURE_REG, raw_temp, 2);

	/* Byte 1 contains the MSByte and Byte 2 contains the LSByte, we need to swap the 2 bytes.
	 * The 4 least significant bits of the LSByte are zero and should be ignored.
	 */
	data->sample = (((uint16_t)raw_temp[0] << 8U) | (uint16_t)raw_temp[1]) >>
		       P3T1755_TEMPERATURE_REG_SHIFT;

	return 0;
}

/* Decode a register temperature value to a signed temperature */
static inline int p3t1755_convert_to_signed(uint16_t reg)
{
	int rv = reg & P3T1755_TEMPERATURE_ABS_MASK;

	if (reg & P3T1755_TEMPERATURE_SIGN_BIT) {
		/* Convert 12-bit 2s complement to signed negative
		 * value.
		 */
		rv = -(1U + (rv ^ P3T1755_TEMPERATURE_ABS_MASK));
	}
	return rv;
}

static int p3t1755_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct p3t1755_data *data = dev->data;
	int32_t raw_val;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	raw_val = p3t1755_convert_to_signed(data->sample);

	/* Temperature data resolution is 0.0625 C, apply a temperature scale */
	raw_val = raw_val * P3T1755_TEMPERATURE_SCALE;

	sensor_value_from_micro(val, raw_val);

	return 0;
}

static int p3t1755_init(const struct device *dev)
{
	const struct p3t1755_config *config = dev->config;
	struct p3t1755_data *data = dev->data;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (config->i3c.bus != NULL) {
		data->i3c_dev = i3c_device_find(config->i3c.bus, &config->i3c.dev_id);
		if (data->i3c_dev == NULL) {
			LOG_ERR("Cannot find I3C device descriptor");
			return -ENODEV;
		}
	}
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	if (config->inst_on_bus == P3T1755_BUS_I2C) {
		if (!i2c_is_ready_dt(&config->bus_cfg.i2c)) {
			LOG_ERR("I2C bus device not ready");
			return -ENODEV;
		}
	}
#endif

	if (config->oneshot_mode) {
		config->ops.read(dev, P3T1755_CONFIG_REG, &data->config_reg, 1);
		/* Operate in shutdown mode. Set the OS bit to start the
		 * one-shot temperature measurement.
		 */
		data->config_reg |= P3T1755_CONFIG_REG_SD;
		config->ops.write(dev, P3T1755_CONFIG_REG, &data->config_reg, 1);
	}

	LOG_DBG("Init complete");

	return 0;
}

static const struct sensor_driver_api p3t1755_driver_api = {
	.sample_fetch = p3t1755_sample_fetch,
	.channel_get = p3t1755_channel_get,
};

/*
 * Instantiation macros used when a device is on an I2C bus.
 */
#define P3T1755_CONFIG_I2C(inst)                                                                   \
	.bus_cfg = {.i2c = I2C_DT_SPEC_INST_GET(inst)},                                            \
	.ops = {                                                                                   \
		.read = p3t1755_i2c_read_reg,                                                      \
		.write = p3t1755_i2c_write_reg,                                                    \
	},                                                                                         \
	.inst_on_bus = P3T1755_BUS_I2C,

/*
 * Instantiation macros used when a device is on an I#C bus.
 */
#define P3T1755_CONFIG_I3C(inst)                                                                   \
	.bus_cfg = {.i3c = &p3t1755_data_##inst.i3c_dev},                                          \
	.ops = {                                                                                   \
		.read = p3t1755_i3c_read_reg,                                                      \
		.write = p3t1755_i3c_write_reg,                                                    \
	},                                                                                         \
	.inst_on_bus = P3T1755_BUS_I3C,                                                            \
	.i3c.bus = DEVICE_DT_GET(DT_INST_BUS(inst)), .i3c.dev_id = I3C_DEVICE_ID_DT_INST(inst),

#define P3T1755_INIT(n)                                                                            \
	static struct p3t1755_data p3t1755_data_##n;                                               \
	static const struct p3t1755_config p3t1755_config_##n = {                                  \
		COND_CODE_1(DT_INST_ON_BUS(n, i3c),                                                \
				(P3T1755_CONFIG_I3C(n)),                                           \
				(P3T1755_CONFIG_I2C(n)))                                           \
		.oneshot_mode = DT_INST_PROP(n, oneshot_mode),                                     \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, p3t1755_init, NULL, &p3t1755_data_##n,                     \
				     &p3t1755_config_##n, POST_KERNEL,                             \
				     CONFIG_SENSOR_INIT_PRIORITY, &p3t1755_driver_api);

DT_INST_FOREACH_STATUS_OKAY(P3T1755_INIT)
