/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_gpio

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_exti.h>
#include <stm32_ll_gpio.h>
#include <stm32_ll_pwr.h>
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
	struct gpio_stm32_data *data = arg;

	gpio_fire_callbacks(&data->cb, data->dev, pin);
}

/**
 * @brief Common gpio flags to custom flags
 */
static int gpio_stm32_flags_to_conf(gpio_flags_t flags, uint32_t *pincfg)
{

	if ((flags & GPIO_OUTPUT) != 0) {
		/* Output only or Output/Input */

		*pincfg = STM32_PINCFG_MODE_OUTPUT;

		if ((flags & GPIO_SINGLE_ENDED) != 0) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				*pincfg |= STM32_PINCFG_OPEN_DRAIN;
			} else  {
				/* Output can't be open source */
				return -ENOTSUP;
			}
		} else {
			*pincfg |= STM32_PINCFG_PUSH_PULL;
		}

		if ((flags & GPIO_PULL_UP) != 0) {
			*pincfg |= STM32_PINCFG_PULL_UP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			*pincfg |= STM32_PINCFG_PULL_DOWN;
		}

	} else if  ((flags & GPIO_INPUT) != 0) {
		/* Input */

		*pincfg = STM32_PINCFG_MODE_INPUT;

		if ((flags & GPIO_PULL_UP) != 0) {
			*pincfg |= STM32_PINCFG_PULL_UP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			*pincfg |= STM32_PINCFG_PULL_DOWN;
		} else {
			*pincfg |= STM32_PINCFG_FLOATING;
		}
	} else {
		/* Deactivated: Analog */
		*pincfg = STM32_PINCFG_MODE_ANALOG;
	}

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

	if (pin_cfg.out_state != 0) {
		flags |= GPIO_OUTPUT_HIGH;
	} else {
		flags |= GPIO_OUTPUT_LOW;
	}

	*out_flags = flags;

	return 0;
}
#endif /* CONFIG_GPIO_GET_CONFIG */

/**
 * @brief Translate pin to pinval that the LL library needs
 */
static inline uint32_t stm32_pinval_get(gpio_pin_t pin)
{
	uint32_t pinval;

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

/**
 * @brief Configure the hardware.
 */
static void gpio_stm32_configure_raw(const struct device *dev, gpio_pin_t pin,
					uint32_t conf, uint32_t func)
{
	const struct gpio_stm32_config *cfg = dev->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)cfg->base;

	uint32_t pin_ll = stm32_pinval_get(pin);

#ifdef CONFIG_SOC_SERIES_STM32F1X
	ARG_UNUSED(func);

	uint32_t temp = conf &
			      (STM32_MODE_INOUT_MASK << STM32_MODE_INOUT_SHIFT);

	if (temp == STM32_MODE_INPUT) {
		temp = conf & (STM32_CNF_IN_MASK << STM32_CNF_IN_SHIFT);

		if (temp == STM32_CNF_IN_ANALOG) {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_ANALOG);
		} else if (temp == STM32_CNF_IN_FLOAT) {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_FLOATING);
		} else {
			temp = conf & (STM32_PUPD_MASK << STM32_PUPD_SHIFT);

			if (temp == STM32_PUPD_PULL_UP) {
				LL_GPIO_SetPinPull(gpio, pin_ll,
							       LL_GPIO_PULL_UP);
			} else {
				LL_GPIO_SetPinPull(gpio, pin_ll,
							     LL_GPIO_PULL_DOWN);
			}

			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_INPUT);
		}

	} else {
		temp = conf & (STM32_CNF_OUT_1_MASK << STM32_CNF_OUT_1_SHIFT);

		if (temp == STM32_CNF_GP_OUTPUT) {
			LL_GPIO_SetPinMode(gpio, pin_ll, LL_GPIO_MODE_OUTPUT);
		} else {
			LL_GPIO_SetPinMode(gpio, pin_ll,
							LL_GPIO_MODE_ALTERNATE);
		}

		temp = conf & (STM32_CNF_OUT_0_MASK << STM32_CNF_OUT_0_SHIFT);

		if (temp == STM32_CNF_PUSH_PULL) {
			LL_GPIO_SetPinOutputType(gpio, pin_ll,
						       LL_GPIO_OUTPUT_PUSHPULL);
		} else {
			LL_GPIO_SetPinOutputType(gpio, pin_ll,
						      LL_GPIO_OUTPUT_OPENDRAIN);
		}

		temp = conf &
			    (STM32_MODE_OSPEED_MASK << STM32_MODE_OSPEED_SHIFT);

		if (temp == STM32_MODE_OUTPUT_MAX_2) {
			LL_GPIO_SetPinSpeed(gpio, pin_ll,
							LL_GPIO_SPEED_FREQ_LOW);
		} else if (temp == STM32_MODE_OUTPUT_MAX_10) {
			LL_GPIO_SetPinSpeed(gpio, pin_ll,
						     LL_GPIO_SPEED_FREQ_MEDIUM);
		} else {
			LL_GPIO_SetPinSpeed(gpio, pin_ll,
						       LL_GPIO_SPEED_FREQ_HIGH);
		}
	}
#else
	uint32_t mode, otype, ospeed, pupd;

	mode = conf & (STM32_MODER_MASK << STM32_MODER_SHIFT);
	otype = conf & (STM32_OTYPER_MASK << STM32_OTYPER_SHIFT);
	ospeed = conf & (STM32_OSPEEDR_MASK << STM32_OSPEEDR_SHIFT);
	pupd = conf & (STM32_PUPDR_MASK << STM32_PUPDR_SHIFT);

	z_stm32_hsem_lock(CFG_HW_GPIO_SEMID, HSEM_LOCK_DEFAULT_RETRY);

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

	if (mode == STM32_MODER_ALT_MODE) {
		if (pin < 8) {
			LL_GPIO_SetAFPin_0_7(gpio, pin_ll, func);
		} else {
			LL_GPIO_SetAFPin_8_15(gpio, pin_ll, func);
		}
	}

	LL_GPIO_SetPinMode(gpio, pin_ll, mode >> STM32_MODER_SHIFT);

	z_stm32_hsem_unlock(CFG_HW_GPIO_SEMID);
#endif  /* CONFIG_SOC_SERIES_STM32F1X */

}

/**
 * @brief GPIO port clock handling
 */
static int gpio_stm32_clock_request(const struct device *dev, bool on)
{
	const struct gpio_stm32_config *cfg = dev->config;
	int ret;

	__ASSERT_NO_MSG(dev != NULL);

	/* enable clock for subsystem */
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (on) {
		ret = clock_control_on(clk,
					(clock_control_subsys_t)&cfg->pclken);
	} else {
		ret = clock_control_off(clk,
					(clock_control_subsys_t)&cfg->pclken);
	}

	return ret;
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
	WRITE_REG(gpio->BSRR, pins);

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
	WRITE_REG(gpio->BRR, pins);
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
	WRITE_REG(gpio->ODR, READ_REG(gpio->ODR) ^ pins);
	z_stm32_hsem_unlock(CFG_HW_GPIO_SEMID);

	return 0;
}

#ifdef CONFIG_SOC_SERIES_STM32F1X
#define IS_GPIO_OUT GPIO_OUT
#else
#define IS_GPIO_OUT STM32_GPIO
#endif

int gpio_stm32_configure(const struct device *dev, gpio_pin_t pin, uint32_t conf, uint32_t func)
{
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	gpio_stm32_configure_raw(dev, pin, conf, func);

	if (func == IS_GPIO_OUT) {
		uint32_t gpio_out = conf & (STM32_ODR_MASK << STM32_ODR_SHIFT);

		if (gpio_out == STM32_ODR_1) {
			gpio_stm32_port_set_bits_raw(dev, BIT(pin));
		} else if (gpio_out == STM32_ODR_0) {
			gpio_stm32_port_clear_bits_raw(dev, BIT(pin));
		}
	}

	return pm_device_runtime_put(dev);
}

/**
 * @brief Configure pin or port
 */
static int gpio_stm32_config(const struct device *dev,
			     gpio_pin_t pin, gpio_flags_t flags)
{
	int err;
	uint32_t pincfg;

	/* figure out if we can map the requested GPIO
	 * configuration
	 */
	err = gpio_stm32_flags_to_conf(flags, &pincfg);
	if (err != 0) {
		return err;
	}

	/* Enable device clock before configuration (requires bank writes) */
	if (((flags & GPIO_OUTPUT) != 0) || ((flags & GPIO_INPUT) != 0)) {
		err = pm_device_runtime_get(dev);
		if (err < 0) {
			return err;
		}
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_stm32_port_set_bits_raw(dev, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_stm32_port_clear_bits_raw(dev, BIT(pin));
		}
	}

	gpio_stm32_configure_raw(dev, pin, pincfg, 0);

#ifdef CONFIG_STM32_WKUP_PINS
	if (flags & STM32_GPIO_WKUP) {
#ifdef CONFIG_POWEROFF
		struct gpio_dt_spec gpio_dt_cfg = {
			.port = dev,
			.pin = pin,
			.dt_flags = (gpio_dt_flags_t)flags,
		};

		if (stm32_pwr_wkup_pin_cfg_gpio((const struct gpio_dt_spec *)&gpio_dt_cfg)) {
			LOG_ERR("Could not configure GPIO %s pin %d as a wake-up source",
					gpio_dt_cfg.port->name, gpio_dt_cfg.pin);
		}
#else
		LOG_DBG("STM32_GPIO_WKUP flag has no effect when CONFIG_POWEROFF=n");
#endif /* CONFIG_POWEROFF */
	}
#endif /* CONFIG_STM32_WKUP_PINS */

	/* Release clock only if pin is disconnected */
	if (((flags & GPIO_OUTPUT) == 0) && ((flags & GPIO_INPUT) == 0)) {
		err = pm_device_runtime_put(dev);
		if (err < 0) {
			return err;
		}
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

	pin_ll = stm32_pinval_get(pin);
	pin_config.type = LL_GPIO_GetPinOutputType(gpio, pin_ll);
	pin_config.pupd = LL_GPIO_GetPinPull(gpio, pin_ll);
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
	struct gpio_stm32_data *data = dev->data;
	const stm32_gpio_irq_line_t irq_line = stm32_gpio_intc_get_pin_irq_line(cfg->port, pin);
	uint32_t edge = 0;
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

	/* Level trigger interrupts not supported */
	if (mode == GPIO_INT_MODE_LEVEL) {
		err = -ENOTSUP;
		goto exit;
	}

	if (stm32_gpio_intc_set_irq_callback(irq_line, gpio_stm32_isr, data) != 0) {
		err = -EBUSY;
		goto exit;
	}

	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		edge = STM32_GPIO_IRQ_TRIG_FALLING;
		break;
	case GPIO_INT_TRIG_HIGH:
		edge = STM32_GPIO_IRQ_TRIG_RISING;
		break;
	case GPIO_INT_TRIG_BOTH:
		edge = STM32_GPIO_IRQ_TRIG_BOTH;
		break;
	default:
		err = -EINVAL;
		goto exit;
	}

#if defined(CONFIG_EXTI_STM32)
	stm32_exti_set_line_src_port(pin, cfg->port);
#endif

	stm32_gpio_intc_select_line_trigger(irq_line, edge);

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

static const struct gpio_driver_api gpio_stm32_driver = {
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

#ifdef CONFIG_PM_DEVICE
static int gpio_stm32_pm_action(const struct device *dev,
				enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return gpio_stm32_clock_request(dev, true);
	case PM_DEVICE_ACTION_SUSPEND:
		return gpio_stm32_clock_request(dev, false);
	default:
		return -ENOTSUP;
	}

	return 0;
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
static int gpio_stm32_init(const struct device *dev)
{
	struct gpio_stm32_data *data = dev->data;
	int ret;

	data->dev = dev;

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		return -ENODEV;
	}

#if (defined(PWR_CR2_IOSV) || defined(PWR_SVMCR_IO2SV)) && \
	DT_NODE_HAS_STATUS(DT_NODELABEL(gpiog), okay)
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	/* Port G[15:2] requires external power supply */
	/* Cf: L4/L5 RM, Chapter "Independent I/O supply rail" */
	LL_PWR_EnableVddIO2();
	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);
#endif
	/* enable port clock (if runtime PM is not enabled) */
	ret = gpio_stm32_clock_request(dev, !IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME));
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_init_suspended(dev);
	}
	(void)pm_device_runtime_enable(dev);

	return 0;
}

#define GPIO_DEVICE_INIT(__node, __suffix, __base_addr, __port, __cenr, __bus) \
	static const struct gpio_stm32_config gpio_stm32_cfg_## __suffix = {   \
		.common = {						       \
			 .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(16U), \
		},							       \
		.base = (uint32_t *)__base_addr,				       \
		.port = __port,						       \
		.pclken = { .bus = __bus, .enr = __cenr }		       \
	};								       \
	static struct gpio_stm32_data gpio_stm32_data_## __suffix;	       \
	PM_DEVICE_DT_DEFINE(__node, gpio_stm32_pm_action);		       \
	DEVICE_DT_DEFINE(__node,					       \
			    gpio_stm32_init,				       \
			    PM_DEVICE_DT_GET(__node),			       \
			    &gpio_stm32_data_## __suffix,		       \
			    &gpio_stm32_cfg_## __suffix,		       \
			    PRE_KERNEL_1,				       \
			    CONFIG_GPIO_INIT_PRIORITY,			       \
			    &gpio_stm32_driver)

#define GPIO_DEVICE_INIT_STM32(__suffix, __SUFFIX)			\
	GPIO_DEVICE_INIT(DT_NODELABEL(gpio##__suffix),	\
			 __suffix,					\
			 DT_REG_ADDR(DT_NODELABEL(gpio##__suffix)),	\
			 STM32_PORT##__SUFFIX,				\
			 DT_CLOCKS_CELL(DT_NODELABEL(gpio##__suffix), bits),\
			 DT_CLOCKS_CELL(DT_NODELABEL(gpio##__suffix), bus))

#define GPIO_DEVICE_INIT_STM32_IF_OKAY(__suffix, __SUFFIX) \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(gpio##__suffix), okay), \
		    (GPIO_DEVICE_INIT_STM32(__suffix, __SUFFIX)), \
		    ())

GPIO_DEVICE_INIT_STM32_IF_OKAY(a, A);
GPIO_DEVICE_INIT_STM32_IF_OKAY(b, B);
GPIO_DEVICE_INIT_STM32_IF_OKAY(c, C);
GPIO_DEVICE_INIT_STM32_IF_OKAY(d, D);
GPIO_DEVICE_INIT_STM32_IF_OKAY(e, E);
GPIO_DEVICE_INIT_STM32_IF_OKAY(f, F);
GPIO_DEVICE_INIT_STM32_IF_OKAY(g, G);
GPIO_DEVICE_INIT_STM32_IF_OKAY(h, H);
GPIO_DEVICE_INIT_STM32_IF_OKAY(i, I);
GPIO_DEVICE_INIT_STM32_IF_OKAY(j, J);
GPIO_DEVICE_INIT_STM32_IF_OKAY(k, K);

GPIO_DEVICE_INIT_STM32_IF_OKAY(m, M);
GPIO_DEVICE_INIT_STM32_IF_OKAY(n, N);
GPIO_DEVICE_INIT_STM32_IF_OKAY(o, O);
GPIO_DEVICE_INIT_STM32_IF_OKAY(p, P);
