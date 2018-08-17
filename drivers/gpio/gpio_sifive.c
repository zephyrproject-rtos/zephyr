/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file GPIO driver for the SiFive Freedom Processor
 */

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <gpio.h>
#include <misc/util.h>

#include "gpio_utils.h"

typedef void (*sifive_cfg_func_t)(void);

/* sifive GPIO register-set structure */
struct gpio_sifive_t {
	unsigned int in_val;
	unsigned int in_en;
	unsigned int out_en;
	unsigned int out_val;
	unsigned int pue;
	unsigned int ds;
	unsigned int rise_ie;
	unsigned int rise_ip;
	unsigned int fall_ie;
	unsigned int fall_ip;
	unsigned int high_ie;
	unsigned int high_ip;
	unsigned int low_ie;
	unsigned int low_ip;
	unsigned int iof_en;
	unsigned int iof_sel;
	unsigned int invert;
};

struct gpio_sifive_config {
	u32_t            gpio_base_addr;
	u32_t            gpio_irq_base;
	sifive_cfg_func_t    gpio_cfg_func;
};

struct gpio_sifive_data {
	/* list of callbacks */
	sys_slist_t cb;
};

/* Helper Macros for GPIO */
#define DEV_GPIO_CFG(dev)						\
	((const struct gpio_sifive_config * const)(dev)->config->config_info)
#define DEV_GPIO(dev)							\
	((volatile struct gpio_sifive_t *)(DEV_GPIO_CFG(dev))->gpio_base_addr)
#define DEV_GPIO_DATA(dev)				\
	((struct gpio_sifive_data *)(dev)->driver_data)

static void gpio_sifive_irq_handler(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct gpio_sifive_data *data = DEV_GPIO_DATA(dev);
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);
	const struct gpio_sifive_config *cfg = DEV_GPIO_CFG(dev);
	int pin_mask;

	/* Get the pin number generating the interrupt */
	pin_mask = 1 << (riscv_plic_get_irq() -
			 (cfg->gpio_irq_base - RISCV_MAX_GENERIC_IRQ));

	/* Call the corresponding callback registered for the pin */
	_gpio_fire_callbacks(&data->cb, dev, pin_mask);

	/*
	 * Write to either the rise_ip, fall_ip, high_ip or low_ip registers
	 * to indicate to GPIO controller that interrupt for the corresponding
	 * pin has been handled.
	 */
	if (gpio->rise_ip & pin_mask)
		gpio->rise_ip = pin_mask;
	else if (gpio->fall_ip & pin_mask)
		gpio->fall_ip = pin_mask;
	else if (gpio->high_ip & pin_mask)
		gpio->high_ip = pin_mask;
	else if (gpio->low_ip & pin_mask)
		gpio->low_ip = pin_mask;
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
static int gpio_sifive_config(struct device *dev,
			     int access_op,
			     u32_t pin,
			     int flags)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);

	if (access_op != GPIO_ACCESS_BY_PIN)
		return -ENOTSUP;

	if (pin >= SIFIVE_PINMUX_PINS)
		return -EINVAL;

	/* Configure gpio direction */
	if (flags & GPIO_DIR_OUT) {
		gpio->in_en &= ~BIT(pin);
		gpio->out_en |= BIT(pin);

		/*
		 * Account for polarity only for GPIO_DIR_OUT.
		 * invert register handles only output gpios
		 */
		if (flags & GPIO_POL_INV)
			gpio->invert |= BIT(pin);
		else
			gpio->invert &= ~BIT(pin);
	} else {
		gpio->out_en &= ~BIT(pin);
		gpio->in_en |= BIT(pin);

		/* Polarity inversion is not supported for input gpio */
		if (flags & GPIO_POL_INV)
			return -EINVAL;

		/*
		 * Pull-up can be configured only for input gpios.
		 * Only Pull-up can be enabled or disabled.
		 */
		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN)
			return -EINVAL;

		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP)
			gpio->pue |= BIT(pin);
		else
			gpio->pue &= ~BIT(pin);
	}

	/*
	 * Configure interrupt if GPIO_INT is set.
	 * Here, we just configure the gpio interrupt behavior,
	 * we do not enable/disable interrupt for a particular
	 * gpio.
	 * Interrupt for a gpio is:
	 * 1) enabled only via a call to gpio_sifive_enable_callback.
	 * 2) disabled only via a call to gpio_sifive_disabled_callback.
	 */
	if (!(flags & GPIO_INT))
		return 0;

	/*
	 * Interrupt cannot be set for GPIO_DIR_OUT
	 */
	if (flags & GPIO_DIR_OUT)
		return -EINVAL;

	/* Edge or Level triggered ? */
	if (flags & GPIO_INT_EDGE) {
		gpio->high_ie &= ~BIT(pin);
		gpio->low_ie &= ~BIT(pin);

		/* Rising Edge, Falling Edge or Double Edge ? */
		if (flags & GPIO_INT_DOUBLE_EDGE) {
			gpio->rise_ie |= BIT(pin);
			gpio->fall_ie |= BIT(pin);
		} else if (flags & GPIO_INT_ACTIVE_HIGH) {
			gpio->rise_ie |= BIT(pin);
			gpio->fall_ie &= ~BIT(pin);
		} else {
			gpio->rise_ie &= ~BIT(pin);
			gpio->fall_ie |= BIT(pin);
		}
	} else {
		gpio->rise_ie &= ~BIT(pin);
		gpio->fall_ie &= ~BIT(pin);

		/* Level High ? */
		if (flags & GPIO_INT_ACTIVE_HIGH) {
			gpio->high_ie |= BIT(pin);
			gpio->low_ie &= ~BIT(pin);
		} else {
			gpio->high_ie &= ~BIT(pin);
			gpio->low_ie |= BIT(pin);
		}
	}

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
static int gpio_sifive_write(struct device *dev,
			    int access_op,
			    u32_t pin,
			    u32_t value)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);

	if (access_op != GPIO_ACCESS_BY_PIN)
		return -ENOTSUP;

	if (pin >= SIFIVE_PINMUX_PINS)
		return -EINVAL;

	/* If pin is configured as input return with error */
	if (gpio->in_en & BIT(pin))
		return -EINVAL;

	if (value)
		gpio->out_val |= BIT(pin);
	else
		gpio->out_val &= ~BIT(pin);

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
static int gpio_sifive_read(struct device *dev,
			   int access_op,
			   u32_t pin,
			   u32_t *value)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);

	if (access_op != GPIO_ACCESS_BY_PIN)
		return -ENOTSUP;

	if (pin >= SIFIVE_PINMUX_PINS)
		return -EINVAL;

	/*
	 * If gpio is configured as output,
	 * read gpio value from out_val register,
	 * otherwise read gpio value from in_val register
	 */
	if (gpio->out_en & BIT(pin))
		*value = !!(gpio->out_val & BIT(pin));
	else
		*value = !!(gpio->in_val & BIT(pin));

	return 0;
}

static int gpio_sifive_manage_callback(struct device *dev,
				      struct gpio_callback *callback,
				      bool set)
{
	struct gpio_sifive_data *data = DEV_GPIO_DATA(dev);

	_gpio_manage_callback(&data->cb, callback, set);

	return 0;
}

static int gpio_sifive_enable_callback(struct device *dev,
				      int access_op,
				      u32_t pin)
{
	const struct gpio_sifive_config *cfg = DEV_GPIO_CFG(dev);

	if (access_op != GPIO_ACCESS_BY_PIN)
		return -ENOTSUP;

	if (pin >= SIFIVE_PINMUX_PINS)
		return -EINVAL;

	/* Enable interrupt for the pin at PLIC level */
	irq_enable(cfg->gpio_irq_base + pin);

	return 0;
}

static int gpio_sifive_disable_callback(struct device *dev,
				       int access_op,
				       u32_t pin)
{
	const struct gpio_sifive_config *cfg = DEV_GPIO_CFG(dev);

	if (access_op != GPIO_ACCESS_BY_PIN)
		return -ENOTSUP;

	if (pin >= SIFIVE_PINMUX_PINS)
		return -EINVAL;

	/* Disable interrupt for the pin at PLIC level */
	irq_disable(cfg->gpio_irq_base + pin);

	return 0;
}

static const struct gpio_driver_api gpio_sifive_driver = {
	.config              = gpio_sifive_config,
	.write               = gpio_sifive_write,
	.read                = gpio_sifive_read,
	.manage_callback     = gpio_sifive_manage_callback,
	.enable_callback     = gpio_sifive_enable_callback,
	.disable_callback    = gpio_sifive_disable_callback,
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
static int gpio_sifive_init(struct device *dev)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);
	const struct gpio_sifive_config *cfg = DEV_GPIO_CFG(dev);

	/* Ensure that all gpio registers are reset to 0 initially */
	gpio->in_en   = 0;
	gpio->out_en  = 0;
	gpio->pue     = 0;
	gpio->rise_ie = 0;
	gpio->fall_ie = 0;
	gpio->high_ie = 0;
	gpio->low_ie  = 0;
	gpio->invert  = 0;

	/* Setup IRQ handler for each gpio pin */
	cfg->gpio_cfg_func();

	return 0;
}

static void gpio_sifive_cfg_0(void);

static const struct gpio_sifive_config gpio_sifive_config0 = {
	.gpio_base_addr    = CONFIG_SIFIVE_GPIO_0_BASE_ADDR,
	.gpio_irq_base     = CONFIG_SIFIVE_GPIO_0_IRQ_0,
	.gpio_cfg_func     = gpio_sifive_cfg_0,
};

static struct gpio_sifive_data gpio_sifive_data0;

DEVICE_AND_API_INIT(gpio_sifive_0, CONFIG_GPIO_SIFIVE_GPIO_NAME,
		    gpio_sifive_init,
		    &gpio_sifive_data0, &gpio_sifive_config0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_sifive_driver);

static void gpio_sifive_cfg_0(void)
{
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_0
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_0,
		    CONFIG_GPIO_SIFIVE_0_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_1
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_1,
		    CONFIG_GPIO_SIFIVE_1_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_2
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_2,
		    CONFIG_GPIO_SIFIVE_2_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_3
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_3,
		    CONFIG_GPIO_SIFIVE_3_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_4
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_4,
		    CONFIG_GPIO_SIFIVE_4_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_5
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_5,
		    CONFIG_GPIO_SIFIVE_5_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_6
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_6,
		    CONFIG_GPIO_SIFIVE_6_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_7
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_7,
		    CONFIG_GPIO_SIFIVE_7_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_8
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_8,
		    CONFIG_GPIO_SIFIVE_8_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_9
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_9,
		    CONFIG_GPIO_SIFIVE_9_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_10
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_10,
		    CONFIG_GPIO_SIFIVE_10_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_11
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_11,
		    CONFIG_GPIO_SIFIVE_11_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_12
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_12,
		    CONFIG_GPIO_SIFIVE_12_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_13
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_13,
		    CONFIG_GPIO_SIFIVE_13_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_14
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_14,
		    CONFIG_GPIO_SIFIVE_14_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_15
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_15,
		    CONFIG_GPIO_SIFIVE_15_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_16
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_16,
		    CONFIG_GPIO_SIFIVE_16_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_17
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_17,
		    CONFIG_GPIO_SIFIVE_17_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_18
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_18,
		    CONFIG_GPIO_SIFIVE_18_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_19
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_19,
		    CONFIG_GPIO_SIFIVE_19_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_20
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_20,
		    CONFIG_GPIO_SIFIVE_20_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_21
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_21,
		    CONFIG_GPIO_SIFIVE_21_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_22
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_22,
		    CONFIG_GPIO_SIFIVE_22_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_23
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_23,
		    CONFIG_GPIO_SIFIVE_23_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_24
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_24,
		    CONFIG_GPIO_SIFIVE_24_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_25
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_25,
		    CONFIG_GPIO_SIFIVE_25_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_26
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_26,
		    CONFIG_GPIO_SIFIVE_26_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_27
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_27,
		    CONFIG_GPIO_SIFIVE_27_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_28
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_28,
		    CONFIG_GPIO_SIFIVE_28_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_29
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_29,
		    CONFIG_GPIO_SIFIVE_29_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_30
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_30,
		    CONFIG_GPIO_SIFIVE_30_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
#ifdef CONFIG_SIFIVE_GPIO_0_IRQ_31
	IRQ_CONNECT(CONFIG_SIFIVE_GPIO_0_IRQ_31,
		    CONFIG_GPIO_SIFIVE_31_PRIORITY,
		    gpio_sifive_irq_handler,
		    DEVICE_GET(gpio_sifive_0),
		    0);
#endif
}
