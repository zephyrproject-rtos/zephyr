/*
 * Copyright (c) 2020 SER Consulting, LLC
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <kernel.h>
#include <drivers/i2c.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include "mcp9600.h"

#define DT_DRV_COMPAT microchip_mcp9600

#define MCP9600_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_REGISTER(MCP9600, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "MCP9600 driver enabled without any devices"
#endif

static int mcp9600_reg_read(const struct device *dev, uint8_t start,
			uint8_t *buf, int size)
{
	struct mcp9600_data *data = dev->data;
	const struct mcp9600_config *cfg = dev->config;

	return i2c_burst_read(data->i2c_master, cfg->i2c_addr,
						start, buf, size);
}

static int mcp9600_sample_fetch(const struct device *dev,
			enum sensor_channel chan)
{
	struct mcp9600_data *data = dev->data;
	uint8_t buf[2];
	int size = 2;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ret = mcp9600_reg_read(dev, MCP9600_REG_TEMP_HOT, buf, size);
	if (ret < 0) {
		return ret;
	}
	data->ttemp = (buf[0] << 8) | buf[1];

	return 0;
}

static int mcp9600_channel_get(const struct device *dev,
			enum sensor_channel chan,
			struct sensor_value *val)
{
	struct mcp9600_data *data = dev->data;
	int32_t temp;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		temp = (int32_t)(int16_t)data->ttemp;
		temp *= 62500;
		val->val1 = temp / 1000000;
		val->val2 = temp % 1000000;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api mcp9600_api_funcs = {
	.sample_fetch = mcp9600_sample_fetch,
	.channel_get = mcp9600_channel_get,
};

static int mcp9600_chip_init(const struct device *dev)
{
	uint8_t buf[2];

	int rc = mcp9600_reg_read(dev, MCP9600_REG_ID_REVISION,
			buf, 2);

	LOG_DBG("mcp9600: id=0x%02x version=0x%02x ret=%d",
			buf[0], buf[1], rc);
	return rc;
}

int mcp9600_init(const struct device *dev)
{
	struct mcp9600_data *data = dev->data;
	const struct mcp9600_config *cfg = dev->config;
	int rc = 0;

	data->i2c_master = device_get_binding(cfg->i2c_bus);
	if (!data->i2c_master) {
		LOG_DBG("mcp9600: i2c master not found: %s", cfg->i2c_bus);
		return -EINVAL;
	}

	rc = mcp9600_chip_init(dev);
	return rc;
}

static struct mcp9600_data mcp9600_data;
static const struct mcp9600_config mcp9600_cfg = {
	.i2c_bus = DT_INST_BUS_LABEL(0),
	.i2c_addr = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, mcp9600_init, device_pm_control_nop,
			&mcp9600_data, &mcp9600_cfg, POST_KERNEL,
			CONFIG_SENSOR_INIT_PRIORITY, &mcp9600_api_funcs);
