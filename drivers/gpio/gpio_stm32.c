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

/* F1 series STM32 use AFIO register to configure interrupt generation
 * and pinmux.
 */
#ifdef CONFIG_SOC_SERIES_STM32F1X
#include "afio_registers.h"
#else
#include "syscfg_registers.h"
#endif

/**
 * @brief Common GPIO driver for STM32 MCUs.
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
 * @brief Common gpio flags to custom flags
 */
const int gpio_stm32_flags_to_conf(int flags, int *pincfg)
{
	int direction = flags & GPIO_DIR_MASK;
	int pud = flags & GPIO_PUD_MASK;

	if (pincfg == NULL) {
		return -EINVAL;
	}

	if (direction == GPIO_DIR_OUT) {
		*pincfg = STM32_PINCFG_MODE_OUTPUT;
	} else {
		/* pull-{up,down} maybe? */
		*pincfg = STM32_PINCFG_MODE_INPUT;
		if (pud == GPIO_PUD_PULL_UP) {
			*pincfg |= STM32_PINCFG_PULL_UP;
		} else if (pud == GPIO_PUD_PULL_DOWN) {
			*pincfg |= STM32_PINCFG_PULL_DOWN;
		} else {
			/* floating */
			*pincfg |= STM32_PINCFG_FLOATING;
		}
	}

	return 0;
}

/**
 * @brief Configure the hardware.
 */
int gpio_stm32_configure(u32_t *base_addr, int pin, int conf, int altf)
{
	volatile struct stm32_gpio *gpio =
		(struct stm32_gpio *)(base_addr);
#ifdef CONFIG_SOC_SERIES_STM32F1X
	int cnf, mode, mode_io;
	int crpin = pin;

	/* pins are configured in CRL (0-7) and CRH (8-15)
	 * registers
	 */
	volatile u32_t *reg = &gpio->crl;

	ARG_UNUSED(altf);

	if (crpin > 7) {
		reg = &gpio->crh;
		crpin -= 8;
	}

	/* each port is configured by 2 registers:
	 * CNFy[1:0]: Port x configuration bits
	 * MODEy[1:0]: Port x mode bits
	 *
	 * memory layout is repeated for every port:
	 *   |  CNF  |  MODE |
	 *   | [0:1] | [0:1] |
	 */

	mode_io = (conf >> STM32_MODE_INOUT_SHIFT) & STM32_MODE_INOUT_MASK;

	if (mode_io == STM32_MODE_INPUT) {
		int in_pudpd = conf & (STM32_PUPD_MASK << STM32_PUPD_SHIFT);

		/* Pin configured in input mode */
		/* Mode: 00 */
		mode = mode_io;
		/* Configuration values: */
		/* 00: Analog mode */
		/* 01: Floating input */
		/* 10: Pull-up/Pull-Down */
		cnf = (conf >> STM32_CNF_IN_SHIFT) & STM32_CNF_IN_MASK;

		if (in_pudpd == STM32_PUPD_PULL_UP) {
			/* enable pull up */
			gpio->odr |= 1 << pin;
		} else if (in_pudpd == STM32_PUPD_PULL_DOWN) {
			/* or pull down */
			gpio->odr &= ~(1 << pin);
		}
	} else {
		/* Pin configured in output mode */
		int mode_speed = ((conf >> STM32_MODE_OSPEED_SHIFT) &
				   STM32_MODE_OSPEED_MASK);
		/* Mode output possible values */
		/* 01: Max speed 10MHz (default value) */
		/* 10: Max speed 2MHz */
		/* 11: Max speed 50MHz */
		mode = mode_speed + mode_io;
		/* Configuration possible values */
		/* x0: Push-pull */
		/* x1: Open-drain */
		/* 0x: General Purpose Output */
		/* 1x: Alternate Function Output */
		cnf = ((conf >> STM32_CNF_OUT_0_SHIFT) & STM32_CNF_OUT_0_MASK) |
		      (((conf >> STM32_CNF_OUT_1_SHIFT) & STM32_CNF_OUT_1_MASK)
		       << 1);
	}

	/* clear bits */
	*reg &= ~(0xf << (crpin * 4));
	/* set bits */
	*reg |= (cnf << (crpin * 4 + 2) | mode << (crpin * 4));
#else
	unsigned int mode, otype, ospeed, pupd;
	unsigned int pin_shift = pin << 1;
	unsigned int afr_bank = pin / 8;
	unsigned int afr_shift = (pin % 8) << 2;
	u32_t scratch;

	mode = (conf >> STM32_MODER_SHIFT) & STM32_MODER_MASK;
	otype = (conf >> STM32_OTYPER_SHIFT) & STM32_OTYPER_MASK;
	ospeed = (conf >> STM32_OSPEEDR_SHIFT) & STM32_OSPEEDR_MASK;
	pupd = (conf >> STM32_PUPDR_SHIFT) & STM32_PUPDR_MASK;

	scratch = gpio->moder & ~(STM32_MODER_MASK << pin_shift);
	gpio->moder = scratch | (mode << pin_shift);

	scratch = gpio->ospeedr & ~(STM32_OSPEEDR_MASK << pin_shift);
	gpio->ospeedr = scratch | (ospeed << pin_shift);

	scratch = gpio->otyper & ~(STM32_OTYPER_MASK << pin);
	gpio->otyper = scratch | (otype << pin);

	scratch = gpio->pupdr & ~(STM32_PUPDR_MASK << pin_shift);
	gpio->pupdr = scratch | (pupd << pin_shift);

	scratch = gpio->afr[afr_bank] & ~(STM32_AFR_MASK << afr_shift);
	gpio->afr[afr_bank] = scratch | (altf << afr_shift);
#endif
	return 0;
}

/**
 * @brief Enable EXTI of the specific line
 */
const int gpio_stm32_enable_int(int port, int pin)
{
#ifdef CONFIG_SOC_SERIES_STM32F1X
	volatile struct stm32_afio *syscfg =
		(struct stm32_afio *)AFIO_BASE;
#else
	volatile struct stm32_syscfg *syscfg =
		(struct stm32_syscfg *)SYSCFG_BASE;
#endif
	volatile union syscfg_exticr *exticr;

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X)
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	struct stm32_pclken pclken = {
		.bus = STM32_CLOCK_BUS_APB2,
		.enr = LL_APB2_GRP1_PERIPH_SYSCFG
	};
	/* Enable SYSCFG clock */
	clock_control_on(clk, (clock_control_subsys_t *) &pclken);
#endif
	int shift = 0;

	if (pin <= 3) {
		exticr = &syscfg->exticr1;
	} else if (pin <= 7) {
		exticr = &syscfg->exticr2;
	} else if (pin <= 11) {
		exticr = &syscfg->exticr3;
	} else if (pin <= 15) {
		exticr = &syscfg->exticr4;
	} else {
		return -EINVAL;
	}

#ifdef CONFIG_SOC_SERIES_STM32L0X
	/*
	 * Ports F and G are not present on some STM32L0 parts, so
	 * for these parts port H external interrupt should be enabled
	 * by writing value 0x5 instead of 0x7.
	 */
	if (port == STM32_PORTH) {
		port = LL_SYSCFG_EXTI_PORTH;
	}
#endif

	shift = 4 * (pin % 4);

	exticr->val &= ~(0xf << shift);
	exticr->val |= port << shift;

	return 0;
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
	map_res = gpio_stm32_flags_to_conf(flags, &pincfg);
	if (map_res) {
		return map_res;
	}

	if (gpio_stm32_configure(cfg->base, pin, pincfg, 0)) {
		return -EIO;
	}

	if (flags & GPIO_INT) {

		if (stm32_exti_set_callback(pin, cfg->port,
					    gpio_stm32_isr, dev)) {
			return -EBUSY;
		}

		gpio_stm32_enable_int(cfg->port, pin);

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
	struct stm32_gpio *gpio = (struct stm32_gpio *)cfg->base;
	int pval = 1 << (pin & 0xf);

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if (value != 0) {
		gpio->odr |= pval;
	} else {
		gpio->odr &= ~pval;
	}

	return 0;
}

/**
 * @brief Read the pin or port status
 */
static int gpio_stm32_read(struct device *dev, int access_op,
			   u32_t pin, u32_t *value)
{
	const struct gpio_stm32_config *cfg = dev->config->config_info;
	struct stm32_gpio *gpio = (struct stm32_gpio *)cfg->base;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	*value = (gpio->idr >> pin) & 0x1;

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

	if (clock_control_on(clk,
		(clock_control_subsys_t *) &cfg->pclken) != 0) {
		return -EIO;
	}

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


#define GPIO_DEVICE_INIT_STM32(__suffix, __SUFFIX)				\
	GPIO_DEVICE_INIT(DT_GPIO_STM32_GPIO##__SUFFIX##_LABEL,		\
			__suffix,						\
			DT_GPIO_STM32_GPIO##__SUFFIX##_BASE_ADDRESS, 	\
			STM32_PORT##__SUFFIX,					\
			DT_GPIO_STM32_GPIO##__SUFFIX##_CLOCK_BITS,		\
			DT_GPIO_STM32_GPIO##__SUFFIX##_CLOCK_BUS)

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
