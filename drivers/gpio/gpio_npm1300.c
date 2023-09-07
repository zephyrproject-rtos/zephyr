/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm1300_gpio

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/dt-bindings/gpio/nordic-npm1300-gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

/* nPM1300 GPIO base address */
#define NPM_GPIO_BASE 0x06U

/* nPM1300 GPIO registers offsets */
#define NPM_GPIO_OFFSET_MODE	  0x00U
#define NPM_GPIO_OFFSET_DRIVE	  0x05U
#define NPM_GPIO_OFFSET_PULLUP	  0x0AU
#define NPM_GPIO_OFFSET_PULLDOWN  0x0FU
#define NPM_GPIO_OFFSET_OPENDRAIN 0x14U
#define NPM_GPIO_OFFSET_DEBOUNCE  0x19U
#define NPM_GPIO_OFFSET_STATUS	  0x1EU

/* nPM1300 Channel counts */
#define NPM1300_GPIO_PINS 5U

#define NPM1300_GPIO_GPIINPUT	    0
#define NPM1300_GPIO_GPILOGIC1	    1
#define NPM1300_GPIO_GPILOGIC0	    2
#define NPM1300_GPIO_GPIEVENTRISE   3
#define NPM1300_GPIO_GPIEVENTFALL   4
#define NPM1300_GPIO_GPOIRQ	    5
#define NPM1300_GPIO_GPORESET	    6
#define NPM1300_GPIO_GPOPWRLOSSWARN 7
#define NPM1300_GPIO_GPOLOGIC1	    8
#define NPM1300_GPIO_GPOLOGIC0	    9

struct gpio_npm1300_config {
	struct gpio_driver_config common;
	const struct device *mfd;
};

struct gpio_npm1300_data {
	struct gpio_driver_data common;
};

static int gpio_npm1300_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_npm1300_config *config = dev->config;
	int ret;
	uint8_t data;

	ret = mfd_npm1300_reg_read(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_STATUS, &data);

	if (ret < 0) {
		return ret;
	}

	*value = data;

	return 0;
}

static int gpio_npm1300_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_npm1300_config *config = dev->config;
	int ret = 0;

	for (size_t idx = 0; idx < NPM1300_GPIO_PINS; idx++) {
		if ((mask & BIT(idx)) != 0U) {
			if ((value & BIT(idx)) != 0U) {
				ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE,
							    NPM_GPIO_OFFSET_MODE + idx,
							    NPM1300_GPIO_GPOLOGIC1);
			} else {
				ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE,
							    NPM_GPIO_OFFSET_MODE + idx,
							    NPM1300_GPIO_GPOLOGIC0);
			}
			if (ret != 0U) {
				return ret;
			}
		}
	}

	return ret;
}

static int gpio_npm1300_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_npm1300_port_set_masked_raw(dev, pins, pins);
}

static int gpio_npm1300_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_npm1300_port_set_masked_raw(dev, pins, 0U);
}

static inline int gpio_npm1300_configure(const struct device *dev, gpio_pin_t pin,
					 gpio_flags_t flags)
{
	const struct gpio_npm1300_config *config = dev->config;
	int ret = 0;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pin >= NPM1300_GPIO_PINS) {
		return -EINVAL;
	}

	/* Configure mode */
	if ((flags & GPIO_INPUT) != 0U) {
		ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_MODE + pin,
					    NPM1300_GPIO_GPIINPUT);
	} else if ((flags & NPM1300_GPIO_WDT_RESET_ON) != 0U) {
		ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_MODE + pin,
					    NPM1300_GPIO_GPORESET);
	} else if ((flags & NPM1300_GPIO_PWRLOSSWARN_ON) != 0U) {
		ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_MODE + pin,
					    NPM1300_GPIO_GPOPWRLOSSWARN);
	} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
		ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_MODE + pin,
					    NPM1300_GPIO_GPOLOGIC1);
	} else if ((flags & GPIO_OUTPUT) != 0U) {
		ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_MODE + pin,
					    NPM1300_GPIO_GPOLOGIC0);
	}

	if (ret < 0) {
		return ret;
	}

	/* Configure open drain */
	ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_OPENDRAIN + pin,
				    !!(flags & GPIO_SINGLE_ENDED));
	if (ret < 0) {
		return ret;
	}

	/* Configure pulls */
	ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_PULLUP + pin,
				    !!(flags & GPIO_PULL_UP));
	if (ret < 0) {
		return ret;
	}

	ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_PULLDOWN + pin,
				    !!(flags & GPIO_PULL_DOWN));
	if (ret < 0) {
		return ret;
	}

	/* Configure drive strength and debounce */
	ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_DRIVE + pin,
				    !!(flags & NPM1300_GPIO_DRIVE_6MA));
	if (ret < 0) {
		return ret;
	}

	ret = mfd_npm1300_reg_write(config->mfd, NPM_GPIO_BASE, NPM_GPIO_OFFSET_DEBOUNCE + pin,
				    !!(flags & NPM1300_GPIO_DEBOUNCE_ON));

	return ret;
}

static int gpio_npm1300_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	uint32_t value;

	ret = gpio_npm1300_port_get_raw(dev, &value);

	if (ret < 0) {
		return ret;
	}

	return gpio_npm1300_port_set_masked_raw(dev, pins, ~value);
}

static const struct gpio_driver_api gpio_npm1300_api = {
	.pin_configure = gpio_npm1300_configure,
	.port_get_raw = gpio_npm1300_port_get_raw,
	.port_set_masked_raw = gpio_npm1300_port_set_masked_raw,
	.port_set_bits_raw = gpio_npm1300_port_set_bits_raw,
	.port_clear_bits_raw = gpio_npm1300_port_clear_bits_raw,
	.port_toggle_bits = gpio_npm1300_port_toggle_bits,
};

static int gpio_npm1300_init(const struct device *dev)
{
	const struct gpio_npm1300_config *config = dev->config;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	return 0;
}

#define GPIO_NPM1300_DEFINE(n)                                                                     \
	static const struct gpio_npm1300_config gpio_npm1300_config##n = {                         \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n))};                                          \
                                                                                                   \
		static struct gpio_npm1300_data gpio_npm1300_data##n;                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &gpio_npm1300_init, NULL, &gpio_npm1300_data##n,                  \
			      &gpio_npm1300_config##n, POST_KERNEL,                                \
			      CONFIG_GPIO_NPM1300_INIT_PRIORITY, &gpio_npm1300_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NPM1300_DEFINE)
