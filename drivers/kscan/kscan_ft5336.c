/*
 * Copyright (c) 2020 NXP
 * Copyright (c) 2020 Mark Olsson <mark@markolsson.se>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT focaltech_ft5336

#include <drivers/kscan.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(ft5336, CONFIG_KSCAN_LOG_LEVEL);

#define FT5406_DATA_SIZE	0x20

enum ft5336_event {
	FT5336_EVENT_DOWN	= 0,
	FT5336_EVENT_UP		= 1,
	FT5336_EVENT_CONTACT	= 2,
	FT5336_EVENT_RESERVED	= 3,
};

struct ft5336_config {
	char *i2c_name;
	u8_t i2c_address;
#ifdef CONFIG_KSCAN_FT5336_INTERRUPT
	char *int_gpio_controller;
	u8_t int_gpio_pin;
	u8_t int_gpio_flags;
	char *label;
#endif
};

struct ft5336_data {
	struct device *i2c;
	kscan_callback_t callback;
	struct k_work work;
#ifdef CONFIG_KSCAN_FT5336_INTERRUPT
	struct gpio_callback int_gpio_cb;
	struct device *int_gpio_dev;
#else
	struct k_timer timer;
#endif
	struct device *dev;
};

static int ft5336_read(struct device *dev)
{
	const struct ft5336_config *config = dev->config_info;
	struct ft5336_data *data = dev->driver_data;
	u8_t buffer[FT5406_DATA_SIZE];
	u8_t event;
	u16_t row, column;
	bool pressed;

	if (i2c_burst_read(data->i2c, config->i2c_address, 1, buffer,
			   sizeof(buffer))) {
		LOG_ERR("Could not read point");
		return -EIO;
	}

	event = buffer[2] >> 6;
	pressed = (event == FT5336_EVENT_DOWN) ||
		  (event == FT5336_EVENT_CONTACT);

	row = ((buffer[2] & 0x0f) << 8) | buffer[3];
	column = ((buffer[4] & 0x0f) << 8) | buffer[5];

	LOG_DBG("row: %d, col: %d, pressed: %d", row, column, pressed);

	data->callback(dev, row, column, pressed);

#ifdef CONFIG_KSCAN_FT5336_INTERRUPT
	gpio_pin_interrupt_configure(data->int_gpio_dev, config->int_gpio_pin,
		   GPIO_INT_EDGE_TO_ACTIVE);
#endif
	return 0;
}

#ifndef CONFIG_KSCAN_FT5336_INTERRUPT
static void ft5336_timer_handler(struct k_timer *timer)
{
	struct ft5336_data *data =
		CONTAINER_OF(timer, struct ft5336_data, timer);

	k_work_submit(&data->work);
}
#endif

static void ft5336_work_handler(struct k_work *work)
{
	struct ft5336_data *data =
		CONTAINER_OF(work, struct ft5336_data, work);

	ft5336_read(data->dev);
}

#ifdef CONFIG_KSCAN_FT5336_INTERRUPT
static void ft5336_isr_handler(struct device *dev, struct gpio_callback *cb,
		    u32_t pins)
{
	const struct ft5336_config *config = dev->config_info;
	struct ft5336_data *drv_data =
		CONTAINER_OF(cb, struct ft5336_data, int_gpio_cb);

	gpio_pin_interrupt_configure(dev, config->int_gpio_pin,
		GPIO_INT_DISABLE);

	k_work_submit(&drv_data->work);
}
#endif

static int ft5336_configure(struct device *dev, kscan_callback_t callback)
{
	struct ft5336_data *data = dev->driver_data;

	if (!callback) {
		return -EINVAL;
	}

	data->callback = callback;

	return 0;
}

static int ft5336_enable_callback(struct device *dev)
{
	struct ft5336_data *data = dev->driver_data;

#ifdef CONFIG_KSCAN_FT5336_INTERRUPT
	gpio_add_callback(data->int_gpio_dev, &data->int_gpio_cb);
#else
	k_timer_start(&data->timer, K_MSEC(CONFIG_KSCAN_FT5336_PERIOD),
		      K_MSEC(CONFIG_KSCAN_FT5336_PERIOD));
#endif

	return 0;
}

static int ft5336_disable_callback(struct device *dev)
{
	struct ft5336_data *data = dev->driver_data;

#ifdef CONFIG_KSCAN_FT5336_INTERRUPT
	gpio_remove_callback(data->int_gpio_dev, &data->int_gpio_cb);
#else
	k_timer_stop(&data->timer);
#endif

	return 0;
}

static int ft5336_init(struct device *dev)
{
	const struct ft5336_config *config = dev->config_info;
	struct ft5336_data *data = dev->driver_data;

	data->i2c = device_get_binding(config->i2c_name);
	if (data->i2c == NULL) {
		LOG_ERR("Could not find I2C device");
		return -EINVAL;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address, 0, 0)) {
		LOG_ERR("Could not enable");
		return -EINVAL;
	}

	data->dev = dev;

	k_work_init(&data->work, ft5336_work_handler);
#ifndef CONFIG_KSCAN_FT5336_INTERRUPT
	k_timer_init(&data->timer, ft5336_timer_handler, NULL);
#else
	int ret;

	data->int_gpio_dev = device_get_binding(config->int_gpio_controller);
	if (data->int_gpio_dev == NULL) {
		LOG_ERR("Error: unknown device %s\n",
			config->int_gpio_controller);
		return -EINVAL;
	}

	ret = gpio_pin_configure(data->int_gpio_dev, config->int_gpio_pin,
		   config->int_gpio_flags | GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure pin %d '%s'\n",
			ret, config->int_gpio_pin, config->label);
		return -EINVAL;
	}

	ret = gpio_pin_interrupt_configure(data->int_gpio_dev,
		   config->int_gpio_pin, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure pin interrupt %d '%s'\n",
			ret, config->int_gpio_pin, config->label);
		return -EINVAL;
	}

	gpio_init_callback(&data->int_gpio_cb, ft5336_isr_handler,
		   BIT(config->int_gpio_pin));
#endif

	return 0;
}


static const struct kscan_driver_api ft5336_driver_api = {
	.config = ft5336_configure,
	.enable_callback = ft5336_enable_callback,
	.disable_callback = ft5336_disable_callback,
};

static const struct ft5336_config ft5336_config = {
	.i2c_name = DT_INST_BUS_LABEL(0),
	.i2c_address = DT_INST_REG_ADDR(0),
#ifdef CONFIG_KSCAN_FT5336_INTERRUPT
	.int_gpio_controller = DT_INST_GPIO_LABEL(0, int_gpios),
	.int_gpio_pin = DT_INST_GPIO_PIN(0, int_gpios),
	.int_gpio_flags = DT_INST_GPIO_FLAGS(0, int_gpios),
	.label = DT_INST_GPIO_LABEL(0, int_gpios),
#endif
};

static struct ft5336_data ft5336_data;

DEVICE_AND_API_INIT(ft5336, DT_INST_LABEL(0), ft5336_init,
		    &ft5336_data, &ft5336_config,
		    POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,
		    &ft5336_driver_api);
