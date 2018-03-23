/*
 * Copyright (c) 2017 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <device.h>
#include <i2c.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>
#include <misc/__assert.h>

#include "amg88xx.h"

static int amg88xx_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct amg88xx_data *drv_data = dev->driver_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (i2c_burst_read(drv_data->i2c, CONFIG_AMG88XX_I2C_ADDR,
			   AMG88XX_OUTPUT_BASE,
			   (u8_t *)drv_data->sample,
			   sizeof(drv_data->sample))) {
		return -EIO;
	}

	return 0;
}

static int amg88xx_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct amg88xx_data *drv_data = dev->driver_data;
	size_t len = ARRAY_SIZE(drv_data->sample);

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	for (size_t idx = 0; idx < len; idx++) {
		/* fix negative values */
		if (drv_data->sample[idx] & (1 << 11)) {
			drv_data->sample[idx] |= 0xF000;
		}
		val[idx].val1 = (((s32_t)drv_data->sample[idx]) *
				  AMG88XX_TREG_LSB_SCALING) / 1000000;
		val[idx].val2 = (((s32_t)drv_data->sample[idx]) *
				  AMG88XX_TREG_LSB_SCALING) % 1000000;
	}

	return 0;
}

static int amg88xx_init_device(struct device *dev)
{
	struct amg88xx_data *drv_data = dev->driver_data;
	u8_t tmp;

	if (amg88xx_reg_read(drv_data, AMG88XX_PCLT, &tmp)) {
		SYS_LOG_ERR("Failed to read Power mode");
		return -EIO;
	}

	SYS_LOG_DBG("Power mode 0x%02x", tmp);
	if (tmp != AMG88XX_PCLT_NORMAL_MODE) {
		if (amg88xx_reg_write(drv_data, AMG88XX_PCLT,
				       AMG88XX_PCLT_NORMAL_MODE)) {
			return -EIO;
		}
		k_busy_wait(AMG88XX_WAIT_MODE_CHANGE_US);
	}

	if (amg88xx_reg_write(drv_data, AMG88XX_RST,
			      AMG88XX_RST_INITIAL_RST)) {
		return -EIO;
	}

	k_busy_wait(AMG88XX_WAIT_INITIAL_RESET_US);

	if (amg88xx_reg_write(drv_data, AMG88XX_FPSC, AMG88XX_FPSC_10FPS)) {
		return -EIO;
	}

	SYS_LOG_DBG("");

	return 0;
}

int amg88xx_init(struct device *dev)
{
	struct amg88xx_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(CONFIG_AMG88XX_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device!",
			    CONFIG_AMG88XX_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	if (amg88xx_init_device(dev) < 0) {
		SYS_LOG_ERR("Failed to initialize device!");
		return -EIO;
	}


#ifdef CONFIG_AMG88XX_TRIGGER
	if (amg88xx_init_interrupt(dev) < 0) {
		SYS_LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	return 0;
}

struct amg88xx_data amg88xx_driver;

static const struct sensor_driver_api amg88xx_driver_api = {
#ifdef CONFIG_AMG88XX_TRIGGER
	.attr_set = amg88xx_attr_set,
	.trigger_set = amg88xx_trigger_set,
#endif
	.sample_fetch = amg88xx_sample_fetch,
	.channel_get = amg88xx_channel_get,
};


DEVICE_AND_API_INIT(amg88xx, CONFIG_AMG88XX_NAME, amg88xx_init,
		    &amg88xx_driver, NULL,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &amg88xx_driver_api);
