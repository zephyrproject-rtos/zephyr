/*
 * Copyright (c) 2020 Innoseis B.V
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/sensor/tmp11x.h>

#define DT_DRV_COMPAT ti_tmp11x_eeprom

struct eeprom_tmp11x_config {
	const struct device *parent;
};

BUILD_ASSERT(CONFIG_EEPROM_INIT_PRIORITY >
	     CONFIG_SENSOR_INIT_PRIORITY,
	     "TMP11X eeprom driver must be initialized after TMP11X sensor "
	     "driver");

static size_t eeprom_tmp11x_size(const struct device *dev)
{
	return EEPROM_TMP11X_SIZE;
}

static int eeprom_tmp11x_write(const struct device *dev, off_t offset,
			       const void *data, size_t len)
{
	const struct eeprom_tmp11x_config *config = dev->config;

	return tmp11x_eeprom_write(config->parent, offset, data, len);
}

static int eeprom_tmp11x_read(const struct device *dev, off_t offset, void *data,
			      size_t len)
{
	const struct eeprom_tmp11x_config *config = dev->config;

	return tmp11x_eeprom_read(config->parent, offset, data, len);
}

static int eeprom_tmp11x_init(const struct device *dev)
{
	const struct eeprom_tmp11x_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(eeprom, eeprom_tmp11x_api) = {
	.read = eeprom_tmp11x_read,
	.write = eeprom_tmp11x_write,
	.size = eeprom_tmp11x_size,
};

#define DEFINE_TMP11X(_num)						       \
	static const struct eeprom_tmp11x_config eeprom_tmp11x_config##_num = { \
		.parent =  DEVICE_DT_GET(DT_INST_BUS(_num))		       \
	};								       \
	DEVICE_DT_INST_DEFINE(_num, eeprom_tmp11x_init, NULL,		       \
			      NULL, &eeprom_tmp11x_config##_num, POST_KERNEL,   \
			      CONFIG_EEPROM_INIT_PRIORITY, &eeprom_tmp11x_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_TMP11X);
