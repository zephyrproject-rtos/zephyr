/*
 * Copyright (c) 2018 Justin Watson
 * Copyright (c) 2020 ATL Electronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cypress_psoc6_gpio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>

#include "gpio_utils.h"
#include "cy_gpio.h"
#include "cy_sysint.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_psoc6);

typedef void (*config_func_t)(const struct device *dev);

struct gpio_psoc6_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_PRT_Type *regs;
	config_func_t config_func;
};

struct gpio_psoc6_runtime {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t cb;
};

static int gpio_psoc6_config(const struct device *dev,
			     gpio_pin_t pin,
			     gpio_flags_t flags)
{
	const struct gpio_psoc6_config * const cfg = dev->config;
	GPIO_PRT_Type * const port = cfg->regs;
	uint32_t drv_mode;
	uint32_t pin_val;

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_SINGLE_ENDED) {
			drv_mode = (flags & GPIO_LINE_OPEN_DRAIN) ?
				CY_GPIO_DM_OD_DRIVESLOW_IN_OFF :
				CY_GPIO_DM_OD_DRIVESHIGH_IN_OFF;

			pin_val = (flags & GPIO_LINE_OPEN_DRAIN) ? 1 : 0;
		} else {
			drv_mode = CY_GPIO_DM_STRONG_IN_OFF;

			pin_val = (flags & GPIO_OUTPUT_INIT_HIGH) ? 1 : 0;
		}
	} else {
		if ((flags & GPIO_PULL_UP) && (flags & GPIO_PULL_DOWN)) {
			drv_mode = CY_GPIO_DM_PULLUP_DOWN_IN_OFF;
		} else if (flags & GPIO_PULL_UP) {
			drv_mode = CY_GPIO_DM_PULLUP_IN_OFF;
		} else if (flags & GPIO_PULL_DOWN) {
			drv_mode = CY_GPIO_DM_PULLDOWN_IN_OFF;
		} else {
			drv_mode = CY_GPIO_DM_ANALOG;
		}

		pin_val = (flags & GPIO_PULL_UP) ? 1 : 0;
	}

	if (flags & GPIO_INPUT) {
		/* enable input buffer */
		drv_mode |= CY_GPIO_DM_HIGHZ;
	}

	Cy_GPIO_Pin_FastInit(port, pin, drv_mode, pin_val, HSIOM_SEL_GPIO);
	Cy_GPIO_SetVtrip(port, pin, CY_GPIO_VTRIP_CMOS);
	Cy_GPIO_SetSlewRate(port, pin, CY_GPIO_SLEW_FAST);
	Cy_GPIO_SetDriveSel(port, pin, CY_GPIO_DRIVE_FULL);

	LOG_DBG("P: 0x%08x, Pin: %d, Mode: 0x%08x, Val: 0x%02x",
			(unsigned int) port, pin, drv_mode, pin_val);

	return 0;
}

static int gpio_psoc6_port_get_raw(const struct device *dev,
				   uint32_t *value)
{
	const struct gpio_psoc6_config * const cfg = dev->config;
	GPIO_PRT_Type * const port = cfg->regs;

	*value = GPIO_PRT_IN(port);

	LOG_DBG("P: 0x%08x, V: 0x%08x", (unsigned int) port, *value);

	return 0;
}

static int gpio_psoc6_port_set_masked_raw(const struct device *dev,
					  uint32_t mask,
					  uint32_t value)
{
	const struct gpio_psoc6_config * const cfg = dev->config;
	GPIO_PRT_Type * const port = cfg->regs;

	GPIO_PRT_OUT(port) = (GPIO_PRT_IN(port) & ~mask) | (mask & value);

	return 0;
}

static int gpio_psoc6_port_set_bits_raw(const struct device *dev,
					uint32_t mask)
{
	const struct gpio_psoc6_config * const cfg = dev->config;
	GPIO_PRT_Type * const port = cfg->regs;

	GPIO_PRT_OUT_SET(port) = mask;

	return 0;
}

static int gpio_psoc6_port_clear_bits_raw(const struct device *dev,
					  uint32_t mask)
{
	const struct gpio_psoc6_config * const cfg = dev->config;
	GPIO_PRT_Type * const port = cfg->regs;

	GPIO_PRT_OUT_CLR(port) = mask;

	return 0;
}

static int gpio_psoc6_port_toggle_bits(const struct device *dev,
				       uint32_t mask)
{
	const struct gpio_psoc6_config * const cfg = dev->config;
	GPIO_PRT_Type * const port = cfg->regs;

	GPIO_PRT_OUT_INV(port) = mask;

	return 0;
}

static int gpio_psoc6_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	const struct gpio_psoc6_config * const cfg = dev->config;
	GPIO_PRT_Type * const port = cfg->regs;
	uint32_t is_enabled = ((mode == GPIO_INT_MODE_DISABLED) ? 0 : 1);
	uint32_t lv_trg = CY_GPIO_INTR_DISABLE;

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	if (is_enabled) {
		switch (trig) {
		case GPIO_INT_TRIG_BOTH:
			lv_trg = CY_GPIO_INTR_BOTH;
			break;
		case GPIO_INT_TRIG_HIGH:
			lv_trg = CY_GPIO_INTR_RISING;
			break;
		case GPIO_INT_TRIG_LOW:
			lv_trg = CY_GPIO_INTR_FALLING;
			break;
		}
	}

	Cy_GPIO_ClearInterrupt(port, pin);
	Cy_GPIO_SetInterruptEdge(port, pin, lv_trg);
	Cy_GPIO_SetInterruptMask(port, pin, is_enabled);
	/**
	 * The driver will set 50ns glitch free filter for any interrupt.
	 */
	Cy_GPIO_SetFilter(port, pin);

	LOG_DBG("config: Pin: %d, Trg: %d", pin, lv_trg);

	return 0;
}

static void gpio_psoc6_isr(const struct device *dev)
{
	const struct gpio_psoc6_config * const cfg = dev->config;
	GPIO_PRT_Type * const port = cfg->regs;
	struct gpio_psoc6_runtime *context = dev->data;
	uint32_t int_stat;

	int_stat = GPIO_PRT_INTR_MASKED(port);

	/* Any INTR MMIO registers AHB clearing must be preceded with an AHB
	 * read access. Taken from Cy_GPIO_ClearInterrupt()
	 */
	(void)GPIO_PRT_INTR(port);
	GPIO_PRT_INTR(port) = int_stat;
	/* This read ensures that the initial write has been flushed out to HW
	 */
	(void)GPIO_PRT_INTR(port);

	gpio_fire_callbacks(&context->cb, dev, int_stat);
}

static int gpio_psoc6_manage_callback(const struct device *port,
				    struct gpio_callback *callback,
				    bool set)
{
	struct gpio_psoc6_runtime *context = port->data;

	return gpio_manage_callback(&context->cb, callback, set);
}

static uint32_t gpio_psoc6_get_pending_int(const struct device *dev)
{
	const struct gpio_psoc6_config * const cfg = dev->config;
	GPIO_PRT_Type * const port = cfg->regs;

	LOG_DBG("Pending: 0x%08x", GPIO_PRT_INTR_MASKED(port));

	return GPIO_PRT_INTR_MASKED(port);
}

static const struct gpio_driver_api gpio_psoc6_api = {
	.pin_configure = gpio_psoc6_config,
	.port_get_raw = gpio_psoc6_port_get_raw,
	.port_set_masked_raw = gpio_psoc6_port_set_masked_raw,
	.port_set_bits_raw = gpio_psoc6_port_set_bits_raw,
	.port_clear_bits_raw = gpio_psoc6_port_clear_bits_raw,
	.port_toggle_bits = gpio_psoc6_port_toggle_bits,
	.pin_interrupt_configure = gpio_psoc6_pin_interrupt_configure,
	.manage_callback = gpio_psoc6_manage_callback,
	.get_pending_int = gpio_psoc6_get_pending_int,
};

int gpio_psoc6_init(const struct device *dev)
{
	const struct gpio_psoc6_config * const cfg = dev->config;

	cfg->config_func(dev);

	return 0;
}

#define GPIO_PSOC6_INIT(n)						\
	static void port_##n##_psoc6_config_func(const struct device *dev); \
									\
	static const struct gpio_psoc6_config port_##n##_psoc6_config = { \
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),\
		},							\
		.regs = (GPIO_PRT_Type *)DT_INST_REG_ADDR(n),		\
		.config_func = port_##n##_psoc6_config_func,		\
	};								\
									\
	static struct gpio_psoc6_runtime port_##n##_psoc6_runtime = { 0 }; \
									\
	DEVICE_DT_INST_DEFINE(n, gpio_psoc6_init, NULL,			\
			    &port_##n##_psoc6_runtime,			\
			    &port_##n##_psoc6_config, PRE_KERNEL_1,	\
			    CONFIG_GPIO_INIT_PRIORITY,			\
			    &gpio_psoc6_api);				\
									\
	static void port_##n##_psoc6_config_func(const struct device *dev) \
	{								\
		CY_PSOC6_DT_INST_NVIC_INSTALL(n, gpio_psoc6_isr);	\
	};

DT_INST_FOREACH_STATUS_OKAY(GPIO_PSOC6_INIT)
