/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gigadevice_gd32_gpio

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <drivers/clock_control/gd32_clock_control.h>
#include <dt-bindings/clock/gd32_clock.h>
#include <drivers/pinmux.h>
#include <sys/util.h>
#ifdef CONFIG_EXTI_GD32
#include <drivers/interrupt_controller/exti_gd32.h>
#endif
#include <drivers/gpio/gpio_gd32.h>

#include "gpio_utils.h"


struct gd32_gpio {
	volatile uint32_t CTL0;
	volatile uint32_t CTL1;
	volatile uint32_t ISTAT;
	volatile uint32_t OCTL;
	volatile uint32_t BOP;
	volatile uint32_t BC;
	volatile uint32_t LOCK;
};

struct gd32_afio {
	volatile uint32_t EC;
	volatile uint32_t PCF0;
	volatile uint32_t EXTISS0;
	volatile uint32_t EXTISS1;
	volatile uint32_t EXTISS2;
	volatile uint32_t EXTISS3;
	volatile uint32_t PCF1;
};

#define DEV_CONFIG(x) ((const struct gpio_gd32_config *)(x->config))
#define GPIO_DEV ((volatile struct gd32_gpio *)(DEV_CONFIG(dev)->base))

#define AFIO ((volatile struct gd32_afio *)(AFIO_BASE))

/* GPIO mode values set */
#define GPIO_MODE_SET(n, mode)           ((uint32_t)((uint32_t)(mode) << (4U * (n))))
#define GPIO_MODE_MASK(n)                (0xFU << (4U * (n)))


/* GPIO mode definitions */
#define GPIO_MODE_AIN                    ((uint8_t)0x00U)               /*!< analog input mode */
#define GPIO_MODE_IN_FLOATING            ((uint8_t)0x04U)               /*!< floating input mode */
#define GPIO_MODE_IPD                    ((uint8_t)0x28U)               /*!< pull-down input mode */
#define GPIO_MODE_IPU                    ((uint8_t)0x48U)               /*!< pull-up input mode */
#define GPIO_MODE_OUT_OD                 ((uint8_t)0x14U)               /*!< GPIO output with open-drain */
#define GPIO_MODE_OUT_PP                 ((uint8_t)0x10U)               /*!< GPIO output with push-pull */
#define GPIO_MODE_AF_OD                  ((uint8_t)0x1CU)               /*!< AFIO output with open-drain */
#define GPIO_MODE_AF_PP                  ((uint8_t)0x18U)               /*!< AFIO output with push-pull */

/* GPIO output max speed value */
#define GPIO_OSPEED_10MHZ                ((uint8_t)0x01U)               /*!< output max speed 10MHz */
#define GPIO_OSPEED_2MHZ                 ((uint8_t)0x02U)               /*!< output max speed 2MHz */
#define GPIO_OSPEED_50MHZ                ((uint8_t)0x03U)               /*!< output max speed 50MHz */

#define GPIO_PIN_SOURCE(x) ((uint8_t)x)

static void pin_init(const struct device *dev, uint32_t mode, uint32_t speed, uint32_t pin)
{
	uint16_t i;
	uint32_t temp_mode = 0U;
	uint32_t reg = 0U;

	/* GPIO mode configuration */
	temp_mode = (mode & 0x0FU);

	/* GPIO speed configuration */
	if (mode & 0x10U) {
		/* output mode max speed:10MHz,2MHz,50MHz */
		temp_mode |= (uint32_t) speed;
	}

	/* configure the eight low port pins with GPIO_CTL0 */
	for (i = 0U; i < 16U; i++) {
		if (BIT(i) & pin) {
			reg = i < 8U ? GPIO_DEV->CTL0 : GPIO_DEV->CTL1;

			/* clear the specified pin mode bits */
			reg &= ~GPIO_MODE_MASK(i % 8U);
			/* set the specified pin mode bits */
			reg |= GPIO_MODE_SET(i % 8U, temp_mode);

			/* set IPD or IPU */
			if (GPIO_MODE_IPD == mode) {
				/* reset the corresponding OCTL bit */
				GPIO_DEV->BC = (BIT(i) & pin);
			} else {
				/* set the corresponding OCTL bit */
				if (GPIO_MODE_IPU == mode) {
					GPIO_DEV->BOP = (BIT(i) & pin);
				}
			}
			if (i < 8U) {
				GPIO_DEV->CTL0 = reg;
			} else {
				GPIO_DEV->CTL1 = reg;
			}
		}
	}
}

/**
 * @brief Common GPIO driver for GD32 MCUs.
 */

/**
 * @brief EXTI interrupt callback
 */
static void gpio_gd32_isr(int line, void *arg)
{
	struct gpio_gd32_data *data = arg;

	gpio_fire_callbacks(&data->cb, data->dev, BIT(line));
}

/**
 * @brief Common gpio flags to custom flags
 */
static int flags_to_conf(int flags, int *pincfg)
{

	if ((flags & GPIO_OUTPUT) != 0) {
		/* Output only or Output/Input */

		if (flags & GPIO_LINE_OPEN_DRAIN) {
			*pincfg = GPIO_MODE_OUT_OD | GPIO_OSPEED_50MHZ;
		} else {
			*pincfg = GPIO_MODE_OUT_PP | GPIO_OSPEED_50MHZ;
		}

	} else if ((flags & GPIO_INPUT) != 0) {
		/* Input */



		if ((flags & GPIO_PULL_UP) != 0) {
			*pincfg |= GPIO_MODE_IPU;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			*pincfg |= GPIO_MODE_IPD;
		} else {
			*pincfg |= GPIO_MODE_IN_FLOATING;
		}
	} else {
		/* Desactivated: Analog */
		*pincfg = GPIO_MODE_AIN;
	}

	return 0;
}

/**
 * @brief Translate pin to pinval that the LL library needs
 */
static uint32_t pinval_get(int pin)
{
	uint32_t pinval;

	pinval = 1 << pin;
	return pinval;
}

/**
 * @brief Configure the hardware.
 */
int gpio_gd32_configure(const struct device *dev, int pin, int conf, int altf)
{
	int pin_ll = pinval_get(pin);


	ARG_UNUSED(altf);

	uint32_t temp = conf &
			(GD32_MODE_INOUT_MASK << GD32_MODE_INOUT_SHIFT);

	if (temp == GD32_MODE_INPUT) {
		temp = conf & (GD32_CNF_IN_MASK << GD32_CNF_IN_SHIFT);

		if (temp == GD32_CNF_IN_ANALOG) {
			pin_init(dev, GPIO_MODE_AIN, 0, pin_ll);
		} else if (temp == GD32_CNF_IN_FLOAT) {
			pin_init(dev, GPIO_MODE_IN_FLOATING, 0, pin_ll);
		} else {
			temp = conf & (GD32_PUPD_MASK << GD32_PUPD_SHIFT);

			if (temp == GD32_PUPD_PULL_UP) {
				pin_init(dev, GPIO_MODE_IPU, 0, pin_ll);
			} else {
				pin_init(dev, GPIO_MODE_IPD, 0, pin_ll);
			}
		}

	} else {
		uint32_t max_hz;

		temp = conf & (GD32_MODE_OSPEED_MASK << GD32_MODE_OSPEED_SHIFT);

		if (temp == GD32_MODE_OUTPUT_MAX_2) {
			max_hz = GPIO_OSPEED_2MHZ;
		} else if (temp == GD32_MODE_OUTPUT_MAX_10) {
			max_hz = GPIO_OSPEED_10MHZ;
		} else {
			max_hz = GPIO_OSPEED_50MHZ;
		}

		temp = conf & (GD32_CNF_OUT_MASK << GD32_CNF_OUT_SHIFT);

		if (temp == GD32_CNF_AF_PP) {
			pin_init(dev, GPIO_MODE_AF_PP, max_hz, pin_ll);
		} else if (temp == GD32_CNF_AF_OD) {
			pin_init(dev, GPIO_MODE_AF_OD, max_hz, pin_ll);
		} else if (temp == GD32_CNF_OUT_PP) {
			pin_init(dev, GPIO_MODE_OUT_PP, max_hz, pin_ll);
		} else {
			pin_init(dev, GPIO_MODE_OUT_OD, max_hz, pin_ll);
		}
	}

	return 0;
}

/**
 * @brief GPIO port clock handling
 */
static int clock_request(const struct device *dev, bool on)
{
	const struct gpio_gd32_config *cfg = dev->config;
	int ret = 0;

	__ASSERT_NO_MSG(dev != NULL);

	/* enable clock for subsystem */
	const struct device *clk = DEVICE_DT_GET(DT_NODELABEL(rcu));

	if (on) {
		ret = clock_control_on(clk,
				       (clock_control_subsys_t *)&cfg->pclken);
	} else {
		ret = clock_control_off(clk,
					(clock_control_subsys_t *)&cfg->pclken);
	}

	if (ret != 0) {
		return ret;
	}

	return ret;
}


static int get_exti_source(int pin)
{
	int port;

	if (pin < GPIO_PIN_SOURCE(4)) {
		port = ((AFIO->EXTISS0 & AFIO_EXTI_PORT_MASK) >> AFIO_EXTI_SOURCE_FIELDS);
	} else if (pin < GPIO_PIN_SOURCE(8)) {
		port = ((AFIO->EXTISS1 & AFIO_EXTI_PORT_MASK) >> AFIO_EXTI_SOURCE_FIELDS);
	} else if (pin < GPIO_PIN_SOURCE(12)) {
		port = ((AFIO->EXTISS2 & AFIO_EXTI_PORT_MASK) >> AFIO_EXTI_SOURCE_FIELDS);
	} else {
		port = ((AFIO->EXTISS3 & AFIO_EXTI_PORT_MASK) >> AFIO_EXTI_SOURCE_FIELDS);
	}

	return port;
}

/**
 * @brief Enable EXTI of the specific line
 */
static int set_exti_source(int port, int pin)
{
	uint32_t source = 0U;

	source = ((uint32_t) 0x0FU)
		 << (AFIO_EXTI_SOURCE_FIELDS * (pin & AFIO_EXTI_SOURCE_MASK));

	/* select EXTI sources */
	if (GPIO_PIN_SOURCE(4) > pin) {
		/* select EXTI0/EXTI1/EXTI2/EXTI3 */
		AFIO->EXTISS0 &= ~source;
		AFIO->EXTISS0 |= (((uint32_t) port)
				  << (AFIO_EXTI_SOURCE_FIELDS
				      * (pin & AFIO_EXTI_SOURCE_MASK)));
	} else if (GPIO_PIN_SOURCE(8) > pin) {
		/* select EXTI4/EXTI5/EXTI6/EXTI7 */
		AFIO->EXTISS1 &= ~source;
		AFIO->EXTISS1 |= (((uint32_t) port)
				  << (AFIO_EXTI_SOURCE_FIELDS
				      * (pin & AFIO_EXTI_SOURCE_MASK)));
	} else if (GPIO_PIN_SOURCE(12) > pin) {
		/* select EXTI8/EXTI9/EXTI10/EXTI11 */
		AFIO->EXTISS2 &= ~source;
		AFIO->EXTISS2 |= (((uint32_t) port)
				  << (AFIO_EXTI_SOURCE_FIELDS
				      * (pin & AFIO_EXTI_SOURCE_MASK)));
	} else {
		/* select EXTI12/EXTI13/EXTI14/EXTI15 */
		AFIO->EXTISS3 &= ~source;
		AFIO->EXTISS3 |= (((uint32_t) port)
				  << (AFIO_EXTI_SOURCE_FIELDS
				      * (pin & AFIO_EXTI_SOURCE_MASK)));
	}

	return 0;
}

static int gpio_gd32_port_get_raw(const struct device *dev, uint32_t *value)
{
	*value = GPIO_DEV->ISTAT;
	return 0;
}

static int gpio_gd32_port_set_masked_raw(const struct device *dev,
					 gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	uint32_t port_value;

	port_value = GPIO_DEV->OCTL;
	GPIO_DEV->OCTL = ((port_value & ~mask) | (mask & value));

	return 0;
}

static int gpio_gd32_port_set_bits_raw(const struct device *dev,
				       gpio_port_pins_t pins)
{
	GPIO_DEV->BOP = pins;
	return 0;
}

static int gpio_gd32_port_clear_bits_raw(const struct device *dev,
					 gpio_port_pins_t pins)
{
	GPIO_DEV->BC = pins;
	return 0;
}

static int gpio_gd32_port_toggle_bits(const struct device *dev,
				      gpio_port_pins_t pins)
{
	GPIO_DEV->OCTL = (GPIO_DEV->OCTL ^ pins);
	return 0;
}

/**
 * @brief Configure pin or port
 */
static int gpio_gd32_pin_configure(const struct device *dev,
				   gpio_pin_t pin, gpio_flags_t flags)
{
	int err = 0;
	int pincfg = 0;

	/* figure out if we can map the requested GPIO
	 * configuration
	 */
	err = flags_to_conf(flags, &pincfg);
	if (err != 0) {
		goto exit;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_gd32_port_set_bits_raw(dev, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_gd32_port_clear_bits_raw(dev, BIT(pin));
		}
	}

	gpio_gd32_configure(dev, pin, pincfg, 0);

exit:
	return err;
}

static int gpio_gd32_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
#ifndef CONFIG_EXTI_GD32
{
	if (false) {
		/* Never reaching here. To suppress unused warning. */
		gpio_gd32_enable_int(0, 0);
		get_exti_source(0);
		gpio_gd32_isr(0, 0);
	}
	return -ENOTSUP;
}
#else
{
	const struct gpio_gd32_config *cfg = dev->config;
	struct gpio_gd32_data *data = dev->data;
	int edge = 0;
	int err = 0;

	if (mode == GPIO_INT_MODE_DISABLED) {
		if (get_exti_source(pin) == cfg->port) {
			gd32_exti_disable(DEVICE_DT_GET(DT_NODELABEL(exti)), pin);
			gd32_exti_unset_callback(DEVICE_DT_GET(DT_NODELABEL(exti)), pin);
			gd32_exti_trigger(DEVICE_DT_GET(DT_NODELABEL(exti)), pin, GD32_EXTI_TRIG_NONE);
		}
		/* else: No irq source configured for pin. Nothing to disable */
		goto exit;
	}

	/* Level trigger interrupts not supported */
	if (mode == GPIO_INT_MODE_LEVEL) {
		err = -ENOTSUP;
		goto exit;
	}

	if (gd32_exti_set_callback(DEVICE_DT_GET(DT_NODELABEL(exti)), pin, gpio_gd32_isr, data) != 0) {
		err = -EBUSY;
		goto exit;
	}

	set_exti_source(cfg->port, pin);

	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		edge = GD32_EXTI_TRIG_FALLING;
		break;
	case GPIO_INT_TRIG_HIGH:
		edge = GD32_EXTI_TRIG_RISING;
		break;
	case GPIO_INT_TRIG_BOTH:
		edge = GD32_EXTI_TRIG_BOTH;
		break;
	}

	gd32_exti_trigger(DEVICE_DT_GET(DT_NODELABEL(exti)), pin, edge);

	gd32_exti_enable(DEVICE_DT_GET(DT_NODELABEL(exti)), pin);

exit:
	return err;
}
#endif

static int gpio_gd32_manage_callback(const struct device *dev,
				     struct gpio_callback *callback,
				     bool set)
{
	struct gpio_gd32_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static const struct gpio_driver_api gpio_gd32_driver = {
	.pin_configure = gpio_gd32_pin_configure,
	.port_get_raw = gpio_gd32_port_get_raw,
	.port_set_masked_raw = gpio_gd32_port_set_masked_raw,
	.port_set_bits_raw = gpio_gd32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_gd32_port_clear_bits_raw,
	.port_toggle_bits = gpio_gd32_port_toggle_bits,
	.pin_interrupt_configure = gpio_gd32_pin_interrupt_configure,
	.manage_callback = gpio_gd32_manage_callback,
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
static int gpio_gd32_init(const struct device *dev)
{
	struct gpio_gd32_data *data = dev->data;

	data->dev = dev;

	return clock_request(dev, true);
}

#define GD32_PORTA 'A'
#define GD32_PORTB 'B'
#define GD32_PORTC 'C'
#define GD32_PORTD 'D'
#define GD32_PORTE 'E'
#define GD32_PORTF 'F'
#define GD32_PORTG 'G'
#define GD32_PORTH 'H'
#define GD32_PORTI 'I'
#define GD32_PORTJ 'J'
#define GD32_PORTK 'K'

#define GPIO_DEVICE_INIT(__node, __suffix, __base_addr, __port, __cenr, __bus) \
	static const struct gpio_gd32_config gpio_gd32_cfg_## __suffix = {     \
		.common = {						       \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(16U),  \
		},							       \
		.base = (uint32_t *)__base_addr,			       \
		.port = __port,						       \
		.pclken = { .bus = __bus, .enr = __cenr }		       \
	};								       \
	static struct gpio_gd32_data gpio_gd32_data_## __suffix;	       \
	DEVICE_DT_DEFINE(__node,					       \
			 gpio_gd32_init,				       \
			 NULL,						       \
			 &gpio_gd32_data_## __suffix,			       \
			 &gpio_gd32_cfg_## __suffix,			       \
			 POST_KERNEL,					       \
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		       \
			 &gpio_gd32_driver)

#define GPIO_DEVICE_INIT_GD32(__suffix, __SUFFIX)			     \
	GPIO_DEVICE_INIT(DT_NODELABEL(gpio##__suffix),			     \
			 __suffix,					     \
			 DT_REG_ADDR(DT_NODELABEL(gpio##__suffix)),	     \
			 GD32_PORT##__SUFFIX,				     \
			 DT_CLOCKS_CELL(DT_NODELABEL(gpio##__suffix), bits), \
			 DT_CLOCKS_CELL(DT_NODELABEL(gpio##__suffix), bus))

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay)
GPIO_DEVICE_INIT_GD32(a, A);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
GPIO_DEVICE_INIT_GD32(b, B);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioc), okay)
GPIO_DEVICE_INIT_GD32(c, C);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioc), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiod), okay)
GPIO_DEVICE_INIT_GD32(d, D);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiod), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioe), okay)
GPIO_DEVICE_INIT_GD32(e, E);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioe), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiof), okay)
GPIO_DEVICE_INIT_GD32(f, F);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiof), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiog), okay)
GPIO_DEVICE_INIT_GD32(g, G);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiog), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioh), okay)
GPIO_DEVICE_INIT_GD32(h, H);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioh), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioi), okay)
GPIO_DEVICE_INIT_GD32(i, I);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioi), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioj), okay)
GPIO_DEVICE_INIT_GD32(j, J);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioj), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiok), okay)
GPIO_DEVICE_INIT_GD32(k, K);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiok), okay) */



static int gpio_gd32_afio_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	const struct device *clk = DEVICE_DT_GET(DT_NODELABEL(rcu));

	clock_control_on(clk, (void *)&GD32_PCLK_EN_AF);

#if defined(CONFIG_GPIO_GD32_SWJ_NONJTRST)
	gpio_pin_remap_config(GPIO_SWJ_NONJTRST_REMAP, ENABLE)
#elif defined(CONFIG_GPIO_GD32_SWJ_DISABLE)
	gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE)
#endif

	clock_control_off(clk, (void *)&GD32_PCLK_EN_AF);

	return 0;
}

SYS_DEVICE_DEFINE("gpio_gd32_afio", gpio_gd32_afio_init, NULL, PRE_KERNEL_2, 0);
