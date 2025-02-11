/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifive_gpio0

/**
 * @file GPIO driver for the SiFive Freedom Processor
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

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
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uintptr_t            gpio_base_addr;
	/* multi-level encoded interrupt corresponding to pin 0 */
	uint32_t                gpio_irq_base;
	sifive_cfg_func_t    gpio_cfg_func;
};

struct gpio_sifive_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* list of callbacks */
	sys_slist_t cb;
};

/* Helper Macros for GPIO */
#define DEV_GPIO_CFG(dev)						\
	((const struct gpio_sifive_config * const)(dev)->config)
#define DEV_GPIO(dev)							\
	((volatile struct gpio_sifive_t *)(DEV_GPIO_CFG(dev))->gpio_base_addr)
#define DEV_GPIO_DATA(dev)				\
	((struct gpio_sifive_data *)(dev)->data)


/* Given gpio_irq_base and the pin number, return the IRQ number for the pin */
static inline unsigned int gpio_sifive_pin_irq(unsigned int base_irq, int pin)
{
	unsigned int level = irq_get_level(base_irq);
	unsigned int pin_irq = 0;

	if (level == 1) {
		pin_irq = base_irq + pin;
	} else if (level == 2) {
		pin_irq = base_irq + (pin << CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	}

	return pin_irq;
}

/* Given the PLIC source number, return the number of the GPIO pin associated
 * with the interrupt
 */
static inline int gpio_sifive_plic_to_pin(unsigned int base_irq, int plic_irq)
{
	unsigned int level = irq_get_level(base_irq);

	if (level == 2) {
		base_irq = irq_from_level_2(base_irq);
	}

	return (plic_irq - base_irq);
}

static void gpio_sifive_irq_handler(const struct device *dev)
{
	struct gpio_sifive_data *data = DEV_GPIO_DATA(dev);
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);
	const struct gpio_sifive_config *cfg = DEV_GPIO_CFG(dev);

	/* Calculate pin and mask from base level 2 line */
	uint8_t pin = 1 + (riscv_plic_get_irq() -
			   (uint8_t)(cfg->gpio_irq_base >> CONFIG_1ST_LEVEL_INTERRUPT_BITS));

	/* This peripheral tracks each condition separately: a
	 * transition from low to high will mark the pending bit for
	 * both rise and high, while low will probably be set from the
	 * previous state.
	 *
	 * It is certainly possible, especially on double-edge, that
	 * multiple conditions are present.  However, there is no way
	 * to tell which one occurred first, and no provision to
	 * indicate which one occurred in the callback.
	 *
	 * Clear all the conditions so we only invoke the callback
	 * once.  Level conditions will remain set after clear.
	 */
	gpio->rise_ip = BIT(pin);
	gpio->fall_ip = BIT(pin);
	gpio->high_ip = BIT(pin);
	gpio->low_ip = BIT(pin);

	/* Call the corresponding callback registered for the pin */
	gpio_fire_callbacks(&data->cb, dev, BIT(pin));
}

/**
 * @brief Configure pin
 *
 * @param dev Device structure
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_sifive_config(const struct device *dev,
			      gpio_pin_t pin,
			      gpio_flags_t flags)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);

	/* We cannot support open-source open-drain configuration */
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	/* We only support pull-ups, not pull-downs */
	if ((flags & GPIO_PULL_DOWN) != 0) {
		return -ENOTSUP;
	}

	/* Set pull-up if requested */
	WRITE_BIT(gpio->pue, pin, flags & GPIO_PULL_UP);

	/* Set the initial output value before enabling output to avoid
	 * glitches
	 */
	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
		gpio->out_val |= BIT(pin);
	}
	if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
		gpio->out_val &= ~BIT(pin);
	}

	/* Enable input/output */
	WRITE_BIT(gpio->out_en, pin, flags & GPIO_OUTPUT);
	WRITE_BIT(gpio->in_en, pin, flags & GPIO_INPUT);

	return 0;
}

static int gpio_sifive_port_get_raw(const struct device *dev,
				    gpio_port_value_t *value)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);

	*value = gpio->in_val;

	return 0;
}

static int gpio_sifive_port_set_masked_raw(const struct device *dev,
					   gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);

	gpio->out_val = (gpio->out_val & ~mask) | (value & mask);

	return 0;
}

static int gpio_sifive_port_set_bits_raw(const struct device *dev,
					 gpio_port_pins_t mask)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);

	gpio->out_val |= mask;

	return 0;
}

static int gpio_sifive_port_clear_bits_raw(const struct device *dev,
					   gpio_port_pins_t mask)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);

	gpio->out_val &= ~mask;

	return 0;
}

static int gpio_sifive_port_toggle_bits(const struct device *dev,
					gpio_port_pins_t mask)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);

	gpio->out_val ^= mask;

	return 0;
}

static int gpio_sifive_pin_interrupt_configure(const struct device *dev,
					       gpio_pin_t pin,
					       enum gpio_int_mode mode,
					       enum gpio_int_trig trig)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);
	const struct gpio_sifive_config *cfg = DEV_GPIO_CFG(dev);

	gpio->rise_ie &= ~BIT(pin);
	gpio->fall_ie &= ~BIT(pin);
	gpio->high_ie &= ~BIT(pin);
	gpio->low_ie  &= ~BIT(pin);

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		irq_disable(gpio_sifive_pin_irq(cfg->gpio_irq_base, pin));
		break;
	case GPIO_INT_MODE_LEVEL:
		/* Board supports both levels, but Zephyr does not. */
		if (trig == GPIO_INT_TRIG_HIGH) {
			gpio->high_ip = BIT(pin);
			gpio->high_ie |= BIT(pin);
		} else {
			__ASSERT_NO_MSG(trig == GPIO_INT_TRIG_LOW);
			gpio->low_ip = BIT(pin);
			gpio->low_ie  |= BIT(pin);
		}
		irq_enable(gpio_sifive_pin_irq(cfg->gpio_irq_base, pin));
		break;
	case GPIO_INT_MODE_EDGE:
		__ASSERT_NO_MSG(GPIO_INT_TRIG_BOTH ==
				(GPIO_INT_LOW_0 | GPIO_INT_HIGH_1));

		if ((trig & GPIO_INT_HIGH_1) != 0) {
			gpio->rise_ip = BIT(pin);
			gpio->rise_ie |= BIT(pin);
		}
		if ((trig & GPIO_INT_LOW_0) != 0) {
			gpio->fall_ip = BIT(pin);
			gpio->fall_ie |= BIT(pin);
		}
		irq_enable(gpio_sifive_pin_irq(cfg->gpio_irq_base, pin));
		break;
	default:
		__ASSERT(false, "Invalid MODE %d passed to driver", mode);
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_sifive_manage_callback(const struct device *dev,
				       struct gpio_callback *callback,
				       bool set)
{
	struct gpio_sifive_data *data = DEV_GPIO_DATA(dev);

	return gpio_manage_callback(&data->cb, callback, set);
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_sifive_port_get_dir(const struct device *dev, gpio_port_pins_t map,
				    gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_sifive_config *cfg = DEV_GPIO_CFG(dev);

	map &= cfg->common.port_pin_mask;

	if (inputs != NULL) {
		*inputs = map & DEV_GPIO(dev)->in_en;
	}

	if (outputs != NULL) {
		*outputs = map & DEV_GPIO(dev)->out_en;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static const struct gpio_driver_api gpio_sifive_driver = {
	.pin_configure           = gpio_sifive_config,
	.port_get_raw            = gpio_sifive_port_get_raw,
	.port_set_masked_raw     = gpio_sifive_port_set_masked_raw,
	.port_set_bits_raw       = gpio_sifive_port_set_bits_raw,
	.port_clear_bits_raw     = gpio_sifive_port_clear_bits_raw,
	.port_toggle_bits        = gpio_sifive_port_toggle_bits,
	.pin_interrupt_configure = gpio_sifive_pin_interrupt_configure,
	.manage_callback         = gpio_sifive_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction      = gpio_sifive_port_get_dir,
#endif /* CONFIG_GPIO_GET_DIRECTION */
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
static int gpio_sifive_init(const struct device *dev)
{
	volatile struct gpio_sifive_t *gpio = DEV_GPIO(dev);
	const struct gpio_sifive_config *cfg = DEV_GPIO_CFG(dev);

	/* Ensure that all gpio registers are reset to 0 initially */
	gpio->in_en   = 0U;
	gpio->out_en  = 0U;
	gpio->pue     = 0U;
	gpio->rise_ie = 0U;
	gpio->fall_ie = 0U;
	gpio->high_ie = 0U;
	gpio->low_ie  = 0U;
	gpio->iof_en  = 0U;
	gpio->iof_sel = 0U;
	gpio->invert  = 0U;

	/* Setup IRQ handler for each gpio pin */
	cfg->gpio_cfg_func();

	return 0;
}

static void gpio_sifive_cfg_0(void);

static const struct gpio_sifive_config gpio_sifive_config0 = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0),
	},
	.gpio_base_addr = DT_INST_REG_ADDR(0),
	.gpio_irq_base  = DT_INST_IRQN(0),
	.gpio_cfg_func  = gpio_sifive_cfg_0,
};

static struct gpio_sifive_data gpio_sifive_data0;

DEVICE_DT_INST_DEFINE(0,
		    gpio_sifive_init,
		    NULL,
		    &gpio_sifive_data0, &gpio_sifive_config0,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_sifive_driver);

#define		IRQ_INIT(n)					\
IRQ_CONNECT(DT_INST_IRQN_BY_IDX(0, n),				\
		DT_INST_IRQ_BY_IDX(0, n, priority),		\
		gpio_sifive_irq_handler,			\
		DEVICE_DT_INST_GET(0),				\
		0);

static void gpio_sifive_cfg_0(void)
{
#if DT_INST_IRQ_HAS_IDX(0, 0)
	IRQ_INIT(0);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 1)
	IRQ_INIT(1);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 2)
	IRQ_INIT(2);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 3)
	IRQ_INIT(3);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 4)
	IRQ_INIT(4);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 5)
	IRQ_INIT(5);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 6)
	IRQ_INIT(6);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 7)
	IRQ_INIT(7);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 8)
	IRQ_INIT(8);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 9)
	IRQ_INIT(9);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 10)
	IRQ_INIT(10);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 11)
	IRQ_INIT(11);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 12)
	IRQ_INIT(12);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 13)
	IRQ_INIT(13);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 14)
	IRQ_INIT(14);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 15)
	IRQ_INIT(15);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 16)
	IRQ_INIT(16);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 17)
	IRQ_INIT(17);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 18)
	IRQ_INIT(18);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 19)
	IRQ_INIT(19);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 20)
	IRQ_INIT(20);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 21)
	IRQ_INIT(21);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 22)
	IRQ_INIT(22);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 23)
	IRQ_INIT(23);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 24)
	IRQ_INIT(24);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 25)
	IRQ_INIT(25);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 26)
	IRQ_INIT(26);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 27)
	IRQ_INIT(27);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 28)
	IRQ_INIT(28);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 29)
	IRQ_INIT(29);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 30)
	IRQ_INIT(30);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 31)
	IRQ_INIT(31);
#endif
}
