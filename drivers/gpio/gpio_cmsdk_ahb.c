/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

#include <device.h>
#include <errno.h>
#include <drivers/gpio.h>
#include <init.h>
#include <soc.h>
#include <clock_control/arm_clock_control.h>

#include "gpio_cmsdk_ahb.h"
#include "gpio_utils.h"

/**
 * @brief GPIO driver for ARM CMSDK AHB GPIO
 */

typedef void (*gpio_config_func_t)(struct device *port);

struct gpio_cmsdk_ahb_cfg {
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
	/* list of callbacks */
	sys_slist_t gpio_cb;
};

static int cmsdk_ahb_gpio_config(struct device *dev, u32_t mask, int flags)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

	/*
	 * Setup the pin direction
	 * Output Enable:
	 * 0 - Input
	 * 1 - Output
	 */
	if ((flags & GPIO_DIR_MASK) == GPIO_DIR_OUT) {
		cfg->port->outenableset = mask;
	} else {
		cfg->port->outenableclr = mask;
	}

	/* Setup interrupt config */
	if (flags & GPIO_INT) {
		if (flags & GPIO_INT_DOUBLE_EDGE) {
			return -ENOTSUP;
		} else {
			/*
			 * Interrupt type:
			 * 0 - LOW or HIGH level
			 * 1 - For falling or rising
			 */
			if (flags & GPIO_INT_EDGE) {
				cfg->port->inttypeclr = mask;
			} else {
				cfg->port->inttypeset = mask;
			}
			/*
			 * Interrupt polarity:
			 * 0 - Low level or falling edge
			 * 1 - High level or rising edge
			 */
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				cfg->port->intpolset = mask;
			} else {
				cfg->port->intpolclr = mask;
			}
		}
	}

	cfg->port->altfuncclr = mask;

	return 0;
}

/**
 * @brief Configure pin or port
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_cmsdk_ahb_config(struct device *dev, int access_op,
				 u32_t pin, int flags)
{
	int ret = 0;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		ret = cmsdk_ahb_gpio_config(dev, BIT(pin), flags);
		break;
	case GPIO_ACCESS_BY_PORT:
		ret = cmsdk_ahb_gpio_config(dev, (0xFFFF), flags);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

/**
 * @brief Set the pin or port output
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value to set (0 or 1)
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_cmsdk_ahb_write(struct device *dev, int access_op,
				u32_t pin, u32_t value)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;
	u32_t key;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		if (value) {
			/*
			 * The irq_lock() here is required to prevent concurrent
			 * callers to corrupt the pin states.
			 */
			key = irq_lock();
			/* set the pin */
			cfg->port->dataout |= BIT(pin);
			irq_unlock(key);
		} else {
			/*
			 * The irq_lock() here is required to prevent concurrent
			 * callers to corrupt the pin states.
			 */
			key = irq_lock();
			/* clear the pin */
			cfg->port->dataout &= ~(BIT(pin));
			irq_unlock(key);
		}
		break;
	case GPIO_ACCESS_BY_PORT:
		if (value) {
			/* set all pins */
			cfg->port->dataout = 0xFFFF;
		} else {
			/* clear all pins */
			cfg->port->dataout = 0x0;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Read the pin or port status
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value of input pin(s)
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_cmsdk_ahb_read(struct device *dev, int access_op,
			       u32_t pin, u32_t *value)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;

	*value = cfg->port->data;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		*value = (*value >> pin) & 0x1;
		break;
	case GPIO_ACCESS_BY_PORT:
		break;
	default:
		return -ENOTSUP;
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

	gpio_fire_callbacks(&data->gpio_cb, dev, int_stat);

	/* clear the port interrupts */
	cfg->port->intclear = 0xFFFFFFFF;
}

static int gpio_cmsdk_ahb_manage_callback(struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct gpio_cmsdk_ahb_dev_data *data = dev->driver_data;

	return gpio_manage_callback(&data->gpio_cb, callback, set);
}

static int gpio_cmsdk_ahb_enable_callback(struct device *dev,
					  int access_op, u32_t pin)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;
	u32_t mask;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		mask = BIT(pin);
		break;
	case GPIO_ACCESS_BY_PORT:
		mask = 0xFFFF;
		break;
	default:
		return -ENOTSUP;
	}

	cfg->port->intenset |= mask;

	return 0;
}

static int gpio_cmsdk_ahb_disable_callback(struct device *dev,
					   int access_op, u32_t pin)
{
	const struct gpio_cmsdk_ahb_cfg * const cfg = dev->config->config_info;
	u32_t mask;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		mask = BIT(pin);
		break;
	case GPIO_ACCESS_BY_PORT:
		mask = 0xFFFF;
		break;
	default:
		return -ENOTSUP;
	}

	cfg->port->intenclr |= mask;

	return 0;
}

static const struct gpio_driver_api gpio_cmsdk_ahb_drv_api_funcs = {
	.config = gpio_cmsdk_ahb_config,
	.write = gpio_cmsdk_ahb_write,
	.read = gpio_cmsdk_ahb_read,
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
