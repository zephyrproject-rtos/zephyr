/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <drivers/pinmux.h>
#include <sys/util.h>
#include <interrupt_controller/exti_stm32.h>

#include "gpio_stm32.h"
#include "gpio_utils.h"

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

	if ((BIT(line) & data->cb_pins) != 0) {
		gpio_fire_callbacks(&data->cb, dev, BIT(line));
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
 * @brief Translate pin to pinval that the LL library needs
 */
static inline u32_t stm32_pinval_get(int pin)
{
	u32_t pinval;

#ifdef CONFIG_SOC_SERIES_STM32F1X
	pinval = (1 << pin) << GPIO_PIN_MASK_POS;
	if (pin < 8) {
		pinval |= 1 << pin;
	} else {
		pinval |= (1 << (pin % 8)) | 0x04000000;
	}
#else
	pinval = 1 << pin;
#endif
	return pinval;
}

/**
 * @brief Configure the hardware.
 */
int gpio_stm32_configure(u32_t *base_addr, int pin, int conf, int altf)
{
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)base_addr;

	int pin_ll = stm32_pinval_get(pin);

#ifdef CONFIG_SOC_SERIES_STM32F1X
	ARG_UNUSED(altf);

	u32_t temp = conf & (STM32_MODE_INOUT_MASK << STM32_MODE_INOUT_SHIFT);

	if (temp == STM32_MODE_INPUT) {
		temp = conf & (STM32_CNF_IN_MASK << STM32_CNF_IN_SHIFT);

		if (temp == STM32_CNF_IN_ANALOG) {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_ANALOG);
		} else if (temp == STM32_CNF_IN_FLOAT) {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_FLOATING);
		} else {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_INPUT);

			temp = conf & (STM32_PUPD_MASK << STM32_PUPD_SHIFT);

			if (temp == STM32_PUPD_PULL_UP) {
				LL_GPIO_SetPinPull(gpio, pin_ll, LL_GPIO_PULL_UP);
			} else {
				LL_GPIO_SetPinPull(gpio, pin_ll, LL_GPIO_PULL_DOWN);
			}
		}

	} else {
		temp = conf & (STM32_CNF_OUT_1_MASK << STM32_CNF_OUT_1_SHIFT);

		if (temp == STM32_CNF_GP_OUTPUT) {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_OUTPUT);
		} else {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_ALTERNATE);
		}

		temp = conf & (STM32_CNF_OUT_0_MASK << STM32_CNF_OUT_0_SHIFT);

		if (temp == STM32_CNF_PUSH_PULL) {
			LL_GPIO_SetPinOutputType(gpio, pin_ll, LL_GPIO_OUTPUT_PUSHPULL);
		} else {
			LL_GPIO_SetPinOutputType(gpio, pin_ll, LL_GPIO_OUTPUT_OPENDRAIN);
		}

		temp = conf & (STM32_MODE_OSPEED_MASK << STM32_MODE_OSPEED_SHIFT);

		if (temp == STM32_MODE_OUTPUT_MAX_2) {
			LL_GPIO_SetPinSpeed(gpio, pin_ll, LL_GPIO_SPEED_FREQ_LOW);
		} else if (temp == STM32_MODE_OUTPUT_MAX_10) {
			LL_GPIO_SetPinSpeed(gpio, pin_ll, LL_GPIO_SPEED_FREQ_MEDIUM);
		} else {
			LL_GPIO_SetPinSpeed(gpio, pin_ll, LL_GPIO_SPEED_FREQ_HIGH);
		}
	}
#else
	unsigned int mode, otype, ospeed, pupd;

	mode = conf & (STM32_MODER_MASK << STM32_MODER_SHIFT);
	otype = conf & (STM32_OTYPER_MASK << STM32_OTYPER_SHIFT);
	ospeed = conf & (STM32_OSPEEDR_MASK << STM32_OSPEEDR_SHIFT);
	pupd = conf & (STM32_PUPDR_MASK << STM32_PUPDR_SHIFT);

	LL_GPIO_SetPinMode(gpio, pin_ll, mode >> STM32_MODER_SHIFT);

	if (STM32_MODER_ALT_MODE == mode) {
		if (pin < 8) {
			LL_GPIO_SetAFPin_0_7(gpio, pin_ll, altf);
		} else {
			LL_GPIO_SetAFPin_8_15(gpio, pin_ll, altf);
		}
	}

#if defined(CONFIG_SOC_SERIES_STM32L4X) && defined(GPIO_ASCR_ASC0)
	/*
	 * For STM32L47xx/48xx, register ASCR should be configured to connect
	 * analog switch of gpio lines to the ADC.
	 */
	if (mode == STM32_MODER_ANALOG_MODE) {
		LL_GPIO_EnablePinAnalogControl(gpio, pin_ll);
	}
#endif

	LL_GPIO_SetPinOutputType(gpio, pin_ll, otype >> STM32_OTYPER_SHIFT);

	LL_GPIO_SetPinSpeed(gpio, pin_ll, ospeed >> STM32_OSPEEDR_SHIFT);

	LL_GPIO_SetPinPull(gpio, pin_ll, pupd >> STM32_PUPDR_SHIFT);

#endif  /* CONFIG_SOC_SERIES_STM32F1X */

	return 0;
}

/**
 * @brief Enable EXTI of the specific line
 */
const int gpio_stm32_enable_int(int port, int pin)
{
#if defined(CONFIG_SOC_SERIES_STM32F2X) ||     \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32L1X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	struct stm32_pclken pclken = {
#ifdef CONFIG_SOC_SERIES_STM32H7X
		.bus = STM32_CLOCK_BUS_APB4,
		.enr = LL_APB4_GRP1_PERIPH_SYSCFG
#else
		.bus = STM32_CLOCK_BUS_APB2,
		.enr = LL_APB2_GRP1_PERIPH_SYSCFG
#endif /* CONFIG_SOC_SERIES_STM32H7X */
	};
	/* Enable SYSCFG clock */
	clock_control_on(clk, (clock_control_subsys_t *) &pclken);
#endif

	uint32_t line;

	if (pin > 15) {
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X)
	line = ((pin % 4 * 4) << 16) | (pin / 4);
#elif defined(CONFIG_SOC_SERIES_STM32MP1X)
	line = (((pin * 8) % 32) << 16) | (pin / 4);
#elif defined(CONFIG_SOC_SERIES_STM32G0X)
	line = ((pin & 0x3) << (16 + 3)) | (pin >> 2);
#else
	line = (0xF << ((pin % 4 * 4) + 16)) | (pin / 4);
#endif

#if defined(CONFIG_SOC_SERIES_STM32L0X) && defined(LL_SYSCFG_EXTI_PORTH)
	/*
	 * Ports F and G are not present on some STM32L0 parts, so
	 * for these parts port H external interrupt should be enabled
	 * by writing value 0x5 instead of 0x7.
	 */
	if (port == STM32_PORTH) {
		port = LL_SYSCFG_EXTI_PORTH;
	}
#endif

#ifdef CONFIG_SOC_SERIES_STM32F1X
	LL_GPIO_AF_SetEXTISource(port, line);
#elif CONFIG_SOC_SERIES_STM32MP1X
	LL_EXTI_SetEXTISource(port, line);
#elif defined(CONFIG_SOC_SERIES_STM32G0X)
	LL_EXTI_SetEXTISource(port, line);
#else
	LL_SYSCFG_SetEXTISource(port, line);
#endif

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

	if ((flags & GPIO_POL_MASK) == GPIO_POL_INV) {
		/* hardware cannot invert signal */
		return -ENOTSUP;
	}

#if defined(CONFIG_STM32H7_DUAL_CORE)
	while (LL_HSEM_1StepLock(HSEM, LL_HSEM_ID_1)) {
	}
#endif /* CONFIG_STM32H7_DUAL_CORE */

	/* figure out if we can map the requested GPIO
	 * configuration
	 */
	map_res = gpio_stm32_flags_to_conf(flags, &pincfg);
	if (map_res != 0) {
		return map_res;
	}

	if (gpio_stm32_configure(cfg->base, pin, pincfg, 0) != 0) {
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_EXTI_STM32) && (flags & GPIO_INT) != 0) {

		if (stm32_exti_set_callback(pin, cfg->port,
					    gpio_stm32_isr, dev) != 0) {
			return -EBUSY;
		}

		gpio_stm32_enable_int(cfg->port, pin);

		if ((flags & GPIO_INT_EDGE) != 0) {
			int edge = 0;

			if ((flags & GPIO_INT_DOUBLE_EDGE) != 0) {
				edge = STM32_EXTI_TRIG_RISING |
				       STM32_EXTI_TRIG_FALLING;
			} else if ((flags & GPIO_INT_ACTIVE_HIGH) != 0) {
				edge = STM32_EXTI_TRIG_RISING;
			} else {
				edge = STM32_EXTI_TRIG_FALLING;
			}

			stm32_exti_trigger(pin, edge);
		} else {
			/* Level trigger interrupts not supported */
			return -ENOTSUP;
		}

		if (stm32_exti_enable(pin) != 0) {
			return -ENOSYS;
		}

	}

#if defined(CONFIG_STM32H7_DUAL_CORE)
	LL_HSEM_ReleaseLock(HSEM, LL_HSEM_ID_1, 0);
#endif /* CONFIG_STM32H7_DUAL_CORE */

	return 0;
}

/**
 * @brief Set the pin or port output
 */
static int gpio_stm32_write(struct device *dev, int access_op,
			    u32_t pin, u32_t value)
{
	const struct gpio_stm32_config *cfg = dev->config->config_info;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	pin = stm32_pinval_get(pin);
	if (value != 0U) {
		LL_GPIO_SetOutputPin(gpio, pin);
	} else {
		LL_GPIO_ResetOutputPin(gpio, pin);
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
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	*value = (LL_GPIO_ReadInputPort(gpio) >> pin) & 0x1;

	return 0;
}

static int gpio_stm32_manage_callback(struct device *dev,
				      struct gpio_callback *callback,
				      bool set)
{
	struct gpio_stm32_data *data = dev->driver_data;

	return gpio_manage_callback(&data->cb, callback, set);
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
			     (clock_control_subsys_t *)&cfg->pclken) != 0) {
		return -EIO;
	}

#ifdef PWR_CR2_IOSV
	if (cfg->port == STM32_PORTG) {
		/* Port G[15:2] requires external power supply */
		/* Cf: L4XX RM, §5.1 Power supplies */
		if (LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_PWR)) {
			LL_PWR_EnableVddIO2();
		} else {
			LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
			LL_PWR_EnableVddIO2();
			LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_PWR);
		}
	}
#endif  /* PWR_CR2_IOSV */

	return 0;
}

#define GPIO_DEVICE_INIT(__name, __suffix, __base_addr, __port, __cenr, __bus) \
	static const struct gpio_stm32_config gpio_stm32_cfg_## __suffix = {   \
		.base = (u32_t *)__base_addr,				       \
		.port = __port,						       \
		.pclken = { .bus = __bus, .enr = __cenr }		       \
	};								       \
	static struct gpio_stm32_data gpio_stm32_data_## __suffix;	       \
	DEVICE_AND_API_INIT(gpio_stm32_## __suffix,			       \
			    __name,					       \
			    gpio_stm32_init,				       \
			    &gpio_stm32_data_## __suffix,		       \
			    &gpio_stm32_cfg_## __suffix,		       \
			    POST_KERNEL,				       \
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	       \
			    &gpio_stm32_driver)

#define GPIO_DEVICE_INIT_STM32(__suffix, __SUFFIX)		      \
	GPIO_DEVICE_INIT(DT_GPIO_STM32_GPIO##__SUFFIX##_LABEL,	      \
			 __suffix,				      \
			 DT_GPIO_STM32_GPIO##__SUFFIX##_BASE_ADDRESS, \
			 STM32_PORT##__SUFFIX,			      \
			 DT_GPIO_STM32_GPIO##__SUFFIX##_CLOCK_BITS,   \
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


#if defined(CONFIG_SOC_SERIES_STM32F1X)

static int gpio_stm32_afio_init(struct device *device)
{
	UNUSED(device);

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);

#if defined(CONFIG_GPIO_STM32_SWJ_NONJTRST)
	/* released PB4 */
	__HAL_AFIO_REMAP_SWJ_NONJTRST();
#elif defined(CONFIG_GPIO_STM32_SWJ_NOJTAG)
	/* released PB4 PB3 PA15 */
	__HAL_AFIO_REMAP_SWJ_NOJTAG();
#elif defined(CONFIG_GPIO_STM32_SWJ_DISABLE)
	/* released PB4 PB3 PA13 PA14 PA15 */
	__HAL_AFIO_REMAP_SWJ_DISABLE();
#endif

	LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_AFIO);

	return 0;
}

DEVICE_INIT(gpio_stm32_afio, "", gpio_stm32_afio_init, NULL, NULL, PRE_KERNEL_2, 0);

#endif /* CONFIG_SOC_SERIES_STM32F1X */
