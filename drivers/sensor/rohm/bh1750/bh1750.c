/*
 * Copyright (c) 2022, Michal Morsisko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bh1750

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(BH1750, CONFIG_SENSOR_LOG_LEVEL);

#define BH1750_CONTINUOUS_HIGH_RES_MODE                 0x10
#define BH1750_CONTINUOUS_HIGH_RES_MODE_2               0x11
#define BH1750_CONTINUOUS_LOW_RES_MODE                  0x13
#define BH1750_ONE_TIME_HIGH_RES_MODE                   0x20
#define BH1750_ONE_TIME_HIGH_RES_MODE_2                 0x21
#define BH1750_ONE_TIME_LOW_RES_MODE                    0x23
#define BH1750_MTREG_HIGH_BYTE                          0x40
#define BH1750_MTREG_LOW_BYTE                           0x60
#define BH1750_MTREG_HIGH_BYTE_MASK                     0xE0
#define BH1750_MTREG_LOW_BYTE_MASK                      0x1F

#define BH1750_DEFAULT_MTREG                            69U
#define BH1750_LOW_RES_MODE_MAX_WAIT                    24U
#define BH1750_HIGH_RES_MODE_MAX_WAIT                   180U
#define BH1750_LOW_RES_MODE_TYPICAL_WAIT                16U
#define BH1750_HIGH_RES_MODE_TYPICAL_WAIT               120U

#define BH1750_LOW_RES_DTS_ENUM                         0U
#define BH1750_HIGH_RES_DTS_ENUM                        1U
#define BH1750_HIGH_RES_2_DTS_ENUM                      2U

struct bh1750_dev_config {
	struct i2c_dt_spec bus;
	uint8_t resolution;
	uint8_t mtreg;
};

struct bh1750_data {
	uint16_t sample;
};

static int bh1750_opcode_read(const struct device *dev, uint8_t opcode,
			      uint16_t *val)
{
	const struct bh1750_dev_config *cfg = dev->config;
	int rc;

	rc = i2c_burst_read_dt(&cfg->bus, opcode, (uint8_t *)val, 2);
	if (rc < 0) {
		return rc;
	}

	*val = sys_be16_to_cpu(*val);
	return 0;
}

static int bh1750_opcode_write(const struct device *dev, uint8_t opcode)
{
	const struct bh1750_dev_config *cfg = dev->config;

	return i2c_write_dt(&cfg->bus, &opcode, 1);
}

static int bh1750_mtreg_write(const struct device *dev, uint8_t mtreg)
{
	int rc;
	uint8_t high_byte = mtreg & BH1750_MTREG_HIGH_BYTE_MASK;
	uint8_t low_byte = mtreg & BH1750_MTREG_LOW_BYTE_MASK;

	rc = bh1750_opcode_write(dev, BH1750_MTREG_HIGH_BYTE | (high_byte >> 5));
	if (rc < 0) {
		LOG_ERR("%s, Failed to write high byte of mtreg!",
			dev->name);
		return rc;
	}

	rc = bh1750_opcode_write(dev, BH1750_MTREG_LOW_BYTE | low_byte);
	if (rc < 0) {
		LOG_ERR("%s, Failed to write low byte of mtreg!",
			dev->name);
		return rc;
	}

	return 0;
}

static uint8_t bh1750_get_mode_from_dts_device(const struct device *dev)
{
	const struct bh1750_dev_config *cfg = dev->config;

	if (cfg->resolution == BH1750_HIGH_RES_2_DTS_ENUM) {
		return BH1750_ONE_TIME_HIGH_RES_MODE_2;
	} else if (cfg->resolution == BH1750_HIGH_RES_DTS_ENUM) {
		return BH1750_ONE_TIME_HIGH_RES_MODE;
	} else {
		return BH1750_ONE_TIME_LOW_RES_MODE;
	}
}

static int bh1750_get_wait_time_from_dts_device(const struct device *dev)
{
	const struct bh1750_dev_config *cfg = dev->config;

	if (cfg->resolution == BH1750_HIGH_RES_2_DTS_ENUM) {
		return BH1750_HIGH_RES_MODE_MAX_WAIT;
	} else if (cfg->resolution == BH1750_HIGH_RES_DTS_ENUM) {
		return BH1750_HIGH_RES_MODE_MAX_WAIT;
	} else {
		return BH1750_LOW_RES_MODE_MAX_WAIT;
	}
}

static int bh1750_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	const struct bh1750_dev_config *cfg = dev->config;
	struct bh1750_data *drv_data = dev->data;
	const int max_wait = bh1750_get_wait_time_from_dts_device(dev);
	const uint8_t mode = bh1750_get_mode_from_dts_device(dev);
	int rc;
	int wait_time;

	if (chan != SENSOR_CHAN_ALL &&
	    chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	/* Start the measurement */
	rc = bh1750_opcode_write(dev, mode);
	if (rc < 0) {
		LOG_ERR("%s, Failed to start measurement!",
			dev->name);
		return rc;
	}

	/* Calculate measurement time */
	wait_time = (max_wait * (cfg->mtreg * 10000 / BH1750_DEFAULT_MTREG)) / 10000;

	/* Wait for the measurement to be stored in the sensor memory */
	k_msleep(wait_time);

	/* Fetch result */
	rc = bh1750_opcode_read(dev, mode, &drv_data->sample);
	if (rc < 0) {
		LOG_ERR("%s: Failed to read measurement result!",
			dev->name);
		return rc;
	}

	return 0;
}

static int bh1750_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct bh1750_dev_config *cfg = dev->config;
	struct bh1750_data *drv_data = dev->data;
	uint32_t tmp;

	if (chan != SENSOR_CHAN_ALL &&
	    chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	/* See datasheet (Technical note 11046EDT01), page 11
	 * https://www.mouser.com/datasheet/2/348/Rohm_11162017_ROHMS34826-1-1279292.pdf
	 * for more information how to convert raw sample to lx
	 */
	tmp = (drv_data->sample * 1000 / 12) * (BH1750_DEFAULT_MTREG * 100 / cfg->mtreg);

	if (cfg->resolution == BH1750_HIGH_RES_2_DTS_ENUM) {
		tmp /= 2;
	}

	val->val1 = tmp / 10000;
	val->val2 = (tmp % 10000) * 100;

	return 0;
}

static DEVICE_API(sensor, bh1750_driver_api) = {
	.sample_fetch = bh1750_sample_fetch,
	.channel_get = bh1750_channel_get
};

static int bh1750_init(const struct device *dev)
{
	const struct bh1750_dev_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	if (cfg->mtreg != BH1750_DEFAULT_MTREG) {
		bh1750_mtreg_write(dev, cfg->mtreg);
	}

	return 0;
}

#define DEFINE_BH1750(_num)							       \
	static struct bh1750_data bh1750_data_##_num;				       \
	static const struct bh1750_dev_config bh1750_config_##_num = {		       \
		.bus = I2C_DT_SPEC_INST_GET(_num),				       \
		.mtreg = DT_INST_PROP(_num, mtreg),				       \
		.resolution = DT_INST_PROP(_num, resolution)			       \
	};									       \
	SENSOR_DEVICE_DT_INST_DEFINE(_num, bh1750_init, NULL,				       \
			      &bh1750_data_##_num, &bh1750_config_##_num, POST_KERNEL, \
			      CONFIG_SENSOR_INIT_PRIORITY, &bh1750_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BH1750)
