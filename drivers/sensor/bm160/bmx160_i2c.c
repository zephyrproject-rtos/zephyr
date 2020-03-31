/* Bosch BMX160 inertial measurement unit driver
 *
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BMX160-DS000.pdf
 */

#include <init.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include "bm160.h"

#define DT_DRV_COMPAT		bosch_bmx160

LOG_MODULE_REGISTER(BMX160, CONFIG_SENSOR_LOG_LEVEL);

int bmx160_init(const  struct device *dev)
{
	struct bmi160_device_data *bmx160 = dev->data;
	int ret;

	bmx160->i2c = device_get_binding(bmx160->bus_name);
	if (!bmx160->i2c) {
		LOG_ERR("I2C master controller not found: %s.",
				bmx160->bus_name);
		return -EINVAL;
	}

	ret =  bm160_device_init(dev);
	if (ret) {
		LOG_ERR("device init fail: %d", ret);
	}

	return ret;
}

static const struct sensor_driver_api bmx160_api = {
	.attr_set = bmi160_attr_set,
#ifdef CONFIG_BMI160_TRIGGER
	.trigger_set = bmi160_trigger_set,
#endif
	.sample_fetch = bmi160_sample_fetch,
	.channel_get = bmi160_channel_get,
};

#define BMX160_INIT(idx)						\
									\
static struct bmi160_device_data bmx160_data_##idx = {			\
	.i2c_addr = DT_INST_REG_ADDR(idx),				\
	.bus_name = DT_INST_BUS_LABEL(idx),				\
};									\
									\
static const struct bmi160_device_config bmx160_config_##idx = {	\
	.chipid = BMX160_CHIP_ID,					\
	.gpio_port = DT_INST_GPIO_LABEL(idx, int_gpios),		\
	.int_pin = DT_INST_GPIO_PIN(idx, int_gpios),			\
	.int_flags = DT_INST_GPIO_FLAGS(idx, int_gpios),		\
};									\
									\
DEVICE_AND_API_INIT(bmx160_##idx, DT_INST_LABEL(idx), bmx160_init,	\
		    &bmx160_data_##idx, &bmx160_config_##idx,		\
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
		    &bmx160_api);

DT_INST_FOREACH_STATUS_OKAY(BMX160_INIT)
