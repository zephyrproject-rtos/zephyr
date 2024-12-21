/* ist8310.c - Driver for Isentek IST8310 Geomagnetic Sensor */

/*
 * Copyright (c) 2023 NXP Semiconductors
 * Copyright (c) 2023 Cognipilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include "ist8310.h"

LOG_MODULE_REGISTER(IST8310, CONFIG_SENSOR_LOG_LEVEL);

static inline int ist8310_bus_check(const struct device *dev)
{
	const struct ist8310_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static inline int ist8310_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	const struct ist8310_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, start, buf, size);
}

static inline int ist8310_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct ist8310_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, val);
}

static int ist8310_sample_fetch(const struct device *dev, enum sensor_channel chan)
{

	struct ist8310_data *drv_data = dev->data;
	uint8_t buff[6];

	if (ist8310_reg_read(dev, IST8310_STATUS_REGISTER1, (uint8_t *)buff, 1) < 0) {
		LOG_ERR("failed to read status register 1");
		return -EIO;
	}

	if ((buff[0] & STAT1_DRDY) == 0) {
		LOG_ERR("Data not ready");
		if (ist8310_reg_write(dev, IST8310_CONTROL_REGISTER1, CTRL1_MODE_SINGLE) < 0) {
			LOG_ERR("failed to set single");
			return -EIO;
		}
		return -EIO;
	}

	if (ist8310_reg_read(dev, IST8310_OUTPUT_VALUE_X_L, (uint8_t *)buff, 6) < 0) {
		LOG_ERR("failed to read mag values");
		return -EIO;
	}

	drv_data->sample_x = (sys_le16_to_cpu(*(uint16_t *)&buff[0]));
	drv_data->sample_y = (sys_le16_to_cpu(*(uint16_t *)&buff[2]));
	drv_data->sample_z = (sys_le16_to_cpu(*(uint16_t *)&buff[4]));

	if (ist8310_reg_write(dev, IST8310_CONTROL_REGISTER1, CTRL1_MODE_SINGLE) < 0) {
		LOG_ERR("failed to set single");
		return -EIO;
	}

	return 0;
}

static int ist8310_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct ist8310_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		sensor_value_from_float(val, drv_data->sample_x * (1.0f / 1320));
		break;
	case SENSOR_CHAN_MAGN_Y:
		sensor_value_from_float(val, drv_data->sample_y * (1.0f / 1320));
		break;
	case SENSOR_CHAN_MAGN_Z:
		sensor_value_from_float(val, drv_data->sample_z * (1.0f / 1320));
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		sensor_value_from_float(val, drv_data->sample_x * (1.0f / 1320));
		sensor_value_from_float(val + 1, drv_data->sample_y * (1.0f / 1320));
		sensor_value_from_float(val + 2, drv_data->sample_z * (1.0f / 1320));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(sensor, ist8310_api_funcs) = {
	.sample_fetch = ist8310_sample_fetch,
	.channel_get = ist8310_channel_get,
};

static int ist8310_init_chip(const struct device *dev)
{
	uint8_t reg;

	/* Read chip ID (can only be read in sleep mode)*/
	if (ist8310_reg_read(dev, IST8310_WHO_AM_I, &reg, 1) < 0) {
		LOG_ERR("failed reading chip id");
		return -EIO;
	}

	if (ist8310_reg_read(dev, IST8310_WHO_AM_I, &reg, 1) < 0) {
		LOG_ERR("failed reading chip id");
		return -EIO;
	}

	if (ist8310_reg_read(dev, IST8310_WHO_AM_I, &reg, 1) < 0) {
		LOG_ERR("failed reading chip id");
		return -EIO;
	}

	if (reg != IST8310_WHO_AM_I_VALUE) {
		LOG_ERR("invalid chip id 0x%x", reg);
		return -EIO;
	}

	if (ist8310_reg_read(dev, IST8310_CONTROL_REGISTER2, &reg, 1) < 0) {
		LOG_ERR("failed reading chip reg2");
		return -EIO;
	}

	reg &= ~CTRL2_SRST;

	if (ist8310_reg_write(dev, IST8310_CONTROL_REGISTER2, reg) < 0) {
		LOG_ERR("failed to set REG2 to %d", reg);
		return -EIO;
	}

	k_sleep(K_MSEC(3));

	if (ist8310_reg_read(dev, IST8310_CONTROL_REGISTER3, &reg, 1) < 0) {
		LOG_ERR("failed reading chip reg3");
		return -EIO;
	}

	reg |= X_16BIT | Y_16BIT | Z_16BIT;

	if (ist8310_reg_write(dev, IST8310_CONTROL_REGISTER3, reg) < 0) {
		LOG_ERR("failed to set REG3 to %d", reg);
		return -EIO;
	}

	if (ist8310_reg_write(dev, IST8310_AVG_REGISTER, XZ_16TIMES_CLEAR | Y_16TIMES_CLEAR) < 0) {
		LOG_ERR("failed to set AVG");
		return -EIO;
	}

	if (ist8310_reg_write(dev, IST8310_AVG_REGISTER, XZ_16TIMES_SET | Y_16TIMES_SET) < 0) {
		LOG_ERR("failed to set AVG");
		return -EIO;
	}

	if (ist8310_reg_write(dev, IST8310_PDCNTL_REGISTER, PULSE_NORMAL) < 0) {
		LOG_ERR("failed to set AVG");
		return -EIO;
	}

	k_sleep(K_MSEC(3));

	if (ist8310_reg_write(dev, IST8310_CONTROL_REGISTER1, CTRL1_MODE_SINGLE) < 0) {
		LOG_ERR("failed to set single");
		return -EIO;
	}

	return 0;
}

static int ist8310_init(const struct device *dev)
{
	int err = 0;

	err = ist8310_bus_check(dev);
	if (err < 0) {
		LOG_DBG("bus check failed: %d", err);
		return err;
	}

	if (ist8310_init_chip(dev) < 0) {
		LOG_ERR("failed to initialize chip");
		return -EIO;
	}

	return 0;
}

/* Initializes a struct ist8310_config for an instance on an I2C bus. */
#define IST8310_CONFIG_I2C(inst)                                                                   \
	.bus.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_io = &ist8310_bus_io_i2c,

#define IST8310_BUS_CFG(inst)                                                                      \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c), (IST8310_CONFIG_I2C(inst)),                         \
		    (IST8310_CONFIG_SPI(inst)))

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define IST8310_DEFINE(inst)                                                                       \
	static struct ist8310_data ist8310_data_##inst;                                            \
	static const struct ist8310_config ist8310_config_##inst = {IST8310_BUS_CFG(inst)};        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ist8310_init, NULL, &ist8310_data_##inst,               \
				     &ist8310_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &ist8310_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(IST8310_DEFINE)
