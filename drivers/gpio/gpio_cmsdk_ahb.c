/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_cmsdk_gpio

#include <kernel.h>

#include <device.h>
#include <errno.h>
#include <drivers/gpio.h>
#include <init.h>
#include <soc.h>
#include <drivers/clock_control/arm_clock_control.h>

#include "gpio_cmsdk_ahb.h"
#include "gpio_utils.h"

/**
 * @brief GPIO driver for ARM CMSDK AHB GPIO
 */

typedef void (*gpio_config_func_t)(struct device *port);

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

static int gpio_cmsdk_ahb_port_get_raw(struct device *dev, u32_t *value)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

	*value = cfg->port->data;

	return 0;
}

static int gpio_cmsdk_ahb_port_set_masked_raw(struct device *dev, u32_t mask,
					 u32_t value)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

	cfg->port->dataout = (cfg->port->dataout & ~mask) | (mask & value);

	return 0;
}

static int gpio_cmsdk_ahb_port_set_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

	cfg->port->dataout |= mask;

	return 0;
}

static int gpio_cmsdk_ahb_port_clear_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

	cfg->port->dataout &= ~mask;

	return 0;
}

static int gpio_cmsdk_ahb_port_toggle_bits(struct device *dev, u32_t mask)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

	cfg->port->dataout ^= mask;

	return 0;
}

static int cmsdk_ahb_gpio_config(struct device *dev, u32_t mask, gpio_flags_t flags)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

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
static int gpio_cmsdk_ahb_config(struct device *dev,
				 gpio_pin_t pin,
				 gpio_flags_t flags)
{
	return cmsdk_ahb_gpio_config(dev, BIT(pin), flags);
}

static int gpio_cmsdk_ahb_pin_interrupt_configure(struct device *dev,
		gpio_pin_t pin, enum gpio_int_mode mode,
		enum gpio_int_trig trig)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

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

		/* Level High or Edge Risising */
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

static void gpio_cmsdk_ahb_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;
	struct gpio_cmsdk_ahb_dev_data *data = dev->driver_data;
	u32_t int_stat;

	int_stat = cfg->port->intstatus;

	/* clear the port interrupts */
	cfg->port->intclear = int_stat;

	gpio_fire_callbacks(&data->gpio_cb, dev, int_stat);

}

static int gpio_cmsdk_ahb_manage_callback(struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct gpio_cmsdk_ahb_dev_data *data = dev->driver_data;

	return gpio_manage_callback(&data->gpio_cb, callback, set);
}

static int gpio_cmsdk_ahb_enable_callback(struct device *dev,
					  gpio_pin_t pin)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

	cfg->port->intenset |= BIT(pin);

	return 0;
}

static int gpio_cmsdk_ahb_disable_callback(struct device *dev,
					   gpio_pin_t pin)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

	cfg->port->intenclr |= BIT(pin);

	return 0;
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
	.enable_callback = gpio_cmsdk_ahb_enable_callback,
	.disable_callback = gpio_cmsdk_ahb_disable_callback,
};

/**
 * @brief Initialization function of GPIO
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_cmsdk_ahb_init(struct device *dev)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

#ifdef CONFIG_CLOCK_CONTROL
	/* Enable clock for subsystem */
	struct device *clk =
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

/* Port 0 */
#ifdef CONFIG_GPIO_CMSDK_AHB_PORT0
static void gpio_cmsdk_ahb_config_0(struct device *dev);

static const struct gpio_cmsdk_ahb_cfg gpio_cmsdk_ahb_0_cfg = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_PROP(0, ngpios)),
	},
	.port = ((volatile struct gpio_cmsdk_ahb *)DT_CMSDK_AHB_GPIO0),
	.gpio_config_func = gpio_cmsdk_ahb_config_0,
	.gpio_cc_as = {.bus = CMSDK_AHB, .state = SOC_ACTIVE,
		       .device = DT_CMSDK_AHB_GPIO0,},
	.gpio_cc_ss = {.bus = CMSDK_AHB, .state = SOC_SLEEP,
		       .device = DT_CMSDK_AHB_GPIO0,},
	.gpio_cc_dss = {.bus = CMSDK_AHB, .state = SOC_DEEPSLEEP,
			.device = DT_CMSDK_AHB_GPIO0,},
};

static struct gpio_cmsdk_ahb_dev_data gpio_cmsdk_ahb_0_data;

DEVICE_AND_API_INIT(gpio_cmsdk_ahb_0,
		    CONFIG_GPIO_CMSDK_AHB_PORT0_DEV_NAME,
		    gpio_cmsdk_ahb_init, &gpio_cmsdk_ahb_0_data,
		    &gpio_cmsdk_ahb_0_cfg, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_cmsdk_ahb_drv_api_funcs);

static void gpio_cmsdk_ahb_config_0(struct device *dev)
{
	IRQ_CONNECT(DT_IRQ_PORT0_ALL, CONFIG_GPIO_CMSDK_AHB_PORT0_IRQ_PRI,
		    gpio_cmsdk_ahb_isr,
		    DEVICE_GET(gpio_cmsdk_ahb_0), 0);
	irq_enable(DT_IRQ_PORT0_ALL);
}
#endif /* CONFIG_GPIO_CMSDK_AHB_PORT0 */

/* Port 1 */
#ifdef CONFIG_GPIO_CMSDK_AHB_PORT1
static void gpio_cmsdk_ahb_config_1(struct device *dev);

static const struct gpio_cmsdk_ahb_cfg gpio_cmsdk_ahb_1_cfg = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_PROP(1, ngpios)),
	},
	.port = ((volatile struct gpio_cmsdk_ahb *)DT_CMSDK_AHB_GPIO1),
	.gpio_config_func = gpio_cmsdk_ahb_config_1,
	.gpio_cc_as = {.bus = CMSDK_AHB, .state = SOC_ACTIVE,
		       .device = DT_CMSDK_AHB_GPIO1,},
	.gpio_cc_ss = {.bus = CMSDK_AHB, .state = SOC_SLEEP,
		       .device = DT_CMSDK_AHB_GPIO1,},
	.gpio_cc_dss = {.bus = CMSDK_AHB, .state = SOC_DEEPSLEEP,
			.device = DT_CMSDK_AHB_GPIO1,},
};

static struct gpio_cmsdk_ahb_dev_data gpio_cmsdk_ahb_1_data;

DEVICE_AND_API_INIT(gpio_cmsdk_ahb_1,
		    CONFIG_GPIO_CMSDK_AHB_PORT1_DEV_NAME,
		    gpio_cmsdk_ahb_init, &gpio_cmsdk_ahb_1_data,
		    &gpio_cmsdk_ahb_1_cfg, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_cmsdk_ahb_drv_api_funcs);

static void gpio_cmsdk_ahb_config_1(struct device *dev)
{
	IRQ_CONNECT(DT_IRQ_PORT1_ALL, CONFIG_GPIO_CMSDK_AHB_PORT1_IRQ_PRI,
		    gpio_cmsdk_ahb_isr,
		    DEVICE_GET(gpio_cmsdk_ahb_1), 0);
	irq_enable(DT_IRQ_PORT1_ALL);
}
#endif /* CONFIG_GPIO_CMSDK_AHB_PORT1 */

/* Port 2 */
#ifdef CONFIG_GPIO_CMSDK_AHB_PORT2
static void gpio_cmsdk_ahb_config_2(struct device *dev);

static const struct gpio_cmsdk_ahb_cfg gpio_cmsdk_ahb_2_cfg = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_PROP(2, ngpios)),
	},
	.port = ((volatile struct gpio_cmsdk_ahb *)DT_CMSDK_AHB_GPIO2),
	.gpio_config_func = gpio_cmsdk_ahb_config_2,
	.gpio_cc_as = {.bus = CMSDK_AHB, .state = SOC_ACTIVE,
		       .device = DT_CMSDK_AHB_GPIO2,},
	.gpio_cc_ss = {.bus = CMSDK_AHB, .state = SOC_SLEEP,
		       .device = DT_CMSDK_AHB_GPIO2,},
	.gpio_cc_dss = {.bus = CMSDK_AHB, .state = SOC_DEEPSLEEP,
			.device = DT_CMSDK_AHB_GPIO2,},
};

static struct gpio_cmsdk_ahb_dev_data gpio_cmsdk_ahb_2_data;

DEVICE_AND_API_INIT(gpio_cmsdk_ahb_2,
		    CONFIG_GPIO_CMSDK_AHB_PORT2_DEV_NAME,
		    gpio_cmsdk_ahb_init, &gpio_cmsdk_ahb_2_data,
		    &gpio_cmsdk_ahb_2_cfg, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_cmsdk_ahb_drv_api_funcs);

static void gpio_cmsdk_ahb_config_2(struct device *dev)
{
	IRQ_CONNECT(DT_IRQ_PORT2_ALL, CONFIG_GPIO_CMSDK_AHB_PORT2_IRQ_PRI,
		    gpio_cmsdk_ahb_isr,
		    DEVICE_GET(gpio_cmsdk_ahb_2), 0);
	irq_enable(DT_IRQ_PORT2_ALL);
}
#endif /* CONFIG_GPIO_CMSDK_AHB_PORT2 */

/* Port 3 */
#ifdef CONFIG_GPIO_CMSDK_AHB_PORT3
static void gpio_cmsdk_ahb_config_3(struct device *dev);

static const struct gpio_cmsdk_ahb_cfg gpio_cmsdk_ahb_3_cfg = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_PROP(3, ngpios)),
	},
	.port = ((volatile struct gpio_cmsdk_ahb *)DT_CMSDK_AHB_GPIO3),
	.gpio_config_func = gpio_cmsdk_ahb_config_3,
	.gpio_cc_as = {.bus = CMSDK_AHB, .state = SOC_ACTIVE,
		       .device = DT_CMSDK_AHB_GPIO3,},
	.gpio_cc_ss = {.bus = CMSDK_AHB, .state = SOC_SLEEP,
		       .device = DT_CMSDK_AHB_GPIO3,},
	.gpio_cc_dss = {.bus = CMSDK_AHB, .state = SOC_DEEPSLEEP,
			.device = DT_CMSDK_AHB_GPIO3,},
};

static struct gpio_cmsdk_ahb_dev_data gpio_cmsdk_ahb_3_data;

DEVICE_AND_API_INIT(gpio_cmsdk_ahb_3,
		    CONFIG_GPIO_CMSDK_AHB_PORT3_DEV_NAME,
		    gpio_cmsdk_ahb_init, &gpio_cmsdk_ahb_3_data,
		    &gpio_cmsdk_ahb_3_cfg, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_cmsdk_ahb_drv_api_funcs);

static void gpio_cmsdk_ahb_config_3(struct device *dev)
{
	IRQ_CONNECT(DT_IRQ_PORT3_ALL, CONFIG_GPIO_CMSDK_AHB_PORT3_IRQ_PRI,
		    gpio_cmsdk_ahb_isr,
		    DEVICE_GET(gpio_cmsdk_ahb_3), 0);
	irq_enable(DT_IRQ_PORT3_ALL);
}
#endif /* CONFIG_GPIO_CMSDK_AHB_PORT3 */
