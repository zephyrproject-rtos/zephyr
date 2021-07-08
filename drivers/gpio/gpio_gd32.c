/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_gpio

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <drivers/gpio.h>
// #include <drivers/clock_control/gd32_clock_control.h>
// #include <pinmux/pinmux_gd32.h>
#include <drivers/pinmux.h>
#include <sys/util.h>
// #include <drivers/interrupt_controller/exti_gd32.h>
#include <pm/device.h>
#include <pm/device_runtime.h>

// #include "gd32_hsem.h"
// #include "gpio_gd32.h"
#include "gpio_utils.h"

/**
 * @brief Common GPIO driver for gd32 MCUs.
 */

/**
 * @brief EXTI interrupt callback
 */
static void gpio_gd32_isr(int line, void *arg)
{
	// struct gpio_gd32_data *data = arg;

	// gpio_fire_callbacks(&data->cb, data->dev, BIT(line));
}

/**
 * @brief Common gpio flags to custom flags
 */
static int gpio_gd32_flags_to_conf(int flags, int *pincfg)
{

	// if ((flags & GPIO_OUTPUT) != 0) {
	// 	/* Output only or Output/Input */

	// 	*pincfg = gd32_PINCFG_MODE_OUTPUT;

	// 	if ((flags & GPIO_SINGLE_ENDED) != 0) {
	// 		if (flags & GPIO_LINE_OPEN_DRAIN) {
	// 			*pincfg |= gd32_PINCFG_OPEN_DRAIN;
	// 		} else  {
	// 			/* Output can't be open source */
	// 			return -ENOTSUP;
	// 		}
	// 	} else {
	// 		*pincfg |= gd32_PINCFG_PUSH_PULL;
	// 	}

	// 	if ((flags & GPIO_PULL_UP) != 0) {
	// 		*pincfg |= gd32_PINCFG_PULL_UP;
	// 	} else if ((flags & GPIO_PULL_DOWN) != 0) {
	// 		*pincfg |= gd32_PINCFG_PULL_DOWN;
	// 	}

	// } else if  ((flags & GPIO_INPUT) != 0) {
	// 	/* Input */

	// 	*pincfg = gd32_PINCFG_MODE_INPUT;

	// 	if ((flags & GPIO_PULL_UP) != 0) {
	// 		*pincfg |= gd32_PINCFG_PULL_UP;
	// 	} else if ((flags & GPIO_PULL_DOWN) != 0) {
	// 		*pincfg |= gd32_PINCFG_PULL_DOWN;
	// 	} else {
	// 		*pincfg |= gd32_PINCFG_FLOATING;
	// 	}
	// } else {
	// 	/* Desactivated: Analog */
	// 	*pincfg = gd32_PINCFG_MODE_ANALOG;
	// }

	return 0;
}

/**
 * @brief Translate pin to pinval that the LL library needs
 */
static inline uint32_t gd32_pinval_get(int pin)
{
	uint32_t pinval;

#ifdef CONFIG_SOC_SERIES_gd32F1X
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
int gpio_gd32_configure(const struct device *dev, int pin, int conf, int altf)
{
	const struct gpio_gd32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

	int pin_ll = gd32_pinval_get(pin);

#ifdef CONFIG_SOC_SERIES_gd32F1X
	ARG_UNUSED(altf);

	uint32_t temp = conf &
			      (gd32_MODE_INOUT_MASK << gd32_MODE_INOUT_SHIFT);

	if (temp == gd32_MODE_INPUT) {
		temp = conf & (gd32_CNF_IN_MASK << gd32_CNF_IN_SHIFT);

		if (temp == gd32_CNF_IN_ANALOG) {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_ANALOG);
		} else if (temp == gd32_CNF_IN_FLOAT) {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_FLOATING);
		} else {
			temp = conf & (gd32_PUPD_MASK << gd32_PUPD_SHIFT);

			if (temp == gd32_PUPD_PULL_UP) {
				LL_GPIO_SetPinPull(gpio, pin_ll,
							       LL_GPIO_PULL_UP);
			} else {
				LL_GPIO_SetPinPull(gpio, pin_ll,
							     LL_GPIO_PULL_DOWN);
			}

			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_INPUT);
		}

	} else {
		temp = conf & (gd32_CNF_OUT_1_MASK << gd32_CNF_OUT_1_SHIFT);

		if (temp == gd32_CNF_GP_OUTPUT) {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_OUTPUT);
		} else {
			LL_GPIO_SetPinMode(gpio, pin_ll,
							LL_GPIO_MODE_ALTERNATE);
		}

		temp = conf & (gd32_CNF_OUT_0_MASK << gd32_CNF_OUT_0_SHIFT);

		if (temp == gd32_CNF_PUSH_PULL) {
			LL_GPIO_SetPinOutputType(gpio, pin_ll,
						       LL_GPIO_OUTPUT_PUSHPULL);
		} else {
			LL_GPIO_SetPinOutputType(gpio, pin_ll,
						      LL_GPIO_OUTPUT_OPENDRAIN);
		}

		temp = conf &
			    (gd32_MODE_OSPEED_MASK << gd32_MODE_OSPEED_SHIFT);

		if (temp == gd32_MODE_OUTPUT_MAX_2) {
			LL_GPIO_SetPinSpeed(gpio, pin_ll,
							LL_GPIO_SPEED_FREQ_LOW);
		} else if (temp == gd32_MODE_OUTPUT_MAX_10) {
			LL_GPIO_SetPinSpeed(gpio, pin_ll,
						     LL_GPIO_SPEED_FREQ_MEDIUM);
		} else {
			LL_GPIO_SetPinSpeed(gpio, pin_ll,
						       LL_GPIO_SPEED_FREQ_HIGH);
		}
	}
#else
	unsigned int mode, otype, ospeed, pupd;

	mode = conf & (gd32_MODER_MASK << gd32_MODER_SHIFT);
	otype = conf & (gd32_OTYPER_MASK << gd32_OTYPER_SHIFT);
	ospeed = conf & (gd32_OSPEEDR_MASK << gd32_OSPEEDR_SHIFT);
	pupd = conf & (gd32_PUPDR_MASK << gd32_PUPDR_SHIFT);

	z_gd32_hsem_lock(CFG_HW_GPIO_SEMID, HSEM_LOCK_DEFAULT_RETRY);

#if defined(CONFIG_SOC_SERIES_gd32L4X) && defined(GPIO_ASCR_ASC0)
	/*
	 * For gd32L47xx/48xx, register ASCR should be configured to connect
	 * analog switch of gpio lines to the ADC.
	 */
	if (mode == gd32_MODER_ANALOG_MODE) {
		LL_GPIO_EnablePinAnalogControl(gpio, pin_ll);
	}
#endif

	LL_GPIO_SetPinOutputType(gpio, pin_ll, otype >> gd32_OTYPER_SHIFT);

	LL_GPIO_SetPinSpeed(gpio, pin_ll, ospeed >> gd32_OSPEEDR_SHIFT);

	LL_GPIO_SetPinPull(gpio, pin_ll, pupd >> gd32_PUPDR_SHIFT);

	if (mode == gd32_MODER_ALT_MODE) {
		if (pin < 8) {
			LL_GPIO_SetAFPin_0_7(gpio, pin_ll, altf);
		} else {
			LL_GPIO_SetAFPin_8_15(gpio, pin_ll, altf);
		}
	}

	LL_GPIO_SetPinMode(gpio, pin_ll, mode >> gd32_MODER_SHIFT);

	z_gd32_hsem_unlock(CFG_HW_GPIO_SEMID);
#endif  /* CONFIG_SOC_SERIES_gd32F1X */

	return 0;
}

/**
 * @brief GPIO port clock handling
 */
int gpio_gd32_clock_request(const struct device *dev, bool on)
{
	const struct gpio_gd32_config *cfg = dev->config;
	int ret = 0;

	__ASSERT_NO_MSG(dev != NULL);

	/* enable clock for subsystem */
	const struct device *clk = DEVICE_DT_GET(gd32_CLOCK_CONTROL_NODE);

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

static inline uint32_t gpio_gd32_pin_to_exti_line(int pin)
{
#if defined(CONFIG_SOC_SERIES_gd32L0X) || \
	defined(CONFIG_SOC_SERIES_gd32F0X)
	return ((pin % 4 * 4) << 16) | (pin / 4);
#elif defined(CONFIG_SOC_SERIES_gd32MP1X)
	return (((pin * 8) % 32) << 16) | (pin / 4);
#elif defined(CONFIG_SOC_SERIES_gd32G0X) || \
	defined(CONFIG_SOC_SERIES_gd32L5X)
	return ((pin & 0x3) << (16 + 3)) | (pin >> 2);
#else
	return (0xF << ((pin % 4 * 4) + 16)) | (pin / 4);
#endif
}

static void gpio_gd32_set_exti_source(int port, int pin)
{
	uint32_t line = gpio_gd32_pin_to_exti_line(pin);

#if defined(CONFIG_SOC_SERIES_gd32L0X) && defined(LL_SYSCFG_EXTI_PORTH)
	/*
	 * Ports F and G are not present on some gd32L0 parts, so
	 * for these parts port H external interrupt should be enabled
	 * by writing value 0x5 instead of 0x7.
	 */
	if (port == gd32_PORTH) {
		port = LL_SYSCFG_EXTI_PORTH;
	}
#endif

	z_gd32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

#ifdef CONFIG_SOC_SERIES_gd32F1X
	LL_GPIO_AF_SetEXTISource(port, line);
#elif CONFIG_SOC_SERIES_gd32MP1X
	LL_EXTI_SetEXTISource(port, line);
#elif defined(CONFIG_SOC_SERIES_gd32G0X) || \
	defined(CONFIG_SOC_SERIES_gd32L5X)
	LL_EXTI_SetEXTISource(port, line);
#else
	LL_SYSCFG_SetEXTISource(port, line);
#endif
	z_gd32_hsem_unlock(CFG_HW_EXTI_SEMID);
}

static int gpio_gd32_get_exti_source(int pin)
{
	uint32_t line = gpio_gd32_pin_to_exti_line(pin);
	int port;

#ifdef CONFIG_SOC_SERIES_gd32F1X
	port = LL_GPIO_AF_GetEXTISource(line);
#elif CONFIG_SOC_SERIES_gd32MP1X
	port = LL_EXTI_GetEXTISource(line);
#elif defined(CONFIG_SOC_SERIES_gd32G0X) || \
	defined(CONFIG_SOC_SERIES_gd32L5X)
	port = LL_EXTI_GetEXTISource(line);
#else
	port = LL_SYSCFG_GetEXTISource(line);
#endif

#if defined(CONFIG_SOC_SERIES_gd32L0X) && defined(LL_SYSCFG_EXTI_PORTH)
	/*
	 * Ports F and G are not present on some gd32L0 parts, so
	 * for these parts port H external interrupt is enabled
	 * by writing value 0x5 instead of 0x7.
	 */
	if (port == LL_SYSCFG_EXTI_PORTH) {
		port = gd32_PORTH;
	}
#endif

	return port;
}

/**
 * @brief Enable EXTI of the specific line
 */
static int gpio_gd32_enable_int(int port, int pin)
{
#if defined(CONFIG_SOC_SERIES_gd32F2X) ||     \
	defined(CONFIG_SOC_SERIES_gd32F3X) || \
	defined(CONFIG_SOC_SERIES_gd32F4X) || \
	defined(CONFIG_SOC_SERIES_gd32F7X) || \
	defined(CONFIG_SOC_SERIES_gd32H7X) || \
	defined(CONFIG_SOC_SERIES_gd32L1X) || \
	defined(CONFIG_SOC_SERIES_gd32L4X) || \
	defined(CONFIG_SOC_SERIES_gd32G4X)
	const struct device *clk = DEVICE_DT_GET(gd32_CLOCK_CONTROL_NODE);
	struct gd32_pclken pclken = {
#ifdef CONFIG_SOC_SERIES_gd32H7X
		.bus = gd32_CLOCK_BUS_APB4,
		.enr = LL_APB4_GRP1_PERIPH_SYSCFG
#else
		.bus = gd32_CLOCK_BUS_APB2,
		.enr = LL_APB2_GRP1_PERIPH_SYSCFG
#endif /* CONFIG_SOC_SERIES_gd32H7X */
	};
	int ret;

	/* Enable SYSCFG clock */
	ret = clock_control_on(clk, (clock_control_subsys_t *) &pclken);
	if (ret != 0) {
		return ret;
	}
#endif

	gpio_gd32_set_exti_source(port, pin);

	return 0;
}

static int gpio_gd32_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_gd32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

	*value = LL_GPIO_ReadInputPort(gpio);

	return 0;
}

static int gpio_gd32_port_set_masked_raw(const struct device *dev,
					  gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct gpio_gd32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;
	uint32_t port_value;

	z_gd32_hsem_lock(CFG_HW_GPIO_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	port_value = LL_GPIO_ReadOutputPort(gpio);
	LL_GPIO_WriteOutputPort(gpio, (port_value & ~mask) | (mask & value));

	z_gd32_hsem_unlock(CFG_HW_GPIO_SEMID);

	return 0;
}

static int gpio_gd32_port_set_bits_raw(const struct device *dev,
					gpio_port_pins_t pins)
{
	const struct gpio_gd32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

	/*
	 * On F1 series, using LL API requires a costly pin mask translation.
	 * Skip it and use CMSIS API directly. Valid also on other series.
	 */
	WRITE_REG(gpio->BSRR, pins);

	return 0;
}

static int gpio_gd32_port_clear_bits_raw(const struct device *dev,
					  gpio_port_pins_t pins)
{
	const struct gpio_gd32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

#ifdef CONFIG_SOC_SERIES_gd32F1X
	/*
	 * On F1 series, using LL API requires a costly pin mask translation.
	 * Skip it and use CMSIS API directly.
	 */
	WRITE_REG(gpio->BRR, pins);
#else
	/* On other series, LL abstraction is needed  */
	LL_GPIO_ResetOutputPin(gpio, pins);
#endif

	return 0;
}

static int gpio_gd32_port_toggle_bits(const struct device *dev,
				       gpio_port_pins_t pins)
{
	const struct gpio_gd32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

	/*
	 * On F1 series, using LL API requires a costly pin mask translation.
	 * Skip it and use CMSIS API directly. Valid also on other series.
	 */
	z_gd32_hsem_lock(CFG_HW_GPIO_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	WRITE_REG(gpio->ODR, READ_REG(gpio->ODR) ^ pins);
	z_gd32_hsem_unlock(CFG_HW_GPIO_SEMID);

	return 0;
}

/**
 * @brief Configure pin or port
 */
static int gpio_gd32_config(const struct device *dev,
			     gpio_pin_t pin, gpio_flags_t flags)
{
#ifdef CONFIG_PM_DEVICE_RUNTIME
	struct gpio_gd32_data *data = dev->data;
#endif /* CONFIG_PM_DEVICE_RUNTIME */
	int err = 0;
	int pincfg;

	/* figure out if we can map the requested GPIO
	 * configuration
	 */
	err = gpio_gd32_flags_to_conf(flags, &pincfg);
	if (err != 0) {
		goto exit;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	/* Enable device clock before configuration (requires bank writes) */
	if (data->power_state != PM_DEVICE_STATE_ACTIVE) {
		err = pm_device_get(dev);
		if (err < 0) {
			return err;
		}
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_gd32_port_set_bits_raw(dev, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_gd32_port_clear_bits_raw(dev, BIT(pin));
		}
	}

	gpio_gd32_configure(dev, pin, pincfg, 0);

	/* Device released */
#ifdef CONFIG_PM_DEVICE_RUNTIME
	/* Release clock only if configuration doesn't require bank writes */
	if ((flags & GPIO_OUTPUT) == 0) {
		err = pm_device_put_async(dev);
		if (err < 0) {
			return err;
		}
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

exit:
	return err;
}

static int gpio_gd32_pin_interrupt_configure(const struct device *dev,
					      gpio_pin_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	const struct gpio_gd32_config *cfg = dev->config;
	struct gpio_gd32_data *data = dev->data;
	int edge = 0;
	int err = 0;

	if (mode == GPIO_INT_MODE_DISABLED) {
		if (gpio_gd32_get_exti_source(pin) == cfg->port) {
			gd32_exti_disable(pin);
			gd32_exti_unset_callback(pin);
			gd32_exti_trigger(pin, gd32_EXTI_TRIG_NONE);
		}
		/* else: No irq source configured for pin. Nothing to disable */
		goto exit;
	}

	/* Level trigger interrupts not supported */
	if (mode == GPIO_INT_MODE_LEVEL) {
		err = -ENOTSUP;
		goto exit;
	}

	if (gd32_exti_set_callback(pin, gpio_gd32_isr, data) != 0) {
		err = -EBUSY;
		goto exit;
	}

	gpio_gd32_enable_int(cfg->port, pin);

	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		edge = gd32_EXTI_TRIG_FALLING;
		break;
	case GPIO_INT_TRIG_HIGH:
		edge = gd32_EXTI_TRIG_RISING;
		break;
	case GPIO_INT_TRIG_BOTH:
		edge = gd32_EXTI_TRIG_BOTH;
		break;
	}

	gd32_exti_trigger(pin, edge);

	gd32_exti_enable(pin);

exit:
	return err;
}

static int gpio_gd32_manage_callback(const struct device *dev,
				      struct gpio_callback *callback,
				      bool set)
{
	struct gpio_gd32_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static const struct gpio_driver_api gpio_gd32_driver = {
	.pin_configure = gpio_gd32_config,
	.port_get_raw = gpio_gd32_port_get_raw,
	.port_set_masked_raw = gpio_gd32_port_set_masked_raw,
	.port_set_bits_raw = gpio_gd32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_gd32_port_clear_bits_raw,
	.port_toggle_bits = gpio_gd32_port_toggle_bits,
	.pin_interrupt_configure = gpio_gd32_pin_interrupt_configure,
	.manage_callback = gpio_gd32_manage_callback,
};

#ifdef CONFIG_PM_DEVICE
static uint32_t gpio_gd32_get_power_state(const struct device *dev)
{
	struct gpio_gd32_data *data = dev->data;

	return data->power_state;
}

static int gpio_gd32_set_power_state(const struct device *dev,
					      enum pm_device_state new_state)
{
	struct gpio_gd32_data *data = dev->data;
	int ret = 0;

	if (new_state == PM_DEVICE_STATE_ACTIVE) {
		ret = gpio_gd32_clock_request(dev, true);
	} else if (new_state == PM_DEVICE_STATE_SUSPEND) {
		ret = gpio_gd32_clock_request(dev, false);
	} else if (new_state == PM_DEVICE_STATE_LOW_POWER) {
		ret = gpio_gd32_clock_request(dev, false);
	}

	if (ret < 0) {
		return ret;
	}

	data->power_state = new_state;

	return 0;
}

static int gpio_gd32_pm_device_ctrl(const struct device *dev,
				     uint32_t ctrl_command,
				     enum pm_device_state *state, pm_device_cb cb, void *arg)
{
	struct gpio_gd32_data *data = dev->data;
	uint32_t new_state;
	int ret = 0;

	switch (ctrl_command) {
	case PM_DEVICE_STATE_SET:
		new_state = *state;
		if (new_state != data->power_state) {
			ret = gpio_gd32_set_power_state(dev, new_state);
		}
		break;
	case PM_DEVICE_STATE_GET:
		*state = gpio_gd32_get_power_state(dev);
		break;
	default:
		ret = -EINVAL;

	}

	if (cb) {
		cb(dev, ret, state, arg);
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */


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

#if defined(PWR_CR2_IOSV) && DT_NODE_HAS_STATUS(DT_NODELABEL(gpiog), okay)
	z_gd32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	/* Port G[15:2] requires external power supply */
	/* Cf: L4/L5 RM, Chapter "Independent I/O supply rail" */
	LL_PWR_EnableVddIO2();
	z_gd32_hsem_unlock(CFG_HW_RCC_SEMID);
#endif

#ifdef CONFIG_PM_DEVICE_RUNTIME
	data->power_state = PM_DEVICE_STATE_OFF;
	pm_device_enable(dev);

	return 0;
#else
#ifdef CONFIG_PM_DEVICE
	data->power_state = PM_DEVICE_STATE_ACTIVE;
#endif
	return gpio_gd32_clock_request(dev, true);
#endif
}

#define GPIO_DEVICE_INIT(__node, __suffix, __base_addr, __port, __cenr, __bus) \
	static const struct gpio_gd32_config gpio_gd32_cfg_## __suffix = {   \
		.common = {						       \
			 .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(16U), \
		},							       \
		.base = (uint32_t *)__base_addr,				       \
		.port = __port,						       \
		.pclken = { .bus = __bus, .enr = __cenr }		       \
	};								       \
	static struct gpio_gd32_data gpio_gd32_data_## __suffix;	       \
	DEVICE_DT_DEFINE(__node,					       \
			    gpio_gd32_init,				       \
			    gpio_gd32_pm_device_ctrl,			       \
			    &gpio_gd32_data_## __suffix,		       \
			    &gpio_gd32_cfg_## __suffix,		       \
			    POST_KERNEL,				       \
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	       \
			    &gpio_gd32_driver)

#define GPIO_DEVICE_INIT_gd32(__suffix, __SUFFIX)			\
	GPIO_DEVICE_INIT(DT_NODELABEL(gpio##__suffix),	\
			 __suffix,					\
			 DT_REG_ADDR(DT_NODELABEL(gpio##__suffix)),	\
			 gd32_PORT##__SUFFIX,				\
			 DT_CLOCKS_CELL(DT_NODELABEL(gpio##__suffix), bits),\
			 DT_CLOCKS_CELL(DT_NODELABEL(gpio##__suffix), bus))

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay)
GPIO_DEVICE_INIT_gd32(a, A);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
GPIO_DEVICE_INIT_gd32(b, B);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioc), okay)
GPIO_DEVICE_INIT_gd32(c, C);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioc), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiod), okay)
GPIO_DEVICE_INIT_gd32(d, D);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiod), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioe), okay)
GPIO_DEVICE_INIT_gd32(e, E);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioe), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiof), okay)
GPIO_DEVICE_INIT_gd32(f, F);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiof), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiog), okay)
GPIO_DEVICE_INIT_gd32(g, G);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiog), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioh), okay)
GPIO_DEVICE_INIT_gd32(h, H);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioh), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioi), okay)
GPIO_DEVICE_INIT_gd32(i, I);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioi), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioj), okay)
GPIO_DEVICE_INIT_gd32(j, J);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioj), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiok), okay)
GPIO_DEVICE_INIT_gd32(k, K);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiok), okay) */


#if defined(CONFIG_SOC_SERIES_gd32F1X)

static int gpio_gd32_afio_init(const struct device *dev)
{
	UNUSED(dev);

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);

#if defined(CONFIG_GPIO_gd32_SWJ_NONJTRST)
	/* released PB4 */
	__HAL_AFIO_REMAP_SWJ_NONJTRST();
#elif defined(CONFIG_GPIO_gd32_SWJ_NOJTAG)
	/* released PB4 PB3 PA15 */
	__HAL_AFIO_REMAP_SWJ_NOJTAG();
#elif defined(CONFIG_GPIO_gd32_SWJ_DISABLE)
	/* released PB4 PB3 PA13 PA14 PA15 */
	__HAL_AFIO_REMAP_SWJ_DISABLE();
#endif

	LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_AFIO);

	return 0;
}

SYS_DEVICE_DEFINE("gpio_gd32_afio", gpio_gd32_afio_init, NULL, PRE_KERNEL_2, 0);

#endif /* CONFIG_SOC_SERIES_gd32F1X */
