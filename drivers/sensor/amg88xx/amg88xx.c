/*
 * Copyright (c) 2017-2019 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "amg88xx.h"

LOG_MODULE_REGISTER(AMG88XX, CONFIG_SENSOR_LOG_LEVEL);

static int amg88xx_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct amg88xx_data *drv_data = dev->driver_data;
	const struct amg88xx_config *config = dev->config->config_info;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (i2c_burst_read(drv_data->i2c, config->i2c_address,
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
	const struct amg88xx_config *config = dev->config->config_info;
	u8_t tmp;

	if (i2c_reg_read_byte(drv_data->i2c, config->i2c_address,
			      AMG88XX_PCLT, &tmp)) {
		LOG_ERR("Failed to read Power mode");
		return -EIO;
	}

	LOG_DBG("Power mode 0x%02x", tmp);
	if (tmp != AMG88XX_PCLT_NORMAL_MODE) {
		if (i2c_reg_write_byte(drv_data->i2c,
				       config->i2c_address,
				       AMG88XX_PCLT,
				       AMG88XX_PCLT_NORMAL_MODE)) {
			return -EIO;
		}
		k_busy_wait(AMG88XX_WAIT_MODE_CHANGE_US);
	}

	if (i2c_reg_write_byte(drv_data->i2c, config->i2c_address,
			       AMG88XX_RST, AMG88XX_RST_INITIAL_RST)) {
		return -EIO;
	}

	k_busy_wait(AMG88XX_WAIT_INITIAL_RESET_US);

	if (i2c_reg_write_byte(drv_data->i2c, config->i2c_address,
			       AMG88XX_FPSC, AMG88XX_FPSC_10FPS)) {
		return -EIO;
	}

	LOG_DBG("");

	return 0;
}

int amg88xx_init(struct device *dev)
{
	struct amg88xx_data *drv_data = dev->driver_data;
	const struct amg88xx_config *config = dev->config->config_info;

	drv_data->i2c = device_get_binding(config->i2c_name);
	if (drv_data->i2c == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			config->i2c_name);
		return -EINVAL;
	}

	if (amg88xx_init_device(dev) < 0) {
		LOG_ERR("Failed to initialize device!");
		return -EIO;
	}


#ifdef CONFIG_AMG88XX_TRIGGER
	if (amg88xx_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt!");
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

static const struct amg88xx_config amg88xx_config = {
	.i2c_name = DT_INST_0_PANASONIC_AMG88XX_BUS_NAME,
	.i2c_address = DT_INST_0_PANASONIC_AMG88XX_BASE_ADDRESS,
#ifdef CONFIG_AMG88XX_TRIGGER
	.gpio_name = DT_INST_0_PANASONIC_AMG88XX_INT_GPIOS_CONTROLLER,
	.gpio_pin = DT_INST_0_PANASONIC_AMG88XX_INT_GPIOS_PIN,
#endif
};

DEVICE_AND_API_INIT(amg88xx, DT_INST_0_PANASONIC_AMG88XX_LABEL, amg88xx_init,
		    &amg88xx_driver, &amg88xx_config,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &amg88xx_driver_api);
