/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <gpio.h>
#include <clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <pinmux.h>
#include <misc/util.h>
#include <interrupt_controller/exti_stm32.h>

#include "gpio_stm32.h"
#include "gpio_utils.h"

/**
 * @brief Common GPIO driver for STM32 MCUs. Each SoC must implement a
 * SoC specific integration glue
 */

/**
 * @brief EXTI interrupt callback
 */
static void gpio_stm32_isr(int line, void *arg)
{
	struct device *dev = arg;
	struct gpio_stm32_data *data = dev->driver_data;

	if (BIT(line) & data->cb_pins) {
		_gpio_fire_callbacks(&data->cb, dev, BIT(line));
	}
}

/**
 * @brief Configure pin or port
 */
static int gpio_stm32_config(struct device *dev, int access_op,
			     u32_t pin, int flags)
{
	const struct gpio_stm32_config *cfg = dev->config->config_info;
	int pincfg;
	int map_res;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	/* figure out if we can map the requested GPIO
	 * configuration
	 */
	map_res = stm32_gpio_flags_to_conf(flags, &pincfg);
	if (map_res) {
		return map_res;
	}

	if (stm32_gpio_configure(cfg->base, pin, pincfg, 0)) {
		return -EIO;
	}

	if (flags & GPIO_INT) {
		stm32_exti_set_callback(pin, gpio_stm32_isr, dev);

		stm32_gpio_enable_int(cfg->port, pin);

		if (flags & GPIO_INT_EDGE) {
			int edge = 0;

			if (flags & GPIO_INT_DOUBLE_EDGE) {
				edge = STM32_EXTI_TRIG_RISING |
					STM32_EXTI_TRIG_FALLING;
			} else if (flags & GPIO_INT_ACTIVE_HIGH) {
				edge = STM32_EXTI_TRIG_RISING;
			} else {
				edge = STM32_EXTI_TRIG_FALLING;
			}

			stm32_exti_trigger(pin, edge);
		}

		stm32_exti_enable(pin);
	}

	return 0;
}

/**
 * @brief Set the pin or port output
 */
static int gpio_stm32_write(struct device *dev, int access_op,
			    u32_t pin, u32_t value)
{
	const struct gpio_stm32_config *cfg = dev->config->config_info;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	return stm32_gpio_set(cfg->base, pin, value);
}

/**
 * @brief Read the pin or port status
 */
static int gpio_stm32_read(struct device *dev, int access_op,
			   u32_t pin, u32_t *value)
{
	const struct gpio_stm32_config *cfg = dev->config->config_info;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	*value = stm32_gpio_get(cfg->base, pin);

	return 0;
}

static int gpio_stm32_manage_callback(struct device *dev,
				      struct gpio_callback *callback,
				      bool set)
{
	struct gpio_stm32_data *data = dev->driver_data;

	_gpio_manage_callback(&data->cb, callback, set);

	return 0;
}

static int gpio_stm32_enable_callback(struct device *dev,
				      int access_op, u32_t pin)
{
	struct gpio_stm32_data *data = dev->driver_data;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	data->cb_pins |= BIT(pin);

	return 0;
}

static int gpio_stm32_disable_callback(struct device *dev,
				       int access_op, u32_t pin)
{
	struct gpio_stm32_data *data = dev->driver_data;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	data->cb_pins &= ~BIT(pin);

	return 0;
}

static const struct gpio_driver_api gpio_stm32_driver = {
	.config = gpio_stm32_config,
	.write = gpio_stm32_write,
	.read = gpio_stm32_read,
	.manage_callback = gpio_stm32_manage_callback,
	.enable_callback = gpio_stm32_enable_callback,
	.disable_callback = gpio_stm32_disable_callback,

};

/**
 * @brief Initialize GPIO port
 *
 * Perform basic initialization of a GPIO port. The code will
 * enable the clock for corresponding peripheral.
 *
 * @param dev GPIO device struct
 *
 * @return 0
 */
static int gpio_stm32_init(struct device *device)
{
	const struct gpio_stm32_config *cfg = device->config->config_info;

	/* enable clock for subsystem */
	struct device *clk =
		device_get_binding(STM32_CLOCK_CONTROL_NAME);

	clock_control_on(clk, (clock_control_subsys_t *) &cfg->pclken);

	return 0;
}


#define GPIO_DEVICE_INIT(__name, __suffix, __base_addr, __port, __cenr, __bus) \
static const struct gpio_stm32_config gpio_stm32_cfg_## __suffix = {	\
	.base = (u32_t *)__base_addr,				\
	.port = __port,							\
	.pclken = { .bus = __bus, .enr = __cenr }			\
};									\
static struct gpio_stm32_data gpio_stm32_data_## __suffix;		\
DEVICE_AND_API_INIT(gpio_stm32_## __suffix,				\
		    __name,						\
		    gpio_stm32_init,					\
		    &gpio_stm32_data_## __suffix,			\
		    &gpio_stm32_cfg_## __suffix,			\
		    POST_KERNEL,					\
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,			\
		    &gpio_stm32_driver);


#ifdef CONFIG_SOC_SERIES_STM32F1X
	/* On STM32F1 series, AFIO should be clocked to access GPIOs */
#define GPIO_DEVICE_INIT_STM32(__suffix, __SUFFIX)			\
	GPIO_DEVICE_INIT("GPIO" #__SUFFIX, __suffix,			\
			 GPIO##__SUFFIX##_BASE, STM32_PORT##__SUFFIX,	\
			 LL_APB2_GRP1_PERIPH_AFIO |			\
			 STM32_PERIPH_GPIO##__SUFFIX,			\
			 STM32_CLOCK_BUS_GPIO)
#else
#define GPIO_DEVICE_INIT_STM32(__suffix, __SUFFIX)			\
	GPIO_DEVICE_INIT("GPIO" #__SUFFIX, __suffix,			\
			 GPIO##__SUFFIX##_BASE, STM32_PORT##__SUFFIX,	\
			 STM32_PERIPH_GPIO##__SUFFIX,			\
			 STM32_CLOCK_BUS_GPIO)
#endif /* CONFIG_SOC_SERIES_STM32F1X */

#ifdef CONFIG_GPIO_STM32_PORTA
GPIO_DEVICE_INIT_STM32(a, A);
#endif /* CONFIG_GPIO_STM32_PORTA */

#ifdef CONFIG_GPIO_STM32_PORTB
GPIO_DEVICE_INIT_STM32(b, B);
#endif /* CONFIG_GPIO_STM32_PORTB */

#ifdef CONFIG_GPIO_STM32_PORTC
GPIO_DEVICE_INIT_STM32(c, C);
#endif /* CONFIG_GPIO_STM32_PORTC */

#ifdef CONFIG_GPIO_STM32_PORTD
GPIO_DEVICE_INIT_STM32(d, D);
#endif /* CONFIG_GPIO_STM32_PORTD */

#ifdef CONFIG_GPIO_STM32_PORTE
GPIO_DEVICE_INIT_STM32(e, E);
#endif /* CONFIG_GPIO_STM32_PORTE */

#ifdef CONFIG_GPIO_STM32_PORTF
GPIO_DEVICE_INIT_STM32(f, F);
#endif /* CONFIG_GPIO_STM32_PORTF */

#ifdef CONFIG_GPIO_STM32_PORTG
GPIO_DEVICE_INIT_STM32(g, G);
#endif /* CONFIG_GPIO_STM32_PORTG */

#ifdef CONFIG_GPIO_STM32_PORTH
GPIO_DEVICE_INIT_STM32(h, H);
#endif /* CONFIG_GPIO_STM32_PORTH */

#ifdef CONFIG_GPIO_STM32_PORTI
GPIO_DEVICE_INIT_STM32(i, I);
#endif /* CONFIG_GPIO_STM32_PORTI */

#ifdef CONFIG_GPIO_STM32_PORTJ
GPIO_DEVICE_INIT_STM32(j, J);
#endif /* CONFIG_GPIO_STM32_PORTJ */

#ifdef CONFIG_GPIO_STM32_PORTK
GPIO_DEVICE_INIT_STM32(k, K);
#endif /* CONFIG_GPIO_STM32_PORTK */
