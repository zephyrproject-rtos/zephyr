/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_cmsdk_gpio

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/clock_control/arm_clock_control.h>

#include "gpio_cmsdk_ahb.h"
#include "gpio_utils.h"

/**
 * @brief GPIO driver for ARM CMSDK AHB GPIO
 */

typedef void (*gpio_config_func_t)(const struct device *port);

struct gpio_cmsdk_ahb_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	volatile struct gpio_cmsdk_ahb *port;
	gpio_config_func_t gpio_config_func;
	/* GPIO Clock control in Active State */
	struct arm_clock_control_t gpio_cc_as;
	/* GPIO Clock control in Sleep State */
	struct arm_clock_control_t gpio_cc_ss;
	/* GPIO Clock control in Deep Sleep State */
	struct arm_clock_control_t gpio_cc_dss;
};

struct gpio_cmsdk_ahb_dev_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* list of callbacks */
	sys_slist_t gpio_cb;
};

static int gpio_cmsdk_ahb_port_get_raw(const struct device *dev,
				       uint32_t *value)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config;

	*value = cfg->port->data;

	return 0;
}

static int gpio_cmsdk_ahb_port_set_masked_raw(const struct device *dev,
					      uint32_t mask,
					      uint32_t value)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config;

	cfg->port->dataout = (cfg->port->dataout & ~mask) | (mask & value);

	return 0;
}

static int gpio_cmsdk_ahb_port_set_bits_raw(const struct device *dev,
					    uint32_t mask)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config;

	cfg->port->dataout |= mask;

	return 0;
}

static int gpio_cmsdk_ahb_port_clear_bits_raw(const struct device *dev,
					      uint32_t mask)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config;

	cfg->port->dataout &= ~mask;

	return 0;
}

static int gpio_cmsdk_ahb_port_toggle_bits(const struct device *dev,
					   uint32_t mask)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config;

	cfg->port->dataout ^= mask;

	return 0;
}

static int cmsdk_ahb_gpio_config(const struct device *dev, uint32_t mask,
				 gpio_flags_t flags)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config;

	if (((flags & GPIO_INPUT) == 0) && ((flags & GPIO_OUTPUT) == 0)) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	/*
	 * Setup the pin direction
	 * Output Enable:
	 * 0 - Input
	 * 1 - Output
	 */
	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_cmsdk_ahb_port_set_bits_raw(dev, mask);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_cmsdk_ahb_port_clear_bits_raw(dev, mask);
		}
		cfg->port->outenableset = mask;
	} else {
		cfg->port->outenableclr = mask;
	}

	cfg->port->altfuncclr = mask;

	return 0;
}

/**
 * @brief Configure pin or port
 *
 * @param dev Device struct
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_cmsdk_ahb_config(const struct device *dev,
				 gpio_pin_t pin,
				 gpio_flags_t flags)
{
	return cmsdk_ahb_gpio_config(dev, BIT(pin), flags);
}

static int gpio_cmsdk_ahb_pin_interrupt_configure(const struct device *dev,
						  gpio_pin_t pin,
						  enum gpio_int_mode mode,
						  enum gpio_int_trig trig)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config;

	if (trig == GPIO_INT_TRIG_BOTH) {
		return -ENOTSUP;
	}

	/* For now treat level interrupts as not supported, as we seem to only
	 * get a single 'edge' still interrupt rather than continuous
	 * interrupts until the cause is cleared */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		cfg->port->intenclr = BIT(pin);
	} else {
		if (mode == GPIO_INT_MODE_EDGE) {
			cfg->port->inttypeset = BIT(pin);
		} else {
			/* LEVEL */
			cfg->port->inttypeclr = BIT(pin);
		}

		/* Level High or Edge Rising */
		if (trig == GPIO_INT_TRIG_HIGH) {
			cfg->port->intpolset = BIT(pin);
		} else {
			cfg->port->intpolclr = BIT(pin);
		}
		cfg->port->intclear = BIT(pin);
		cfg->port->intenset = BIT(pin);
	}

	return 0;
}

static void gpio_cmsdk_ahb_isr(const struct device *dev)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config;
	struct gpio_cmsdk_ahb_dev_data *data = dev->data;
	uint32_t int_stat;

	int_stat = cfg->port->intstatus;

	/* clear the port interrupts */
	cfg->port->intclear = int_stat;

	gpio_fire_callbacks(&data->gpio_cb, dev, int_stat);

}

static int gpio_cmsdk_ahb_manage_callback(const struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct gpio_cmsdk_ahb_dev_data *data = dev->data;

	return gpio_manage_callback(&data->gpio_cb, callback, set);
}

static const struct gpio_driver_api gpio_cmsdk_ahb_drv_api_funcs = {
	.pin_configure = gpio_cmsdk_ahb_config,
	.port_get_raw = gpio_cmsdk_ahb_port_get_raw,
	.port_set_masked_raw = gpio_cmsdk_ahb_port_set_masked_raw,
	.port_set_bits_raw = gpio_cmsdk_ahb_port_set_bits_raw,
	.port_clear_bits_raw = gpio_cmsdk_ahb_port_clear_bits_raw,
	.port_toggle_bits = gpio_cmsdk_ahb_port_toggle_bits,
	.pin_interrupt_configure = gpio_cmsdk_ahb_pin_interrupt_configure,
	.manage_callback = gpio_cmsdk_ahb_manage_callback,
};

/**
 * @brief Initialization function of GPIO
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_cmsdk_ahb_init(const struct device *dev)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config;

#ifdef CONFIG_CLOCK_CONTROL
	/* Enable clock for subsystem */
	const struct device *clk =
		device_get_binding(CONFIG_ARM_CLOCK_CONTROL_DEV_NAME);

#ifdef CONFIG_SOC_SERIES_BEETLE
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->gpio_cc_as);
	clock_control_off(clk, (clock_control_subsys_t *) &cfg->gpio_cc_ss);
	clock_control_off(clk, (clock_control_subsys_t *) &cfg->gpio_cc_dss);
#endif /* CONFIG_SOC_SERIES_BEETLE */
#endif /* CONFIG_CLOCK_CONTROL */

	cfg->gpio_config_func(dev);

	return 0;
}

#define CMSDK_AHB_GPIO_DEVICE(n)						\
	static void gpio_cmsdk_port_##n##_config_func(const struct device *dev); \
										\
	static const struct gpio_cmsdk_ahb_cfg gpio_cmsdk_port_##n##_config = {	\
		.common = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},								\
		.port = ((volatile struct gpio_cmsdk_ahb *)DT_INST_REG_ADDR(n)),\
		.gpio_config_func = gpio_cmsdk_port_##n##_config_func,		\
		.gpio_cc_as = {.bus = CMSDK_AHB, .state = SOC_ACTIVE,		\
			       .device = DT_INST_REG_ADDR(n),},			\
		.gpio_cc_ss = {.bus = CMSDK_AHB, .state = SOC_SLEEP,		\
			       .device = DT_INST_REG_ADDR(n),},			\
		.gpio_cc_dss = {.bus = CMSDK_AHB, .state = SOC_DEEPSLEEP,	\
				.device = DT_INST_REG_ADDR(n),},		\
	};									\
										\
	static struct gpio_cmsdk_ahb_dev_data gpio_cmsdk_port_##n##_data;	\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			    gpio_cmsdk_ahb_init,				\
			    NULL,						\
			    &gpio_cmsdk_port_##n##_data,			\
			    &gpio_cmsdk_port_## n ##_config,			\
			    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,	\
			    &gpio_cmsdk_ahb_drv_api_funcs);			\
										\
	static void gpio_cmsdk_port_##n##_config_func(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    gpio_cmsdk_ahb_isr,					\
			    DEVICE_DT_INST_GET(n), 0);				\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(CMSDK_AHB_GPIO_DEVICE)
