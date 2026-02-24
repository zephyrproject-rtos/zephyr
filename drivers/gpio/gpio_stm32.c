/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Define for grep-ability, even though this will not be used */
#define DT_DRV_COMPAT st_stm32_gpio

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_exti.h>
#include <stm32_ll_gpio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/interrupt_controller/gpio_intc_stm32.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/misc/stm32_wkup_pins/stm32_wkup_pins.h>
#include <zephyr/dt-bindings/gpio/stm32-gpio.h>

#include "stm32_hsem.h"
#include "gpio_stm32.h"
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stm32, CONFIG_GPIO_LOG_LEVEL);

/**
 * @brief Common GPIO driver for STM32 MCUs.
 */

/**
 * @brief EXTI interrupt callback
 */
static void gpio_stm32_isr(gpio_port_pins_t pin, void *arg)
{
	const struct device *dev = arg;
	struct gpio_stm32_data *data = dev->data;

	gpio_fire_callbacks(&data->cb, dev, pin);
}

/**
 * @brief Common gpio flags to custom flags
 */
static int gpio_stm32_flags_to_conf(gpio_flags_t flags, uint32_t *pincfg)
{
	uint32_t cfg;

	if ((flags & GPIO_OUTPUT) != 0) {
		/* Output only or Output/Input */

		cfg = STM32_PINCFG_MODE_OUTPUT;

		if ((flags & GPIO_SINGLE_ENDED) != 0) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				cfg |= STM32_PINCFG_OPEN_DRAIN;
			} else  {
				/* Output can't be open source */
				return -ENOTSUP;
			}
		} else {
			cfg |= STM32_PINCFG_PUSH_PULL;
		}

		if ((flags & GPIO_PULL_UP) != 0) {
			cfg |= STM32_PINCFG_PULL_UP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			cfg |= STM32_PINCFG_PULL_DOWN;
		}

	} else if  ((flags & GPIO_INPUT) != 0) {
		/* Input */

		cfg = STM32_PINCFG_MODE_INPUT;

		if ((flags & GPIO_PULL_UP) != 0) {
			cfg |= STM32_PINCFG_PULL_UP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			cfg |= STM32_PINCFG_PULL_DOWN;
		} else {
			cfg |= STM32_PINCFG_FLOATING;
		}
	} else {
		/* Deactivated: Analog */
		cfg = STM32_PINCFG_MODE_ANALOG;
	}

#if !defined(CONFIG_SOC_SERIES_STM32F1X)
	switch (flags & (STM32_GPIO_SPEED_MASK << STM32_GPIO_SPEED_SHIFT)) {
	case STM32_GPIO_VERY_HIGH_SPEED:
		cfg |= STM32_OSPEEDR_VERY_HIGH_SPEED;
		break;
	case STM32_GPIO_HIGH_SPEED:
		cfg |= STM32_OSPEEDR_HIGH_SPEED;
		break;
	case STM32_GPIO_MEDIUM_SPEED:
		cfg |= STM32_OSPEEDR_MEDIUM_SPEED;
		break;
	default:
		cfg |= STM32_OSPEEDR_LOW_SPEED;
		break;
	}
#endif /* !CONFIG_SOC_SERIES_STM32F1X */

	*pincfg = cfg;

	return 0;
}

#if defined(CONFIG_GPIO_GET_CONFIG) && !defined(CONFIG_SOC_SERIES_STM32F1X)
/**
 * @brief Custom stm32 flags to zephyr
 */
static int gpio_stm32_pincfg_to_flags(struct gpio_stm32_pin pin_cfg,
				      gpio_flags_t *out_flags)
{
	gpio_flags_t flags = 0;

	if (pin_cfg.mode == LL_GPIO_MODE_OUTPUT) {
		flags |= GPIO_OUTPUT;
		if (pin_cfg.type == LL_GPIO_OUTPUT_OPENDRAIN) {
			flags |= GPIO_OPEN_DRAIN;
		}

		if (pin_cfg.out_state == 0) {
			flags |= GPIO_OUTPUT_INIT_LOW;
		} else {
			flags |= GPIO_OUTPUT_INIT_HIGH;
		}
	} else if (pin_cfg.mode == LL_GPIO_MODE_INPUT) {
		flags |= GPIO_INPUT;
#ifdef CONFIG_SOC_SERIES_STM32F1X
	} else if (pin_cfg.mode == LL_GPIO_MODE_FLOATING) {
		flags |= GPIO_INPUT;
#endif
	} else {
		flags |= GPIO_DISCONNECTED;
	}

	if (pin_cfg.pupd == LL_GPIO_PULL_UP) {
		flags |= GPIO_PULL_UP;
	} else if (pin_cfg.pupd == LL_GPIO_PULL_DOWN) {
		flags |= GPIO_PULL_DOWN;
	}

	*out_flags = flags;

	return 0;
}
#endif /* CONFIG_GPIO_GET_CONFIG */

__maybe_unused static inline uint32_t ll_gpio_get_pin_pull(GPIO_TypeDef *GPIOx, uint32_t Pin)
{
#if defined(CONFIG_SOC_SERIES_STM32WB0X)
	/* On STM32WB0, the PWRC PU/PD control registers should be used instead
	 * of the GPIO controller registers, so we cannot use LL_GPIO_GetPinPull.
	 */
	const uint32_t gpio = (GPIOx == GPIOA) ? LL_PWR_GPIO_A : LL_PWR_GPIO_B;

	if (LL_PWR_IsEnabledGPIOPullDown(gpio, Pin)) {
		return LL_GPIO_PULL_DOWN;
	} else if (LL_PWR_IsEnabledGPIOPullUp(gpio, Pin)) {
		return LL_GPIO_PULL_UP;
	} else {
		return LL_GPIO_PULL_NO;
	}
#else
	return LL_GPIO_GetPinPull(GPIOx, Pin);
#endif /* CONFIG_SOC_SERIES_STM32WB0X */
}

static inline void gpio_stm32_disable_pin_irqs(uint32_t port, gpio_pin_t pin)
{
#if defined(CONFIG_EXTI_STM32)
	if (port != stm32_exti_get_line_src_port(pin)) {
		/* EXTI line not owned by this port - do nothing */
		return;
	}
#endif
	stm32_gpio_irq_line_t irq_line = stm32_gpio_intc_get_pin_irq_line(port, pin);

	stm32_gpio_intc_disable_line(irq_line);
	stm32_gpio_intc_remove_irq_callback(irq_line);
	stm32_gpio_intc_select_line_trigger(irq_line, STM32_GPIO_IRQ_TRIG_NONE);
}

static int gpio_stm32_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_stm32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

	*value = LL_GPIO_ReadInputPort(gpio);

	return 0;
}

static int gpio_stm32_port_set_masked_raw(const struct device *dev,
					  gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct gpio_stm32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;
	uint32_t port_value;

	z_stm32_hsem_lock(CFG_HW_GPIO_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	port_value = LL_GPIO_ReadOutputPort(gpio);
	LL_GPIO_WriteOutputPort(gpio, (port_value & ~mask) | (mask & value));

	z_stm32_hsem_unlock(CFG_HW_GPIO_SEMID);

	return 0;
}

static int gpio_stm32_port_set_bits_raw(const struct device *dev,
					gpio_port_pins_t pins)
{
	const struct gpio_stm32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

	/*
	 * On F1 series, using LL API requires a costly pin mask translation.
	 * Skip it and use CMSIS API directly. Valid also on other series.
	 */
	stm32_reg_write(&gpio->BSRR, pins);

	return 0;
}

static int gpio_stm32_port_clear_bits_raw(const struct device *dev,
					  gpio_port_pins_t pins)
{
	const struct gpio_stm32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

#ifdef CONFIG_SOC_SERIES_STM32F1X
	/*
	 * On F1 series, using LL API requires a costly pin mask translation.
	 * Skip it and use CMSIS API directly.
	 */
	stm32_reg_write(&gpio->BRR, pins);
#else
	/* On other series, LL abstraction is needed  */
	LL_GPIO_ResetOutputPin(gpio, pins);
#endif

	return 0;
}

static int gpio_stm32_port_toggle_bits(const struct device *dev,
				       gpio_port_pins_t pins)
{
	const struct gpio_stm32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

	/*
	 * On F1 series, using LL API requires a costly pin mask translation.
	 * Skip it and use CMSIS API directly. Valid also on other series.
	 */
	z_stm32_hsem_lock(CFG_HW_GPIO_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	stm32_reg_write(&gpio->ODR, stm32_reg_read(&gpio->ODR) ^ pins);
	z_stm32_hsem_unlock(CFG_HW_GPIO_SEMID);

	return 0;
}


/**
 * @brief Configure pin or port
 */
static int gpio_stm32_config(const struct device *dev,
			     gpio_pin_t pin, gpio_flags_t flags)
{
	int err;
	uint32_t pincfg;
	struct gpio_stm32_data *data = dev->data;

	/* figure out if we can map the requested GPIO
	 * configuration
	 */
	err = gpio_stm32_flags_to_conf(flags, &pincfg);
	if (err != 0) {
		return err;
	}

	/* Enable device clock before configuration (requires bank writes) */
	if ((((flags & GPIO_OUTPUT) != 0) || ((flags & GPIO_INPUT) != 0)) &&
	    !(data->pin_has_clock_enabled & BIT(pin))) {
		err = pm_device_runtime_get(dev);
		if (err < 0) {
			return err;
		}
		data->pin_has_clock_enabled |= BIT(pin);
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_stm32_port_set_bits_raw(dev, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_stm32_port_clear_bits_raw(dev, BIT(pin));
		}
	}

	stm32_gpioport_configure_pin(dev, pin, pincfg, 0);

#ifdef CONFIG_STM32_WKUP_PINS
	if (flags & STM32_GPIO_WKUP) {
#ifdef CONFIG_POWEROFF
		/*
		 * On some series, wake-up pins must have a specific configuration
		 * to work properly. The following per-series checks validate that
		 * the configuration provided by caller is correct.
		 */
		if (IS_ENABLED(CONFIG_SOC_SERIES_STM32WBAX) &&
		    (flags & GPIO_OUTPUT) == 0 &&
		    ((flags & GPIO_INPUT) == 0 || (flags & (GPIO_PULL_DOWN | GPIO_PULL_UP)) == 0)) {
			/*
			 * RM0493 Rev. 7 Table 93 / RM0515 Rev. 3 Table 95:
			 * Only input pins with PU/PD and output pins are retained in Standby.
			 * Other pins are placed in High-Z state and can't be used for wake-up.
			 */
			LOG_ERR("STM32WBA: wake-up pin must be configured as "
				"output, or input+pull-up/pull-down");
			return -EINVAL;
		}

		struct gpio_dt_spec gpio_dt_cfg = {
			.port = dev,
			.pin = pin,
			.dt_flags = (gpio_dt_flags_t)flags,
		};

		err = stm32_pwr_wkup_pin_cfg_gpio(&gpio_dt_cfg);
		if (err < 0) {
			LOG_ERR("Could not configure GPIO %s pin %d as a wake-up source",
					gpio_dt_cfg.port->name, gpio_dt_cfg.pin);
			return err;
		}
#else
		LOG_DBG("STM32_GPIO_WKUP flag has no effect when CONFIG_POWEROFF=n");
#endif /* CONFIG_POWEROFF */
	}
#endif /* CONFIG_STM32_WKUP_PINS */

	/* Decrement GPIO usage count only if pin is now disconnected after being connected */
	if (((flags & GPIO_OUTPUT) == 0) && ((flags & GPIO_INPUT) == 0) &&
	    (data->pin_has_clock_enabled & BIT(pin))) {
		err = pm_device_runtime_put(dev);
		if (err < 0) {
			return err;
		}
		data->pin_has_clock_enabled &= ~BIT(pin);
	}

	return 0;
}

#if defined(CONFIG_GPIO_GET_CONFIG) && !defined(CONFIG_SOC_SERIES_STM32F1X)
/**
 * @brief Get configuration of pin
 */
static int gpio_stm32_get_config(const struct device *dev,
				 gpio_pin_t pin, gpio_flags_t *flags)
{
	const struct gpio_stm32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;
	struct gpio_stm32_pin pin_config;
	uint32_t pin_ll;
	int err;

	err = pm_device_runtime_get(dev);
	if (err < 0) {
		return err;
	}

	pin_ll = stm32_gpiomgr_pinnum_to_ll_val(pin);
	pin_config.type = LL_GPIO_GetPinOutputType(gpio, pin_ll);
	pin_config.pupd = ll_gpio_get_pin_pull(gpio, pin_ll);
	pin_config.mode = LL_GPIO_GetPinMode(gpio, pin_ll);
	pin_config.out_state = LL_GPIO_IsOutputPinSet(gpio, pin_ll);

	gpio_stm32_pincfg_to_flags(pin_config, flags);

	return pm_device_runtime_put(dev);
}
#endif /* CONFIG_GPIO_GET_CONFIG */

static int gpio_stm32_pin_interrupt_configure(const struct device *dev,
					      gpio_pin_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	const struct gpio_stm32_config *cfg = dev->config;
	const stm32_gpio_irq_line_t irq_line = stm32_gpio_intc_get_pin_irq_line(cfg->port, pin);
	uint32_t irq_trigger = 0;
	int err = 0;

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	if (mode == GPIO_INT_MODE_DISABLE_ONLY) {
		stm32_gpio_intc_disable_line(irq_line);
		goto exit;
	} else if (mode == GPIO_INT_MODE_ENABLE_ONLY) {
		stm32_gpio_intc_enable_line(irq_line);
		goto exit;
	}
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	if (mode == GPIO_INT_MODE_DISABLED) {
		gpio_stm32_disable_pin_irqs(cfg->port, pin);
		goto exit;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		/* Level-sensitive interrupts are only supported on STM32WB0. */
		if (!IS_ENABLED(CONFIG_SOC_SERIES_STM32WB0X)) {
			err = -ENOTSUP;
			goto exit;
		} else {
			switch (trig) {
			case GPIO_INT_TRIG_LOW:
				irq_trigger = STM32_GPIO_IRQ_TRIG_LOW_LEVEL;
				break;
			case GPIO_INT_TRIG_HIGH:
				irq_trigger = STM32_GPIO_IRQ_TRIG_HIGH_LEVEL;
				break;
			default:
				err = -EINVAL;
				goto exit;
			}
		}
	} else {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			irq_trigger = STM32_GPIO_IRQ_TRIG_FALLING;
			break;
		case GPIO_INT_TRIG_HIGH:
			irq_trigger = STM32_GPIO_IRQ_TRIG_RISING;
			break;
		case GPIO_INT_TRIG_BOTH:
			irq_trigger = STM32_GPIO_IRQ_TRIG_BOTH;
			break;
		default:
			err = -EINVAL;
			goto exit;
		}
	}

	if (stm32_gpio_intc_set_irq_callback(irq_line, gpio_stm32_isr, (void *)dev) != 0) {
		err = -EBUSY;
		goto exit;
	}

#if defined(CONFIG_EXTI_STM32)
	stm32_exti_set_line_src_port(pin, cfg->port);
#endif

	stm32_gpio_intc_select_line_trigger(irq_line, irq_trigger);

	stm32_gpio_intc_enable_line(irq_line);

exit:
	return err;
}

static int gpio_stm32_manage_callback(const struct device *dev,
				      struct gpio_callback *callback,
				      bool set)
{
	struct gpio_stm32_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

DEVICE_API(gpio, gpio_stm32_driver) = {
	.pin_configure = gpio_stm32_config,
#if defined(CONFIG_GPIO_GET_CONFIG) && !defined(CONFIG_SOC_SERIES_STM32F1X)
	.pin_get_config = gpio_stm32_get_config,
#endif /* CONFIG_GPIO_GET_CONFIG */
	.port_get_raw = gpio_stm32_port_get_raw,
	.port_set_masked_raw = gpio_stm32_port_set_masked_raw,
	.port_set_bits_raw = gpio_stm32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_stm32_port_clear_bits_raw,
	.port_toggle_bits = gpio_stm32_port_toggle_bits,
	.pin_interrupt_configure = gpio_stm32_pin_interrupt_configure,
	.manage_callback = gpio_stm32_manage_callback,
};

/*
 * GPIO port devices are not instanced by this driver.
 * See `soc/st/stm32/common/gpioport_mgr.c` for details.
 */
