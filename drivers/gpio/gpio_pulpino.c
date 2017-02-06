/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for the pulpino GPIO controller.
 */

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <gpio.h>
#include <misc/util.h>

#include "gpio_utils.h"

typedef void (*pulpino_cfg_func_t)(void);

/* pulpino GPIO register-set structure */
struct gpio_pulpino_t {
	uint32_t paddir;
	uint32_t padin;
	uint32_t padout;
	uint32_t inten;
	uint32_t inttype0;
	uint32_t inttype1;
	uint32_t intstatus;
};

struct gpio_pulpino_config {
	uint32_t              gpio_base_addr;
	pulpino_cfg_func_t    gpio_cfg_func;
};

struct gpio_pulpino_data {
	/* list of callbacks */
	sys_slist_t cb;
};

/* Helper Macros for GPIO */
#define DEV_GPIO_CFG(dev)						\
	((const struct gpio_pulpino_config * const)(dev)->config->config_info)
#define DEV_GPIO(dev)							\
	((volatile struct gpio_pulpino_t *)(DEV_GPIO_CFG(dev))->gpio_base_addr)
#define DEV_GPIO_DATA(dev)					\
	((struct gpio_pulpino_data *)(dev)->driver_data)

static void gpio_pulpino_irq_handler(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct gpio_pulpino_data *data = DEV_GPIO_DATA(dev);
	volatile struct gpio_pulpino_t *gpio = DEV_GPIO(dev);

	_gpio_fire_callbacks(&data->cb, dev, gpio->intstatus);
}

/**
 * @brief Configure pin
 *
 * @param dev Device structure
 * @param access_op Access operation
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_pulpino_config(struct device *dev,
			       int access_op,
			       uint32_t pin,
			       int flags)
{
	volatile struct gpio_pulpino_t *gpio = DEV_GPIO(dev);

	if (access_op != GPIO_ACCESS_BY_PIN)
		return -ENOTSUP;

	if (pin > 31)
		return -EINVAL;

	/* Configure pin as gpio */
	PULP_PADMUX |= (PULP_PAD_GPIO << pin);

	/* Configure gpio direction */
	if (flags & GPIO_DIR_OUT)
		gpio->paddir |= BIT(pin);
	else
		gpio->paddir &= ~BIT(pin);

	/*
	 * Configure interrupt if GPIO_INT is set.
	 * Here, we just configure the gpio interrupt behavior,
	 * we do not enable/disable interrupt for a particular
	 * gpio.
	 * Interrupt for a gpio is:
	 * 1) enabled only via a call to gpio_pulpino_enable_callback.
	 * 2) disabled only via a call to gpio_pulpino_disabled_callback.
	 */
	if (!(flags & GPIO_INT))
		return 0;

	/*
	 * Interrupt cannot be set for GPIO_DIR_OUT
	 */
	if (flags & GPIO_DIR_OUT)
		return -EINVAL;

	/* Double edge trigger not supported */
	if (flags & GPIO_INT_DOUBLE_EDGE)
		return -ENOTSUP;

	/* Edge or Level triggered ? */
	if (flags & GPIO_INT_EDGE)
		gpio->inttype1 |= BIT(pin);
	else
		gpio->inttype1 &= ~BIT(pin);

	/* Level High/Rising Edge ? */
	if (flags & GPIO_INT_ACTIVE_HIGH)
		gpio->inttype0 &= ~BIT(pin);
	else
		gpio->inttype0 |= BIT(pin);

	return 0;
}

/**
 * @brief Set the pin
 *
 * @param dev Device struct
 * @param access_op Access operation
 * @param pin The pin number
 * @param value Value to set (0 or 1)
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_pulpino_write(struct device *dev,
			      int access_op,
			      uint32_t pin,
			      uint32_t value)
{
	volatile struct gpio_pulpino_t *gpio = DEV_GPIO(dev);

	if (access_op != GPIO_ACCESS_BY_PIN)
		return -ENOTSUP;

	if (value)
		gpio->padout |= BIT(pin);
	else
		gpio->padout &= ~BIT(pin);

	return 0;
}

/**
 * @brief Read the pin
 *
 * @param dev Device struct
 * @param access_op Access operation
 * @param pin The pin number
 * @param value Value of input pin(s)
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_pulpino_read(struct device *dev,
			     int access_op,
			     uint32_t pin,
			     uint32_t *value)
{
	volatile struct gpio_pulpino_t *gpio = DEV_GPIO(dev);

	if (access_op != GPIO_ACCESS_BY_PIN)
		return -ENOTSUP;

	/*
	 * If gpio is configured as output,
	 * read gpio value from padout register,
	 * otherwise read gpio value from padin register
	 */
	if (gpio->paddir & BIT(pin))
		*value = !!(gpio->padout & BIT(pin));
	else
		*value = !!(gpio->padin & BIT(pin));

	return 0;
}

static int gpio_pulpino_manage_callback(struct device *dev,
					struct gpio_callback *callback,
					bool set)
{
	struct gpio_pulpino_data *data = DEV_GPIO_DATA(dev);

	_gpio_manage_callback(&data->cb, callback, set);

	return 0;
}

static int gpio_pulpino_enable_callback(struct device *dev,
					int access_op,
					uint32_t pin)
{
	volatile struct gpio_pulpino_t *gpio = DEV_GPIO(dev);

	if (access_op != GPIO_ACCESS_BY_PIN)
		return -ENOTSUP;

	/* Enable interrupt for pin */
	gpio->inten |= BIT(pin);

	return 0;
}

static int gpio_pulpino_disable_callback(struct device *dev,
					 int access_op,
					 uint32_t pin)
{
	volatile struct gpio_pulpino_t *gpio = DEV_GPIO(dev);

	if (access_op != GPIO_ACCESS_BY_PIN)
		return -ENOTSUP;

	/* Disable interrupt for pin */
	gpio->inten &= ~BIT(pin);

	return 0;
}

static const struct gpio_driver_api gpio_pulpino_driver = {
	.config              = gpio_pulpino_config,
	.write               = gpio_pulpino_write,
	.read                = gpio_pulpino_read,
	.manage_callback     = gpio_pulpino_manage_callback,
	.enable_callback     = gpio_pulpino_enable_callback,
	.disable_callback    = gpio_pulpino_disable_callback,
};

/**
 * @brief Initialize a GPIO controller
 *
 * Perform basic initialization of a GPIO controller
 *
 * @param dev GPIO device struct
 *
 * @return 0
 */
static int gpio_pulpino_init(struct device *dev)
{
	const struct gpio_pulpino_config *cfg = DEV_GPIO_CFG(dev);

	cfg->gpio_cfg_func();

	return 0;
}

static void gpio_pulpino_cfg_0(void);

static const struct gpio_pulpino_config gpio_pulpino_config0 = {
	.gpio_base_addr    = PULP_GPIO_0_BASE,
	.gpio_cfg_func     = gpio_pulpino_cfg_0,
};

static struct gpio_pulpino_data gpio_pulpino_data0;

DEVICE_AND_API_INIT(gpio_pulpino_0, "gpio0", gpio_pulpino_init,
		    &gpio_pulpino_data0, &gpio_pulpino_config0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_pulpino_driver);

static void gpio_pulpino_cfg_0(void)
{
	IRQ_CONNECT(PULP_GPIO_0_IRQ,
		    0,
		    gpio_pulpino_irq_handler,
		    DEVICE_GET(gpio_pulpino_0),
		    0);
	irq_enable(PULP_GPIO_0_IRQ);
}
