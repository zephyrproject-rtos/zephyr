/*
 * Copyright (c) 2019 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver for the HT16K33 I2C LED driver with keyscan
 */

#include <gpio.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_ht16k33);

#include <led/ht16k33.h>

#include "gpio_utils.h"

/* HT16K33 size definitions */
#define HT16K33_KEYSCAN_ROWS 3

struct gpio_ht16k33_cfg {
	char *parent_dev_name;
	u8_t keyscan_idx;
};

struct gpio_ht16k33_data {
	struct device *parent;
	sys_slist_t callbacks;
};

static int gpio_ht16k33_cfg(struct device *dev, int access_op,
			       u32_t pin, int flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pin);

	/* Keyscan is input-only */
	if ((flags & GPIO_DIR_MASK) != GPIO_DIR_IN) {
		return -EINVAL;
	}

	return 0;
}

static int gpio_ht16k33_write(struct device *dev, int access_op,
			      u32_t pin, u32_t value)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pin);
	ARG_UNUSED(value);

	/* Keyscan is input-only */
	return -ENOTSUP;
}

static int gpio_ht16k33_read(struct device *dev, int access_op,
			     u32_t pin, u32_t *value)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pin);
	ARG_UNUSED(value);

	/* Keyscan only supports interrupt mode */
	return -ENOTSUP;
}

void ht16k33_process_keyscan_row_data(struct device *dev,
				      u32_t keys)
{
	struct gpio_ht16k33_data *data = dev->driver_data;

	gpio_fire_callbacks(&data->callbacks, dev, keys);
}

static int gpio_ht16k33_manage_callback(struct device *dev,
					struct gpio_callback *callback,
					bool set)
{
	struct gpio_ht16k33_data *data = dev->driver_data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int gpio_ht16k33_enable_callback(struct device *dev,
					int access_op,
					u32_t pin)
{
	/* All callbacks are always enabled */
	return 0;
}

static int gpio_ht16k33_disable_callback(struct device *dev,
					int access_op,
					u32_t pin)
{
	/* Individual callbacks can not be disabled */
	return -ENOTSUP;
}

static u32_t gpio_ht16k33_get_pending_int(struct device *dev)
{
	struct gpio_ht16k33_data *data = dev->driver_data;

	return ht16k33_get_pending_int(data->parent);
}

static int gpio_ht16k33_init(struct device *dev)
{
	const struct gpio_ht16k33_cfg *config = dev->config->config_info;
	struct gpio_ht16k33_data *data = dev->driver_data;

	if (config->keyscan_idx >= HT16K33_KEYSCAN_ROWS) {
		LOG_ERR("HT16K33 keyscan index out of bounds (%d)",
			config->keyscan_idx);
		return -EINVAL;
	}

	/* Establish reference to parent and vice versa */
	data->parent = device_get_binding(config->parent_dev_name);
	if (!data->parent) {
		LOG_ERR("HT16K33 parent device '%s' not found",
			config->parent_dev_name);
		return -EINVAL;
	}

	return ht16k33_register_keyscan_device(data->parent, dev,
					       config->keyscan_idx);
}

static const struct gpio_driver_api gpio_ht16k33_api = {
	.config = gpio_ht16k33_cfg,
	.write = gpio_ht16k33_write,
	.read = gpio_ht16k33_read,
	.manage_callback = gpio_ht16k33_manage_callback,
	.enable_callback = gpio_ht16k33_enable_callback,
	.disable_callback = gpio_ht16k33_disable_callback,
	.get_pending_int = gpio_ht16k33_get_pending_int,
};

#define GPIO_HT16K33_DEVICE(id)						\
	static const struct gpio_ht16k33_cfg gpio_ht16k33_##id##_cfg = {\
		.parent_dev_name =					\
			DT_HOLTEK_HT16K33_KEYSCAN_##id##_BUS_NAME,	\
		.keyscan_idx     =					\
			DT_HOLTEK_HT16K33_KEYSCAN_##id##_BASE_ADDRESS,	\
	};								\
									\
	static struct gpio_ht16k33_data gpio_ht16k33_##id##_data;	\
									\
	DEVICE_AND_API_INIT(gpio_ht16k33_##id,				\
			    DT_HOLTEK_HT16K33_KEYSCAN_##id##_LABEL,	\
			    &gpio_ht16k33_init,				\
			    &gpio_ht16k33_##id##_data,			\
			    &gpio_ht16k33_##id##_cfg, POST_KERNEL,	\
			    CONFIG_GPIO_HT16K33_INIT_PRIORITY,		\
			    &gpio_ht16k33_api)

/* Support up to eight HT16K33 devices, each with three keyscan devices */

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_0
GPIO_HT16K33_DEVICE(0);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_1
GPIO_HT16K33_DEVICE(1);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_2
GPIO_HT16K33_DEVICE(2);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_3
GPIO_HT16K33_DEVICE(3);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_4
GPIO_HT16K33_DEVICE(4);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_5
GPIO_HT16K33_DEVICE(5);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_6
GPIO_HT16K33_DEVICE(6);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_7
GPIO_HT16K33_DEVICE(7);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_8
GPIO_HT16K33_DEVICE(8);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_9
GPIO_HT16K33_DEVICE(9);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_10
GPIO_HT16K33_DEVICE(10);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_11
GPIO_HT16K33_DEVICE(11);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_12
GPIO_HT16K33_DEVICE(12);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_13
GPIO_HT16K33_DEVICE(13);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_14
GPIO_HT16K33_DEVICE(14);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_15
GPIO_HT16K33_DEVICE(15);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_16
GPIO_HT16K33_DEVICE(16);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_17
GPIO_HT16K33_DEVICE(17);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_18
GPIO_HT16K33_DEVICE(18);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_19
GPIO_HT16K33_DEVICE(19);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_20
GPIO_HT16K33_DEVICE(20);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_21
GPIO_HT16K33_DEVICE(21);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_22
GPIO_HT16K33_DEVICE(22);
#endif

#ifdef DT_HOLTEK_HT16K33_KEYSCAN_23
GPIO_HT16K33_DEVICE(23);
#endif

