/* bno055.c - Driver for Bosch BNO055 orientation sensor */

/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/sensor.h>
#include <init.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>

#include <logging/log.h>

#include "bno055.h"

LOG_MODULE_REGISTER(BNO055, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "BNO055 driver enabled without any devices"
#endif

struct bno055_data {
	uint8_t chip_id;
	uint8_t page_id;

	struct bno055_euler_t euler_reg_hpr;
	struct bno055_euler_double_t euler_double_hpr;

};

struct bno055_config {
	union bno055_bus bus;
	const struct bno055_bus_io *bus_io;
};

static inline struct bno055_data *to_data(const struct device *dev)
{
	return dev->data;
}

static inline int bno055_bus_check(const struct device *dev)
{
	const struct bno055_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static inline int bno055_reg_read(const struct device *dev,
				  uint8_t start, uint8_t *buf, int size)
{
	const struct bno055_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, start, buf, size);
}

static inline int bno055_reg_write(const struct device *dev, uint8_t reg,
				   uint8_t val)
{
	const struct bno055_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, val);
}

static int bno055_read_euler_hrp(const struct device *dev)
{
	/* Array holding the Euler hrp value
	 * data_u8[BNO055_SENSOR_DATA_EULER_HRP_H_LSB] - h->LSB
	 * data_u8[BNO055_SENSOR_DATA_EULER_HRP_H_MSB] - h->MSB
	 * data_u8[BNO055_SENSOR_DATA_EULER_HRP_R_LSB] - r->MSB
	 * data_u8[BNO055_SENSOR_DATA_EULER_HRP_R_MSB] - r->MSB
	 * data_u8[BNO055_SENSOR_DATA_EULER_HRP_P_LSB] - p->MSB
	 * data_u8[BNO055_SENSOR_DATA_EULER_HRP_P_MSB] - p->MSB
	 */
	struct bno055_data *data = to_data(dev);
	uint8_t data_u8[BNO055_EULER_HRP_DATA_SIZE] = { 0, 0, 0, 0, 0, 0 };
	int ret = BNO055_ERROR;

	if (data->page_id != BNO055_PAGE_ZERO) {
		ret = bno055_reg_write(dev, BNO055_PAGE_ID_ADDR, BNO055_PAGE_ZERO);
	}

	if ((ret == BNO055_SUCCESS) || (data->page_id == BNO055_PAGE_ZERO)) {
		ret = bno055_reg_read(dev, BNO055_EULER_H_LSB_ADDR,
				      data_u8, BNO055_EULER_HRP_DATA_SIZE);

		/* Data h*/
		data_u8[BNO055_SENSOR_DATA_EULER_HRP_H_LSB] = BNO055_GET_BITSLICE(
			data_u8[BNO055_SENSOR_DATA_EULER_HRP_H_LSB],
			BNO055_EULER_H_LSB_VALUEH);
		data_u8[BNO055_SENSOR_DATA_EULER_HRP_H_MSB] = BNO055_GET_BITSLICE(
			data_u8[BNO055_SENSOR_DATA_EULER_HRP_H_MSB],
			BNO055_EULER_H_MSB_VALUEH);
		data->euler_reg_hpr.h =
			((((int16_t)((int8_t)data_u8[BNO055_SENSOR_DATA_EULER_HRP_H_MSB]))
				<< BNO055_SHIFT_EIGHT_BITS) |
			 (data_u8[BNO055_SENSOR_DATA_EULER_HRP_H_LSB]));

		/* Data r*/
		data_u8[BNO055_SENSOR_DATA_EULER_HRP_R_LSB] = BNO055_GET_BITSLICE(
			data_u8[BNO055_SENSOR_DATA_EULER_HRP_R_LSB],
			BNO055_EULER_R_LSB_VALUER);
		data_u8[BNO055_SENSOR_DATA_EULER_HRP_R_MSB] = BNO055_GET_BITSLICE(
			data_u8[BNO055_SENSOR_DATA_EULER_HRP_R_MSB],
			BNO055_EULER_R_MSB_VALUER);
		data->euler_reg_hpr.r =
			((((int16_t)((int8_t)data_u8[BNO055_SENSOR_DATA_EULER_HRP_R_MSB]))
				<< BNO055_SHIFT_EIGHT_BITS) |
			 (data_u8[BNO055_SENSOR_DATA_EULER_HRP_R_LSB]));

		/* Data p*/
		data_u8[BNO055_SENSOR_DATA_EULER_HRP_P_LSB] = BNO055_GET_BITSLICE(
			data_u8[BNO055_SENSOR_DATA_EULER_HRP_P_LSB],
			BNO055_EULER_P_LSB_VALUEP);
		data_u8[BNO055_SENSOR_DATA_EULER_HRP_P_MSB] = BNO055_GET_BITSLICE(
			data_u8[BNO055_SENSOR_DATA_EULER_HRP_P_MSB],
			BNO055_EULER_P_MSB_VALUEP);
		data->euler_reg_hpr.p =
			((((int16_t)((int8_t)data_u8[BNO055_SENSOR_DATA_EULER_HRP_P_MSB]))
				<< BNO055_SHIFT_EIGHT_BITS) |
			 (data_u8[BNO055_SENSOR_DATA_EULER_HRP_P_LSB]));
	}

	return ret;
}

static int bno055_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	int ret = 0;

	switch (chan) {
	case SENSOR_CHAN_YRP:
		ret = bno055_read_euler_hrp(dev);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static int bno055_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bno055_data *data = to_data(dev);

	switch (chan) {
	case SENSOR_CHAN_YAW:
		data->euler_double_hpr.h = (double)(data->euler_reg_hpr.h / BNO055_EULER_DIV_DEG);
		sensor_value_from_double(val, data->euler_double_hpr.h);
		break;
	case SENSOR_CHAN_PITCH:
		data->euler_double_hpr.p = (double)(data->euler_reg_hpr.p / BNO055_EULER_DIV_DEG);
		sensor_value_from_double(val, data->euler_double_hpr.p);
		break;
	case SENSOR_CHAN_ROLL:
		data->euler_double_hpr.r = (double)(data->euler_reg_hpr.r / BNO055_EULER_DIV_DEG);
		sensor_value_from_double(val, data->euler_double_hpr.r);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api bno055_api_funcs = {
	.sample_fetch = bno055_sample_fetch,
	.channel_get = bno055_channel_get,
};

static int bno055_chip_init(const struct device *dev)
{
	struct bno055_data *data = to_data(dev);
	int err;

	err = bno055_bus_check(dev);
	if (err < 0) {
		LOG_DBG("bus check failed: %d", err);
		return err;
	}

	/* selects page zero */
	err += bno055_reg_write(dev, BNO055_PAGE_ID_ADDR, BNO055_PAGE_ZERO);

	/* reads out chip id */
	err += bno055_reg_read(dev, BNO055_CHIP_ID_ADDR,
			       &data->chip_id, BNO055_GEN_READ_WRITE_LENGTH);

	if (data->chip_id == BNO055_ID) {
		LOG_DBG("ID OK");
	} else {
		LOG_DBG("bad chip id 0x%x", data->chip_id);
		return -ENOTSUP;
	}

	/* Reads the page id */
	err += bno055_reg_read(dev, BNO055_PAGE_ID_ADDR,
			       &data->page_id, BNO055_GEN_READ_WRITE_LENGTH);

	/* sets operation mode */
	err += bno055_reg_write(dev, BNO055_OPR_MODE_ADDR, BNO055_OPERATION_MODE);

	k_sleep(K_MSEC(600));

	if (err != 0) {
		LOG_DBG("ID read failed: %d", err);
		return err;
	}

	LOG_DBG("\"%s\" OK", dev->name);
	return 0;
}

/* Initializes a struct bno055_config for an instance on an I2C bus. */
#define BNO055_CONFIG_I2C(inst)			       \
	{					       \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.bus_io = &bno055_bus_io_i2c,	       \
	}

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define BNO055_DEFINE(inst)					 \
	static struct bno055_data bno055_data_##inst;		 \
	static const struct bno055_config bno055_config_##inst = \
		BNO055_CONFIG_I2C(inst);			 \
	DEVICE_DT_INST_DEFINE(inst,				 \
			      bno055_chip_init,			 \
			      bno055_pm_ctrl,			 \
			      &bno055_data_##inst,		 \
			      &bno055_config_##inst,		 \
			      POST_KERNEL,			 \
			      CONFIG_SENSOR_INIT_PRIORITY,	 \
			      &bno055_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(BNO055_DEFINE)
