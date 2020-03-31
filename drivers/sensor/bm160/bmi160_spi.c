/* Bosch BMI160 inertial measurement unit driver
 *
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BMI160-DS000.pdf
 */

#include <init.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include "bm160.h"

#define DT_DRV_COMPAT		bosch_bmi160

LOG_MODULE_REGISTER(BMI160, CONFIG_SENSOR_LOG_LEVEL);

int bmi160_init(const struct device *dev)
{
	struct bmi160_device_data *bmi160 = dev->data;
	int ret;

	bmi160->spi = device_get_binding(bmi160->bus_name);
	if (!bmi160->spi) {
		LOG_DBG("SPI master controller not found: %s.",
			    bmi160->bus_name);
		return -EINVAL;
	}

	bmi160->spi_cfg.operation = SPI_WORD_SET(8);

	ret =  bm160_device_init(dev);
	if (ret) {
		LOG_DBG("device init fail: %d", ret);
	}

	return ret;
}

static const struct sensor_driver_api bmi160_api = {
	.attr_set = bmi160_attr_set,
#ifdef CONFIG_BMI160_TRIGGER
	.trigger_set = bmi160_trigger_set,
#endif
	.sample_fetch = bmi160_sample_fetch,
	.channel_get = bmi160_channel_get,
};

#define BMI160_INIT(idx)						\
									\
static struct bmi160_device_data bmi160_data_##idx = {			\
	.spi_cfg = {							\
	    .slave = DT_INST_REG_ADDR(idx),				\
	    .frequency = DT_INST_PROP(idx, spi_max_frequency),		\
	},								\
	.bus_name = DT_INST_BUS_LABEL(idx),				\
};									\
									\
static const struct bmi160_device_config bmi160_config_##idx = {	\
	.chipid = BMI160_CHIP_ID,					\
	.gpio_port = DT_INST_GPIO_LABEL(idx, int_gpios),		\
	.int_pin = DT_INST_GPIO_PIN(idx, int_gpios),			\
	.int_flags = DT_INST_GPIO_FLAGS(idx, int_gpios),		\
};									\
									\
DEVICE_AND_API_INIT(bmi160_##idx, DT_INST_LABEL(idx), bmi160_init,	\
		    &bmi160_data_##idx, &bmi160_config_##idx,		\
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
		    &bmi160_api);

DT_INST_FOREACH_STATUS_OKAY(BMI160_INIT)
