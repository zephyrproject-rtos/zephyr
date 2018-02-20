/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2018 Bobby Noelte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief STM32 GPIO implementation
 * @defgroup device_driver_gpio_stm32_pin_controller STM32 GPIO inplementation
 *
 * A common driver for STM32 GPIOs. SoC specific adaptions are done by
 * device tree and soc.h.
 *
 * The driver uses the PINCTRL device driver as a backend.
 *
 * @{
 */

#include <autoconf.h>
#include <generated_dts_board.h>
#include <soc.h>

#include <pinctrl_common.h>
#include <gpio_common.h>
#include <dt-bindings/pinctrl/pinctrl_stm32.h>
#include <dt-bindings/gpio/gpio.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_GPIO_LEVEL
#include <logging/sys_log.h>

#include <errno.h>
#include <string.h>
#include <zephyr/types.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <kernel.h>
#include <device.h>
#include <pinctrl.h>
#include <gpio.h>

#include <drivers/clock_control/stm32_clock_control.h>
#include <interrupt_controller/exti_stm32.h>

#include "gpio_utils.h" /* _gpio_manage_callback, _gpio_fire_callbacks */

/**
 * @brief Configuration info for one GPIO device
 */
struct gpio_stm32_config {
	/** Pin controller that controls the GPIO port pins. */
	const char *pinctrl_name;
	/** Name of the GPIO bank (one of "GPIOA", ...) */
	const char *bank_name;
	/** Pin-controller pin number of GPIO port pin 0 */
	u16_t pinctrl_base;
	/** STM32Cube GPIOx */
	GPIO_TypeDef *ll_gpio_port;
};

/**
 * @brief Driver data for one GPIO device.
 */
struct gpio_stm32_data {
	/** Pin mask to enabled INT pins generate a callback */
	u32_t cb_pins;
	/* User ISR callbacks */
	sys_slist_t cb;
	/** PINCTRL GPIO function */
	u16_t pinctrl_function;
	/** STM32Cube LL_SYSCFG_EXTI_PORTx */
	u32_t ll_syscfg_exti_port;
};

/**
 * @brief Get pin controller for this GPIO.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @return Pointer to device structure of pin controller.
 */
static inline struct device *gpio_stm32_pin_controller(struct device *dev)
{
	const struct gpio_stm32_config *cfg = dev->config->config_info;

	return device_get_binding(cfg->pinctrl_name);
}

/**
 * @brief Convert external interrupt line number to SYSCFG line definiton.
 *
 * @param exti_line Number of external interrupt line [0..15].
 * @return STM32CubeMX LL_SYSCFG_EXTI_LINEx.
 */
static inline u32_t gpio_stm32_ll_syscfg_exti_line(u8_t exti_line)
{

	const u32_t ll_syscfg_exti_line[16] = {
		LL_SYSCFG_EXTI_LINE0,
		LL_SYSCFG_EXTI_LINE1,
		LL_SYSCFG_EXTI_LINE2,
		LL_SYSCFG_EXTI_LINE3,
		LL_SYSCFG_EXTI_LINE4,
		LL_SYSCFG_EXTI_LINE5,
		LL_SYSCFG_EXTI_LINE6,
		LL_SYSCFG_EXTI_LINE7,
		LL_SYSCFG_EXTI_LINE8,
		LL_SYSCFG_EXTI_LINE9,
		LL_SYSCFG_EXTI_LINE10,
		LL_SYSCFG_EXTI_LINE11,
		LL_SYSCFG_EXTI_LINE12,
		LL_SYSCFG_EXTI_LINE13,
		LL_SYSCFG_EXTI_LINE14,
		LL_SYSCFG_EXTI_LINE15
	};

	return ll_syscfg_exti_line[exti_line];
}

/**
 * @brief EXTI interrupt callback
 *
 * @param line External interrupt line (same as port pin idx).
 * @param arg Device driver structure of GPIO device.
 */
static void gpio_stm32_isr(int line, void *arg)
{
	struct device *dev = arg;
	struct gpio_stm32_data *data = dev->driver_data;

	u32_t pin_mask = GPIO_PORT_PIN(line);

	if (pin_mask & data->cb_pins) {
		_gpio_fire_callbacks(&data->cb, dev, pin_mask);
	}
}

/* --- SoC specific functions --- */

/**
 * @brief Connect external interrupt line to GPIO.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param exti_line Number of external interrupt line [0..15].
 */
static inline void gpio_stm32_syscfg_set_exti_source(struct device *dev,
							     u8_t exti_line)
{
	struct gpio_stm32_data *data = dev->driver_data;

#if !CONFIG_SOC_SERIES_STM32F1X
	/* Assure system configuration register (SYSCFG) is
	 * clocked by associated peripherial bus clock.
	 * SYSCFG manages the external interrupt line
	 * connection to the GPIOs (among other purposes).
	 */
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	struct stm32_pclken pclken = {
#if CONFIG_SOC_SERIES_STM32F0X
		.bus = STM32_CLOCK_BUS_APB1_2,
		.enr = LL_APB1_GRP2_PERIPH_SYSCFG,
#elif CONFIG_SOC_SERIES_STM32F3X || \
	CONFIG_SOC_SERIES_STM32F4X || \
	CONFIG_SOC_SERIES_STM32F7X || \
	CONFIG_SOC_SERIES_STM32L0X || \
	CONFIG_SOC_SERIES_STM32L4X
		.bus = STM32_CLOCK_BUS_APB2,
		.enr = LL_APB2_GRP1_PERIPH_SYSCFG,
#else
#error "Unknown STM32 SoC series"
#endif
	};

	clock_control_on(clk, (clock_control_subsys_t *)&pclken);
#endif

	/* Connect external line to GPIO */
	LL_SYSCFG_SetEXTISource(
		data->ll_syscfg_exti_port,
		gpio_stm32_ll_syscfg_exti_line(exti_line));
}

/* --- API --- */

/**
 * @brief Configure pin or port
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param access_op GPIO_ACCESS_BY_PIN or GPIO_ACCESS_BY_PORT
 * @param pin Pin to be configured in case of GPIO_ACCESS_BY_PIN.
 * @param flags GPIO configuration flags
 */
static int gpio_stm32_config(struct device *dev, int access_op,
			     u32_t pin, int flags)
{
	const struct gpio_stm32_config *cfg = dev->config->config_info;
	u32_t pin_mask;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		pin_mask = pin;
	} else {
		pin = GPIO_PORT_PIN0 | GPIO_PORT_PIN1 | GPIO_PORT_PIN2 |
		      GPIO_PORT_PIN3 | GPIO_PORT_PIN4 | GPIO_PORT_PIN5 |
		      GPIO_PORT_PIN6 | GPIO_PORT_PIN7 | GPIO_PORT_PIN8 |
		      GPIO_PORT_PIN9 | GPIO_PORT_PIN10 | GPIO_PORT_PIN11 |
		      GPIO_PORT_PIN12 | GPIO_PORT_PIN13 | GPIO_PORT_PIN14 |
		      GPIO_PORT_PIN15;
		pin_mask = GPIO_PORT_PIN0;
	}

	while (pin != 0) {
		u32_t gpio_port_pin = pin & pin_mask;

		pin &= ~pin_mask;         /* remove this pin from pin(s) */
		pin_mask = pin_mask << 1; /* prepare for next pin */
		if (!gpio_port_pin) {
			continue;
		}
		int err;
		u16_t pinctrl_pin =
			cfg->pinctrl_base + gpio_port_pin_idx(gpio_port_pin);

		SYS_LOG_DBG("Configure  GPIO pin: 0x%x, "
			    "PINCTRL pin: %d, flags: 0x%x.",
			     gpio_port_pin, (int)pinctrl_pin, flags);

#if CONFIG_PINCTRL_RUNTIME_DTS
		/* request pin */
		err = pinctrl_mux_request(gpio_stm32_pin_controller(dev),
					  pinctrl_pin, dev->config->name);

		if (err) {
			SYS_LOG_DBG("Mux request failed (%d) - "
				    "GPIO pin: 0x%x, PINCTRL pin: %d.",
				    err, gpio_port_pin, (int)pinctrl_pin);
			return err;
		}
#endif
		/* convert GPIO flags to PINCTRL pin configuration
		 * and configure pin
		 */
		u32_t config = 0;
		u16_t func;

		err = pinctrl_config_get(gpio_stm32_pin_controller(dev),
					 pinctrl_pin, &config);
		if (err) {
			SYS_LOG_DBG("Config get failed (%d) - "
				    "GPIO pin: 0x%x, PINCTRL pin: %d.",
				    err, gpio_port_pin, (int)pinctrl_pin);
			return err;
		}
		config &= ~PINCTRL_CONFIG_INPUT_MASK;
		config &= ~PINCTRL_CONFIG_OUTPUT_MASK;
		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_OUT) {
			config |= PINCTRL_CONFIG_INPUT_ENABLE;
			config |= PINCTRL_CONFIG_OUTPUT_ENABLE;
			func = PINCTRL_STM32_FUNCTION_OUTPUT;
		} else {
			config |= PINCTRL_CONFIG_INPUT_ENABLE;
			config |= PINCTRL_CONFIG_OUTPUT_DISABLE;
			func = PINCTRL_STM32_FUNCTION_INPUT;
		}
		if ((flags & GPIO_POL_MASK) == GPIO_POL_INV) {
			config &= ~PINCTRL_CONFIG_OUTPUT_HIGH;
			config |= PINCTRL_CONFIG_OUTPUT_LOW;
		} else {
			config &= ~PINCTRL_CONFIG_OUTPUT_LOW;
			config |= PINCTRL_CONFIG_OUTPUT_HIGH;
		}
		config &= ~PINCTRL_CONFIG_BIAS_MASK;
		switch (flags & GPIO_PUD_MASK) {
		case GPIO_PUD_PULL_UP:
			config |= PINCTRL_CONFIG_BIAS_PULL_UP;
			break;
		case GPIO_PUD_PULL_DOWN:
			config |= PINCTRL_CONFIG_BIAS_PULL_DOWN;
			break;
		case GPIO_PUD_NORMAL:
		default:
			config |= PINCTRL_CONFIG_BIAS_DISABLE;
			break;
		}
		config &= ~PINCTRL_CONFIG_DRIVE_MASK;
		config &= ~PINCTRL_CONFIG_DRIVE_STRENGTH_MASK;
		if ((flags & GPIO_DS_LOW_MASK) == GPIO_DS_DISCONNECT_LOW) {
			config |= PINCTRL_CONFIG_DRIVE_OPEN_SOURCE;
			/* low is disconnect, take high values */
			if ((flags & GPIO_DS_HIGH_MASK) == GPIO_DS_ALT_HIGH) {
				config |= PINCTRL_CONFIG_DRIVE_STRENGTH_7;
			} else {
				config |= PINCTRL_CONFIG_DRIVE_STRENGTH_DEFAULT;
			}
		} else if ((flags & GPIO_DS_HIGH_MASK) ==
			   GPIO_DS_DISCONNECT_HIGH) {
			config |= PINCTRL_CONFIG_DRIVE_OPEN_DRAIN;
			/* high is disconnect, take low values */
			if ((flags & GPIO_DS_LOW_MASK) == GPIO_DS_ALT_LOW) {
				config |= PINCTRL_CONFIG_DRIVE_STRENGTH_7;
			} else {
				config |= PINCTRL_CONFIG_DRIVE_STRENGTH_DEFAULT;
			}
		} else {
			config |= PINCTRL_CONFIG_DRIVE_PUSH_PULL;
			if (((flags & GPIO_DS_LOW_MASK) == GPIO_DS_ALT_LOW) ||
			    ((flags & GPIO_DS_HIGH_MASK) == GPIO_DS_ALT_HIGH)) {
				config |= PINCTRL_CONFIG_DRIVE_STRENGTH_7;
			} else {
				config |= PINCTRL_CONFIG_DRIVE_STRENGTH_DEFAULT;
			}
		}
		err = pinctrl_config_set(gpio_stm32_pin_controller(dev),
					 pinctrl_pin, config);
		if (err) {
			SYS_LOG_DBG("Configure failed (%d) - "
				    "GPIO pin: 0x%x, PINCTRL pin: %d.",
				    err, gpio_port_pin, (int)pinctrl_pin);
			return err;
		}
		err = pinctrl_mux_set(gpio_stm32_pin_controller(dev),
				      pinctrl_pin, func);
		if (err) {
			SYS_LOG_DBG("Mux failed (%d) - "
				    "GPIO pin: 0x%x, PINCTRL pin: %d.",
				    err, gpio_port_pin, (int)pinctrl_pin);
			return err;
		}
		SYS_LOG_DBG("Configured GPIO pin: 0x%x, PINCTRL pin: %d.",
			     gpio_port_pin, (int)pinctrl_pin);
		/* setup pin signalling */
		if (flags & GPIO_INT) {
			/* There is no level interrupt mode for exti */
			if (flags & GPIO_INT_LEVEL) {
				return -ENOTSUP;
			}
			u8_t exti_line = gpio_port_pin_idx(gpio_port_pin);

			SYS_LOG_DBG("Setup interrupt GPIO pin: 0x%x, line: %d.",
				    gpio_port_pin, (int)exti_line);

			/* Register callback on EXTI line interrupt.
			 * Unset any callback already registered.
			 * @TODO find a better way
			 */
			stm32_exti_unset_callback(exti_line);
			stm32_exti_set_callback(exti_line, gpio_stm32_isr, dev);

			/* Connect external interrupt line to GPIO. */
			gpio_stm32_syscfg_set_exti_source(dev, exti_line);

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
				/* Configure interrupt trigger mode */
				stm32_exti_trigger(exti_line, edge);
			}
			/* Enable interrupt for this line */
			stm32_exti_enable(exti_line);

			SYS_LOG_DBG("Interrupt enabled "
				    "GPIO pin: 0x%x, line: %d.",
				    gpio_port_pin, (int)exti_line);
		}
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

	if (access_op == GPIO_ACCESS_BY_PIN) {
#if CONFIG_PINCTRL_RUNTIME_DTS
		u16_t pinctrl_pin = cfg->pinctrl_base + gpio_port_pin_idx(pin);
		/* request pin */
		int err = pinctrl_mux_request(
			gpio_stm32_pin_controller(dev), pinctrl_pin,
					      dev->config->name);

		if (err) {
			return err;
		}
#endif
		if (value > 0) {
			LL_GPIO_SetOutputPin(cfg->ll_gpio_port, pin);
		} else {
			LL_GPIO_ResetOutputPin(cfg->ll_gpio_port, pin);
		}
	} else {
		u32_t value_curr = LL_GPIO_ReadOutputPort(cfg->ll_gpio_port);

		value = (value_curr & ~pin) | (value & pin);
		LL_GPIO_WriteOutputPort(cfg->ll_gpio_port, value);
	}

	return 0;
}

/**
 * @brief Read data value from the port.
 *
 * Read the input state of a port or a pin.
 *
 * In case port access is requested by GPIO_ACCESS_BY_PORT the state of each
 * pin is represented by one bit in the returned value.  Pin 0 corresponds to
 * the least significant bit, pin 31 corresponds to the most
 * significant bit. Unused bits for ports with less that 32 physical
 * pins are returned as 0.
 *
 * In case a single pin is requested value is set to 1 in case the pin is set,
 * 0 otherwise.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin in case single pin value is requested by GPIO_ACCESS_BY_PIN.
 * @param access_op GPIO_ACCESS_BY_PIN or GPIO_ACCESS_BY_PORT
 * @param value Integer pointer to receive the data value from the port.
 * @return 0 if successful, negative errno code on failure.
 */
static int gpio_stm32_read(struct device *dev, int access_op, u32_t pin,
			   u32_t *value)
{
	const struct gpio_stm32_config *cfg = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (LL_GPIO_IsInputPinSet(cfg->ll_gpio_port, pin)) {
			*value = 1;
		} else {
			*value = 0;
		}
	} else {
		*value = LL_GPIO_ReadInputPort(cfg->ll_gpio_port);
	}

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

static int gpio_stm32_enable_callback(struct device *dev, int access_op,
					      u32_t pin)
{
	struct gpio_stm32_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->cb_pins |= pin;
	} else {
		data->cb_pins |=
			GPIO_PORT_PIN0 | GPIO_PORT_PIN1 | GPIO_PORT_PIN2 |
			GPIO_PORT_PIN3 | GPIO_PORT_PIN4 | GPIO_PORT_PIN5 |
			GPIO_PORT_PIN6 | GPIO_PORT_PIN7 | GPIO_PORT_PIN8 |
			GPIO_PORT_PIN9 | GPIO_PORT_PIN10 | GPIO_PORT_PIN11 |
			GPIO_PORT_PIN12 | GPIO_PORT_PIN13 | GPIO_PORT_PIN14 |
			GPIO_PORT_PIN15;
	}

	return 0;
}

static int gpio_stm32_disable_callback(struct device *dev,
				       int access_op, u32_t pin)
{
	struct gpio_stm32_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->cb_pins &= ~pin;
	} else {
		data->cb_pins = 0;
	}
	return 0;
}

static const struct gpio_driver_api gpio_stm32_driver_api = {
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
 * Perform basic initialization of a GPIO port.
 *
 * Clock enable is delegated to PINCTRL driver.
 *
 * @param device GPIO device struct
 *
 * @return 0
 */
static int gpio_stm32_init(struct device *device)
{
	const struct gpio_stm32_config *cfg =
		device->config->config_info;

	struct gpio_stm32_data *data = device->driver_data;

	__ASSERT(gpio_stm32_pin_controller(device)
		 == device_get_binding("PINCTRL"),
		 "GPIO pin controller not given (0x%x != 0x%x)",
		 (u32_t)gpio_stm32_pin_controller(device),
		 (u32_t)device_get_binding("PINCTRL"));

	switch (cfg->bank_name[4] - 'A') {
#if CONFIG_GPIO_STM32_PORTA
	case 0:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTA;
		break;
#endif
#if CONFIG_GPIO_STM32_PORTB
	case 1:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTB;
		break;
#endif
#if CONFIG_GPIO_STM32_PORTC
	case 2:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTC;
		break;
#endif
#if CONFIG_GPIO_STM32_PORTD
	case 3:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTD;
		break;
#endif
#if CONFIG_GPIO_STM32_PORTE
	case 4:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTE;
		break;
#endif
#if CONFIG_GPIO_STM32_PORTF
	case 5:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTF;
		break;
#endif
#if CONFIG_GPIO_STM32_PORTG
	case 6:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTG;
		break;
#endif
#if CONFIG_GPIO_STM32_PORTH
	case 7:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTH;
		break;
#endif
#if CONFIG_GPIO_STM32_PORTI
	case 8:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTI;
		break;
#endif
#if CONFIG_GPIO_STM32_PORTJ
	case 9:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTJ;
		break;
#endif
#if CONFIG_GPIO_STM32_PORTK
	case 10:
		data->ll_syscfg_exti_port = LL_SYSCFG_EXTI_PORTK;
		break;
#endif
	default:
		__ASSERT(0, "GPIO bank unknown");
		break;
	}
	return 0;
}

/**
 * @code{.codegen}
 * codegen.import_module('devicedeclare')
 *
 * device_configs = ['CONFIG_GPIO_STM32_PORT{}'.format(chr(x)) \
 *                   for x in range(ord('A'), ord('K') + 1)]
 * driver_names = ['GPIO{}'.format(chr(x)) \
 *                 for x in range(ord('A'), ord('K') + 1)]
 * device_inits = 'gpio_stm32_init'
 * device_levels = 'POST_KERNEL'
 * device_prios = 'CONFIG_KERNEL_INIT_PRIORITY_DEFAULT'
 * device_api = 'gpio_stm32_driver_api'
 * device_info = \
 * """
 * static const struct gpio_stm32_config ${device-config-info} = {
 *         .ll_gpio_port = (GPIO_TypeDef *)${reg/0/address/0},
 *         .pinctrl_name = "${${gpio-ranges/0/pin-controller}:driver-name}",
 *         .pinctrl_base = ${gpio-ranges/0/pin-controller-base},
 *         .bank_name = "${st,bank-name}",
 * };
 * static struct gpio_stm32_data ${device-data};
 * """
 *
 * devicedeclare.device_declare_multi( \
 *     device_configs,
 *     driver_names,
 *     device_inits,
 *     device_levels,
 *     device_prios,
 *     device_api,
 *     device_info)
 * @endcode{.codegen}
 */
/** @code{.codeins}@endcode */

/** @} device_driver_gpio_stm32_pin_controller */

