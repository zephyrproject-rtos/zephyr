/* sensor_mcp9808.c - Driver for MCP9808 temperature sensor */

/*
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

#include "mcp9808.h"

LOG_MODULE_REGISTER(MCP9808, CONFIG_SENSOR_LOG_LEVEL);

int mcp9808_reg_read(struct mcp9808_data *data, u8_t reg, u16_t *val)
{
	int rc = i2c_write_read(data->i2c_master, data->i2c_slave_addr,
				&reg, sizeof(reg),
				val, sizeof(*val));

	if (rc == 0) {
		*val = sys_be16_to_cpu(*val);
	}

	return rc;
}

static int mcp9808_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct mcp9808_data *data = dev->driver_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	return mcp9808_reg_read(data, MCP9808_REG_TEMP_AMB, &data->reg_val);
}

static int mcp9808_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct mcp9808_data *data = dev->driver_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP);

	val->val1 = (data->reg_val & MCP9808_TEMP_INT_MASK) >>
		     MCP9808_TEMP_INT_SHIFT;
	val->val2 = (data->reg_val & MCP9808_TEMP_FRAC_MASK) * 62500U;

	if (data->reg_val & MCP9808_SIGN_BIT) {
		val->val1 -= 256;
	}

	return 0;
}

static const struct sensor_driver_api mcp9808_api_funcs = {
	.sample_fetch = mcp9808_sample_fetch,
	.channel_get = mcp9808_channel_get,
	.attr_set = mcp9808_attr_set,
	.trigger_set = mcp9808_trigger_set,
};

int mcp9808_init(struct device *dev)
{
	struct mcp9808_data *data = dev->driver_data;

	data->i2c_master =
	       device_get_binding(DT_INST_0_MICROCHIP_MCP9808_BUS_NAME);
	if (!data->i2c_master) {
		LOG_DBG("mcp9808: i2c master not found: %s",
		    DT_INST_0_MICROCHIP_MCP9808_BUS_NAME);
		return -EINVAL;
	}

	data->i2c_slave_addr = DT_INST_0_MICROCHIP_MCP9808_BASE_ADDRESS;

	mcp9808_setup_interrupt(dev);

	return 0;
}

struct mcp9808_data mcp9808_data;

DEVICE_AND_API_INIT(mcp9808, DT_INST_0_MICROCHIP_MCP9808_LABEL, mcp9808_init,
		    &mcp9808_data, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &mcp9808_api_funcs);
