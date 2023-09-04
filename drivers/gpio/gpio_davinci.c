/*
 * Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_davinci_gpio

#include <errno.h>

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>

LOG_MODULE_REGISTER(gpio_davinci, CONFIG_GPIO_LOG_LEVEL);

/* Helper Macros for GPIO */
#define DEV_CFG(dev) \
		((const struct gpio_davinci_config *)((dev)->config))
#define DEV_DATA(dev) ((struct gpio_davinci_data *)(dev)->data)
#define DEV_GPIO_CFG_BASE(dev) \
	((struct gpio_davinci_regs *)DEVICE_MMIO_NAMED_GET(dev, port_base))

#define GPIO_DAVINCI_DIR_RESET_VAL	(0xFFFFFFFF)

struct gpio_davinci_regs {
	uint32_t dir;
	uint32_t out_data;
	uint32_t set_data;
	uint32_t clr_data;
	uint32_t in_data;
	uint32_t set_ris_trig;
	uint32_t clr_ris_trig;
	uint32_t set_fal_trig;
	uint32_t clr_fal_trig;
	uint32_t intstat;
};

struct gpio_davinci_data {
	struct gpio_driver_data common;

	DEVICE_MMIO_NAMED_RAM(port_base);

	sys_slist_t cb;
};

struct gpio_davinci_config {
	void (*bank_config)(const struct device *dev);
	struct gpio_driver_config common;

	DEVICE_MMIO_NAMED_ROM(port_base);

	uint32_t port_num;
	const struct pinctrl_dev_config *pcfg;
};

static int gpio_davinci_configure(const struct device *dev, gpio_pin_t pin,
					gpio_flags_t flags)
{
	volatile struct gpio_davinci_regs *regs = DEV_GPIO_CFG_BASE(dev);

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			regs->set_data = BIT(pin);
		} else {
			regs->clr_data = BIT(pin);
		}
		regs->dir &= ~(BIT(pin));
	} else {
		regs->dir |= BIT(pin);
	}

	return 0;
}

static int gpio_davinci_port_get_raw(const struct device *dev,
					gpio_port_value_t *value)
{
	volatile struct gpio_davinci_regs *regs = DEV_GPIO_CFG_BASE(dev);

	*value = regs->in_data;

	return 0;
}

static int gpio_davinci_port_set_masked_raw(const struct device *dev,
		gpio_port_pins_t mask, gpio_port_value_t value)
{
	volatile struct gpio_davinci_regs *regs = DEV_GPIO_CFG_BASE(dev);

	regs->out_data = (regs->out_data & (~mask)) | (mask & value);

	return 0;
}

static int gpio_davinci_port_set_bits_raw(const struct device *dev,
						gpio_port_pins_t mask)
{
	volatile struct gpio_davinci_regs *regs = DEV_GPIO_CFG_BASE(dev);

	regs->set_data |= mask;

	return 0;
}

static int gpio_davinci_port_clear_bits_raw(const struct device *dev,
						gpio_port_pins_t mask)
{
	volatile struct gpio_davinci_regs *regs = DEV_GPIO_CFG_BASE(dev);

	regs->clr_data |= mask;

	return 0;
}

static int gpio_davinci_port_toggle_bits(const struct device *dev,
						gpio_port_pins_t mask)
{
	volatile struct gpio_davinci_regs *regs = DEV_GPIO_CFG_BASE(dev);

	regs->out_data ^= mask;

	return 0;
}

static const struct gpio_driver_api gpio_davinci_driver_api = {
	.pin_configure = gpio_davinci_configure,
	.port_get_raw = gpio_davinci_port_get_raw,
	.port_set_masked_raw = gpio_davinci_port_set_masked_raw,
	.port_set_bits_raw = gpio_davinci_port_set_bits_raw,
	.port_clear_bits_raw = gpio_davinci_port_clear_bits_raw,
	.port_toggle_bits = gpio_davinci_port_toggle_bits
};

static int gpio_davinci_init(const struct device *dev)
{
	const struct gpio_davinci_config *config = DEV_CFG(dev);
	volatile struct gpio_davinci_regs *regs = DEV_GPIO_CFG_BASE(dev);
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, port_base, K_MEM_CACHE_NONE);

	regs->dir = GPIO_DAVINCI_DIR_RESET_VAL;

	config->bank_config(dev);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("failed to apply pinctrl");
		return ret;
	}
	return 0;
}

#define GPIO_DAVINCI_INIT_FUNC(n)						  \
	static void gpio_davinci_bank_##n##_config(const struct device *dev)	  \
	{									  \
		volatile struct gpio_davinci_regs *regs = DEV_GPIO_CFG_BASE(dev); \
		ARG_UNUSED(regs);						  \
	}

#define GPIO_DAVINCI_INIT(n)							  \
	PINCTRL_DT_INST_DEFINE(n);						  \
	GPIO_DAVINCI_INIT_FUNC(n);						  \
	static const struct gpio_davinci_config gpio_davinci_##n##_config = {	  \
		.bank_config = gpio_davinci_bank_##n##_config,			  \
		.common = {							  \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	  \
		},								  \
		DEVICE_MMIO_NAMED_ROM_INIT(port_base, DT_DRV_INST(n)),		  \
		.port_num = n,							  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			  \
	};									  \
										  \
	static struct gpio_davinci_data gpio_davinci_##n##_data;		  \
										  \
	DEVICE_DT_INST_DEFINE(n,						  \
		&gpio_davinci_init,						  \
		NULL,								  \
		&gpio_davinci_##n##_data,					  \
		&gpio_davinci_##n##_config,					  \
		PRE_KERNEL_2,							  \
		CONFIG_GPIO_INIT_PRIORITY,					  \
		&gpio_davinci_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_DAVINCI_INIT)
