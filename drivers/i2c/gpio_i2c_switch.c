/*
 * Copyright (c) 2023 Ayush Singh <ayushdevel1325@gmail.com>
 * Copyright (c) 2021 Jason Kridner, BeagleBoard.org Foundation
 * Copyright (c) 2020 Innoseis BV
 *
 * Based on i2c_tca9656a.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_i2c_switch

#define GPIO_I2C_TOGGLE_DELAY_US 1
#define GPIO_I2C_LOCK_TIMEOUT_US   (GPIO_I2C_TOGGLE_DELAY_US * 2 + 100)

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_i2c_switch, CONFIG_I2C_LOG_LEVEL);

struct gpio_i2c_switch_config {
	const struct device *bus;
	const struct gpio_dt_spec gpio;
};

struct gpio_i2c_switch_data {
	struct k_mutex lock;
};

static int gpio_i2c_switch_configure(const struct device *dev, uint32_t dev_config)
{
	const struct gpio_i2c_switch_config *config = dev->config;

	return i2c_configure(config->bus, dev_config);
}

static int gpio_i2c_switch_transfer(const struct device *dev, struct i2c_msg *msgs,
				    uint8_t num_msgs, uint16_t addr)
{
	int res;
	struct gpio_i2c_switch_data *data = dev->data;
	const struct gpio_i2c_switch_config *config = dev->config;

	res = k_mutex_lock(&data->lock, K_USEC(GPIO_I2C_LOCK_TIMEOUT_US));
	if (res != 0) {
		return res;
	}

	/* enable switch */
	gpio_pin_set_dt(&config->gpio, 1);
	k_busy_wait(GPIO_I2C_TOGGLE_DELAY_US);

	res = i2c_transfer(config->bus, msgs, num_msgs, addr);

	/* disable switch */
	gpio_pin_set_dt(&config->gpio, 0);
	k_busy_wait(GPIO_I2C_TOGGLE_DELAY_US);
	k_mutex_unlock(&data->lock);

	return res;
}

static const struct i2c_driver_api gpio_i2c_switch_api_funcs = {
	.configure = gpio_i2c_switch_configure,
	.transfer = gpio_i2c_switch_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static int gpio_i2c_switch_init(const struct device *dev)
{
	const struct gpio_i2c_switch_config *config = dev->config;
	struct gpio_i2c_switch_data *data = dev->data;

	k_mutex_init(&data->lock);

	return gpio_pin_configure_dt(&config->gpio, GPIO_OUTPUT_INACTIVE);
}

#define DEFINE_GPIO_I2C_SWITCH(inst)                                                               \
                                                                                                   \
	static struct gpio_i2c_switch_data gpio_i2c_switch_dev_data_##inst;                        \
                                                                                                   \
	static const struct gpio_i2c_switch_config gpio_i2c_switch_dev_cfg_##inst = {              \
		.bus = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(inst), controller)),                   \
		.gpio = GPIO_DT_SPEC_GET(DT_DRV_INST(inst), gpios),                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, gpio_i2c_switch_init, device_pm_control_nop,                   \
			      &gpio_i2c_switch_dev_data_##inst, &gpio_i2c_switch_dev_cfg_##inst,   \
			      POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &gpio_i2c_switch_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_GPIO_I2C_SWITCH)
