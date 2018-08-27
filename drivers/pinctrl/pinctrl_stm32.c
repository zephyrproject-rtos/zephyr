/*
 * Copyright (c) 2018 Bobby Noelte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief STM32 pinctrl implementation
 * @defgroup device_driver_pinctrl_stm32 STM32 pinctrl inplementation
 *
 * A common driver for STM32 pinctrl. SoC specific adaptions are
 * done by device tree and soc.h.
 *
 * @{
 */

#include <autoconf.h>
#include <generated_dts_board.h>
#include <soc.h>

#include <pinctrl_common.h>
#include <dt-bindings/clock/stm32_clock.h>
#include <dt-bindings/pinctrl/pinctrl_stm32.h>

#include <errno.h>
#include <string.h>
#include <zephyr/types.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <kernel.h>
#include <device.h>
#include <gpio.h>
#include <pinctrl.h>

#include <clock_control/stm32_clock_control.h>

/**
 * @def PINCTRL_STM32_GPIO_COUNT
 * @brief Number of GPIO ports that may be managed by the pinctrl driver.
 *
 * This is the total number of GPIO ports available. The number of activated
 * GPIO ports may be less.
 */
/* GPIOx is defined by the STM32 CubeMX library. */
#if defined(GPIOK)
#define PINCTRL_STM32_GPIO_COUNT 11
#elif defined(GPIOJ)
#define PINCTRL_STM32_GPIO_COUNT 10
#elif defined(GPIOI)
#define PINCTRL_STM32_GPIO_COUNT 9
#elif defined(GPIOH)
#define PINCTRL_STM32_GPIO_COUNT 8
#elif defined(GPIOG)
#define PINCTRL_STM32_GPIO_COUNT 7
#elif defined(GPIOF)
#define PINCTRL_STM32_GPIO_COUNT 6
#elif defined(GPIOE)
#define PINCTRL_STM32_GPIO_COUNT 5
#elif defined(GPIOD)
#define PINCTRL_STM32_GPIO_COUNT 4
#elif defined(GPIOC)
#define PINCTRL_STM32_GPIO_COUNT 3
#elif defined(GPIOB)
#define PINCTRL_STM32_GPIO_COUNT 2
#elif defined(GPIOA)
#define PINCTRL_STM32_GPIO_COUNT 1
#else
#define PINCTRL_STM32_GPIO_COUNT 0
#error "Missing GPIOx definition!"
#endif

/**
 * @brief Driver data for one PINCTRL device.
 */
struct pinctrl_stm32_data {
	/** clock control device */
	struct device *clk;
	/** Peripherial bus clock enable info for gpio devices */
	struct stm32_pclken pclken_gpio;
};

/**
 * @brief GPIO info.
 */
struct pinctrl_stm32_gpio {
	u32_t ll_gpio_pclken_enr;
	GPIO_TypeDef *ll_gpio_port;
};

/**
 * @brief GPIO data.
 */
static const struct pinctrl_stm32_gpio
	pinctrl_stm32_gpio_data[PINCTRL_STM32_GPIO_COUNT] = {
/**
 * @code{.codegen}
 * # Evaluate the clock bus for the GPIOs.
 * # @todo Take clock bus from device tree
 * bus = None
 * if codegen.config_property('CONFIG_SOC_SERIES_STM32F0X', 0) or \
 *    codegen.config_property('CONFIG_SOC_SERIES_STM32F3X', 0) or \
 *    codegen.config_property('CONFIG_SOC_SERIES_STM32F4X', 0) or \
 *    codegen.config_property('CONFIG_SOC_SERIES_STM32F7X', 0):
 *     bus = 'AHB1'
 * elif codegen.config_property('CONFIG_SOC_SERIES_STM32F1X', 0):
 *     bus = 'APB1'
 * elif codegen.config_property('CONFIG_SOC_SERIES_STM32L4X', 0):
 *     bus = 'AHB2'
 * # Iterate the possible GPIOs
 * for x in range(0, 11):
 *     port = 'GPIO{}'.format(chr(ord('A') + x))
 *     codegen.outl('#if PINCTRL_STM32_GPIO_COUNT > {}'.format(x))
 *     codegen.outl('\t{')
 *     codegen.outl('#if defined({})'.format(port))
 *     codegen.outl('\t\t.ll_gpio_pclken_enr = LL_{}_GRP1_PERIPH_{},'.format(
 *         bus, port))
 *     codegen.outl('\t\t.ll_gpio_port = {},'.format(port))
 *     codegen.outl('#else')
 *     codegen.outl('\t\t.ll_gpio_pclken_enr = 0,')
 *     codegen.outl('\t\t.ll_gpio_port = 0,')
 *     codegen.outl('#endif'.format(port))
 *     codegen.outl('\t},')
 *     codegen.outl('#endif')
 * @endcode{.codegen}
 */
/** @code{.codeins}@endcode */
};

/* --- API (using the PINCTRL template code) --- */

/* forward declarations */
static int pinctrl_stm32_config_get(struct device *dev, u16_t pin,
				    u32_t *config);
static int pinctrl_stm32_config_set(struct device *dev, u16_t pin,
				    u32_t config);
static int pinctrl_stm32_mux_get(struct device *dev, u16_t pin, u16_t *func);
static int pinctrl_stm32_mux_set(struct device *dev, u16_t pin, u16_t func);
static int pinctrl_stm32_device_init(struct device *dev);

/**
 * @code{.codegen}
 * compatible = 'st,stm32-pinctrl'
 * config_get = 'pinctrl_stm32_config_get'
 * config_set = 'pinctrl_stm32_config_set'
 * mux_get = 'pinctrl_stm32_mux_get'
 * mux_set = 'pinctrl_stm32_mux_set'
 * device_init = 'pinctrl_stm32_device_init'
 * data_info = 'struct pinctrl_stm32_data'
 * codegen.out_include('templates/drivers/pinctrl_tmpl.c')
 * @endcode{.codegen}
 */
/** @code{.codeins}@endcode */

/**
 * @brief Convert pin number to STM32CubeMX GPIO port descriptor.
 *
 * @param pin Pin number
 * @return STM32CubeMX GPIO port descriptor
 */
static GPIO_TypeDef *pinctrl_stm32_ll_gpio_port(u16_t pin)
{
	int port_idx = (pin >> 4) & 0x0F;

	__ASSERT((port_idx < PINCTRL_STM32_GPIO_COUNT),
		 "pin unknown - no GPIO port descriptor");
	__ASSERT((pinctrl_stm32_gpio_data[port_idx].ll_gpio_port != 0),
		 "pin unknown - no GPIO port descriptor");

	return pinctrl_stm32_gpio_data[port_idx].ll_gpio_port;
}

/**
 * @brief Convert pin number to STM32CubeMX GPIO pin descriptor.
 *
 * @param pin Pin number
 * @return STM32CubeMX GPIO pin descriptor
 */
static inline u32_t pinctrl_stm32_ll_gpio_pin(u16_t pin)
{
	return 0x01U << (pin & 0x0F);
}

/**
 * @brief Get STM32CubeMX GPIO mode associated to function.
 *
 * @param func Function
 * @return STM32CubeMX GPIO mode
 */
static u32_t pinctrl_stm32_ll_gpio_mode(u16_t func)
{
	switch (func) {
	case PINCTRL_STM32_FUNCTION_ALT_0:
	case PINCTRL_STM32_FUNCTION_ALT_1:
	case PINCTRL_STM32_FUNCTION_ALT_2:
	case PINCTRL_STM32_FUNCTION_ALT_3:
	case PINCTRL_STM32_FUNCTION_ALT_4:
	case PINCTRL_STM32_FUNCTION_ALT_5:
	case PINCTRL_STM32_FUNCTION_ALT_6:
	case PINCTRL_STM32_FUNCTION_ALT_7:
	case PINCTRL_STM32_FUNCTION_ALT_8:
	case PINCTRL_STM32_FUNCTION_ALT_9:
	case PINCTRL_STM32_FUNCTION_ALT_10:
	case PINCTRL_STM32_FUNCTION_ALT_11:
	case PINCTRL_STM32_FUNCTION_ALT_12:
	case PINCTRL_STM32_FUNCTION_ALT_13:
	case PINCTRL_STM32_FUNCTION_ALT_14:
	case PINCTRL_STM32_FUNCTION_ALT_15:
		return LL_GPIO_MODE_ALTERNATE;
	case PINCTRL_STM32_FUNCTION_INPUT:
		return LL_GPIO_MODE_INPUT;
	case PINCTRL_STM32_FUNCTION_OUTPUT:
		return LL_GPIO_MODE_OUTPUT;
	case PINCTRL_STM32_FUNCTION_ANALOG:
		return LL_GPIO_MODE_ANALOG;
	}
	__ASSERT(0, "function 0x%x unknown - no LL_GPIO_MODE", (int)func);
	return 0;
}

static inline u32_t pinctrl_stm32_ll_gpio_alt_func(u16_t func)
{
	if ((func >= PINCTRL_STM32_FUNCTION_ALT_0) &&
	    (func <= PINCTRL_STM32_FUNCTION_ALT_15)) {
		return func - PINCTRL_STM32_FUNCTION_ALT_0;
	}
	__ASSERT(0, "function 0x%x unknown - no LL_GPIO_ALT_FUNC", (int)func);
	return func;
}

/**
 * @brief Enable GPIO port to be clocked by associated peripherial bus clock.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param port_idx Index of GPIO port
 */
static void pinctrl_stm32_gpio_enable_clock(struct device *dev, int port_idx)
{
	__ASSERT((port_idx < PINCTRL_STM32_GPIO_COUNT),
		 "port unknown - invalid port_idx");
	__ASSERT((pinctrl_stm32_gpio_data[port_idx].ll_gpio_port != 0),
		 "port unknown - invalid port_idx");

	struct pinctrl_stm32_data *data = dev->driver_data;
	/* data->pclken_gpio.bus already set by init */
	data->pclken_gpio.enr =
		pinctrl_stm32_gpio_data[port_idx].ll_gpio_pclken_enr;
	clock_control_on(
		data->clk,
		(clock_control_subsys_t)&data->pclken_gpio);
}

/**
 * Get the configuration of a pin.
 *
 * The following configuration options are supported.
 *
 * Options set by configuration or mux function:
 *   - @ref PINCTRL_CONFIG_BIAS_DISABLE
 *
 * Options set by configuration:
 *   - @ref PINCTRL_CONFIG_BIAS_PULL_UP
 *   - @ref PINCTRL_CONFIG_BIAS_PULL_DOWN
 *   - @ref PINCTRL_CONFIG_DRIVE_PUSH_PULL
 *   - @ref PINCTRL_CONFIG_DRIVE_OPEN_DRAIN
 *   - @ref PINCTRL_CONFIG_SPEED_SLOW
 *   - @ref PINCTRL_CONFIG_SPEED_MEDIUM
 *   - @ref PINCTRL_CONFIG_SPEED_HIGH
 *
 * Options set/ forced by mux function only:
 *   - @ref PINCTRL_CONFIG_INPUT_ENABLE
 *   - @ref PINCTRL_CONFIG_INPUT_DISABLE
 *   - @ref PINCTRL_CONFIG_INPUT_SCHMITT_ENABLE
 *   - @ref PINCTRL_CONFIG_INPUT_SCHMITT_DISABLE
 *   - @ref PINCTRL_CONFIG_OUTPUT_ENABLE
 *   - @ref PINCTRL_CONFIG_OUTPUT_DISABLE
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin
 * @param config
 * @retval 0 on success
 * @retval -ENOTSUP if requested pin is not available on this controller
 * @retval -EINVAL if requested pin is available but disabled
 */
static int pinctrl_stm32_config_get(struct device *dev, u16_t pin,
				    u32_t *config)
{
	ARG_UNUSED(dev);

	if (pin >= pinctrl_tmpl_control_get_pins_count(dev)) {
		return -ENOTSUP;
	}

	*config = 0;

	GPIO_TypeDef *ll_gpio_port = pinctrl_stm32_ll_gpio_port(pin);
	u32_t ll_gpio_pin = pinctrl_stm32_ll_gpio_pin(pin);

	/* current pin mode (muxing function) */
	switch (LL_GPIO_GetPinMode(ll_gpio_port, ll_gpio_pin)) {
	case LL_GPIO_MODE_ALTERNATE:
		*config |= PINCTRL_CONFIG_OUTPUT_ENABLE |
			   PINCTRL_CONFIG_INPUT_ENABLE |
			   PINCTRL_CONFIG_INPUT_SCHMITT_ENABLE;
		break;
	case LL_GPIO_MODE_OUTPUT:
		*config |= PINCTRL_CONFIG_OUTPUT_ENABLE |
			   PINCTRL_CONFIG_INPUT_ENABLE |
			   PINCTRL_CONFIG_INPUT_SCHMITT_ENABLE;
		break;
	case LL_GPIO_MODE_INPUT:
		*config |= PINCTRL_CONFIG_INPUT_ENABLE |
			   PINCTRL_CONFIG_OUTPUT_DISABLE |
			   PINCTRL_CONFIG_INPUT_SCHMITT_ENABLE;
		break;
	case LL_GPIO_MODE_ANALOG:
		*config |= PINCTRL_CONFIG_OUTPUT_DISABLE |
			   PINCTRL_CONFIG_INPUT_DISABLE |
			   PINCTRL_CONFIG_INPUT_SCHMITT_DISABLE |
			   PINCTRL_CONFIG_BIAS_DISABLE;
		break;
	}

	if (!(*config | PINCTRL_CONFIG_BIAS_DISABLE)) {
		switch (LL_GPIO_GetPinOutputType(ll_gpio_port, ll_gpio_pin)) {
		case LL_GPIO_OUTPUT_PUSHPULL:
			*config |= PINCTRL_CONFIG_BIAS_DISABLE;
			break;
		case LL_GPIO_PULL_UP:
			*config |= PINCTRL_CONFIG_BIAS_PULL_UP;
			break;
		case LL_GPIO_PULL_DOWN:
			*config |= PINCTRL_CONFIG_BIAS_PULL_DOWN;
			break;
		}
	}

	switch (LL_GPIO_GetPinSpeed(ll_gpio_port, ll_gpio_pin)) {
	case LL_GPIO_SPEED_FREQ_LOW:
		*config |= PINCTRL_CONFIG_SPEED_SLOW;
		break;
	case LL_GPIO_SPEED_FREQ_MEDIUM:
		*config |= PINCTRL_CONFIG_SPEED_MEDIUM;
		break;
	case LL_GPIO_SPEED_FREQ_HIGH:
		*config |= PINCTRL_CONFIG_SPEED_HIGH;
		break;
	}

	switch (LL_GPIO_GetPinOutputType(ll_gpio_port, ll_gpio_pin)) {
	case LL_GPIO_OUTPUT_PUSHPULL:
		*config |= PINCTRL_CONFIG_DRIVE_PUSH_PULL;
		break;
	case LL_GPIO_OUTPUT_OPENDRAIN:
		*config |= PINCTRL_CONFIG_DRIVE_OPEN_DRAIN;
		break;
	}

	return 0;
}

/**
 * @brief Configure a pin.
 *
 * The following configuration options are supported:
 *   - @ref PINCTRL_CONFIG_BIAS_DISABLE
 *   - @ref PINCTRL_CONFIG_BIAS_PULL_UP
 *   - @ref PINCTRL_CONFIG_BIAS_PULL_DOWN
 *   - @ref PINCTRL_CONFIG_BIAS_PULL_PIN_DEFAULT
 *   - @ref PINCTRL_CONFIG_DRIVE_PUSH_PULL
 *   - @ref PINCTRL_CONFIG_DRIVE_OPEN_DRAIN
 *   - @ref PINCTRL_CONFIG_SPEED_SLOW
 *   - @ref PINCTRL_CONFIG_SPEED_MEDIUM
 *   - @ref PINCTRL_CONFIG_SPEED_FAST
 *   - @ref PINCTRL_CONFIG_SPEED_HIGH
 *
 * @note For safety purposes the pin should be reset to input before
 * configuration.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin
 * @param config
 * @retval 0 on success
 * @retval -ENOTSUP if requested pin is not available on this controller
 */
static int pinctrl_stm32_config_set(struct device *dev, u16_t pin, u32_t config)
{
	ARG_UNUSED(dev);

	if (pin >= pinctrl_tmpl_control_get_pins_count(dev)) {
		return -ENOTSUP;
	}

	u32_t ll_gpio_output_type = LL_GPIO_OUTPUT_PUSHPULL;
	u32_t ll_gpio_speed = LL_GPIO_SPEED_FREQ_LOW;
	u32_t ll_gpio_pull = LL_GPIO_PULL_NO;

	GPIO_TypeDef *ll_gpio_port = pinctrl_stm32_ll_gpio_port(pin);
	u32_t ll_gpio_pin = pinctrl_stm32_ll_gpio_pin(pin);

	switch (config & PINCTRL_CONFIG_SPEED_HIGH) {
	case PINCTRL_CONFIG_SPEED_SLOW:
		ll_gpio_speed = LL_GPIO_SPEED_FREQ_LOW;
		break;
	case PINCTRL_CONFIG_SPEED_MEDIUM:
		ll_gpio_speed = LL_GPIO_SPEED_FREQ_MEDIUM;
		break;
	case PINCTRL_CONFIG_SPEED_FAST:
	case PINCTRL_CONFIG_SPEED_HIGH:
		ll_gpio_speed = LL_GPIO_SPEED_FREQ_HIGH;
		break;
	}
	LL_GPIO_SetPinSpeed(ll_gpio_port, ll_gpio_pin, ll_gpio_speed);

	if (config & (PINCTRL_CONFIG_BIAS_DISABLE |
		      PINCTRL_CONFIG_BIAS_PULL_PIN_DEFAULT)) {
		ll_gpio_pull = LL_GPIO_PULL_NO;
	} else if (config & PINCTRL_CONFIG_BIAS_PULL_UP) {
		ll_gpio_pull = LL_GPIO_PULL_UP;
	} else if (config & PINCTRL_CONFIG_BIAS_PULL_DOWN) {
		ll_gpio_pull = LL_GPIO_PULL_DOWN;
	}
	LL_GPIO_SetPinPull(ll_gpio_port, ll_gpio_pin, ll_gpio_pull);

	if (config & PINCTRL_CONFIG_DRIVE_OPEN_DRAIN) {
		ll_gpio_output_type = LL_GPIO_OUTPUT_OPENDRAIN;
	} else if (config & PINCTRL_CONFIG_DRIVE_PUSH_PULL) {
		ll_gpio_output_type = LL_GPIO_OUTPUT_PUSHPULL;
	}
	LL_GPIO_SetPinOutputType(
		ll_gpio_port, ll_gpio_pin, ll_gpio_output_type);

	return 0;
}

/**
 * @brief Get muxing function at pin
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin
 * @param[out] func Muxing function
 * @retval 0 on success
 * @retval -ENOTSUP if requested pin is not available on this controller
 */
static int pinctrl_stm32_mux_get(struct device *dev, u16_t pin, u16_t *func)
{
	ARG_UNUSED(dev);

	if (pin >= pinctrl_tmpl_control_get_pins_count(dev)) {
		return -ENOTSUP;
	}

	GPIO_TypeDef *ll_gpio_port = pinctrl_stm32_ll_gpio_port(pin);
	u32_t ll_gpio_pin = pinctrl_stm32_ll_gpio_pin(pin);

	/* current pin mode (muxing function) */
	switch (LL_GPIO_GetPinMode(ll_gpio_port, ll_gpio_pin)) {
	case LL_GPIO_MODE_ALTERNATE:
		if (ll_gpio_pin <= LL_GPIO_PIN_7) {
			*func = LL_GPIO_GetAFPin_0_7(ll_gpio_port, ll_gpio_pin);
		} else {
			*func = LL_GPIO_GetAFPin_8_15(ll_gpio_port,
						      ll_gpio_pin);
		}
		*func += PINCTRL_STM32_FUNCTION_ALT_0;
		break;
	case LL_GPIO_MODE_OUTPUT:
		*func = PINCTRL_STM32_FUNCTION_OUTPUT;
		break;
	case LL_GPIO_MODE_INPUT:
		*func = PINCTRL_STM32_FUNCTION_INPUT;
		break;
	case LL_GPIO_MODE_ANALOG:
		*func = PINCTRL_STM32_FUNCTION_ANALOG;
		break;
	}

	return 0;
}

/**
 * @brief Set muxing function at pin
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin
 * @param func Muxing function
 * @retval 0 on success
 * @retval -ENOTSUP if requested pin is not available on this controller
 * @retval -EINVAL if requested function is unknown
 */
static int pinctrl_stm32_mux_set(struct device *dev, u16_t pin, u16_t func)
{
	/* func always denotes a hardware pinmux control
	 * due to the usage of the pinctrl_tmpl.
	 */
	GPIO_TypeDef *ll_gpio_port = pinctrl_stm32_ll_gpio_port(pin);
	u32_t ll_gpio_pin = pinctrl_stm32_ll_gpio_pin(pin);
	u32_t ll_gpio_mode = pinctrl_stm32_ll_gpio_mode(func);

	if (ll_gpio_mode == LL_GPIO_MODE_ALTERNATE) {
		if (pinctrl_stm32_ll_gpio_alt_func(func) <= LL_GPIO_AF_7) {
			/* valid alternate function */
			if (pinctrl_stm32_ll_gpio_pin(pin) <= LL_GPIO_PIN_7) {
				LL_GPIO_SetAFPin_0_7(
					ll_gpio_port,
					ll_gpio_pin,
					pinctrl_stm32_ll_gpio_alt_func(func));
			} else {
				LL_GPIO_SetAFPin_8_15(
					ll_gpio_port,
					ll_gpio_pin,
					pinctrl_stm32_ll_gpio_alt_func(func));
			}
		}
	}
	LL_GPIO_SetPinMode(ll_gpio_port, ll_gpio_pin, ll_gpio_mode);
	return 0;
}

/**
 * @brief Initialise pin controller.
 *
 * Enable all GPIOS controlled by the pin controller (currently all GPIOs)
 * to be clocked by the associated peripherial bus clock.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
static int pinctrl_stm32_device_init(struct device *dev)
{
	/* Enable all GPIOS to be clocked by the associated peripherial bus
	 * clock
	 */
	struct pinctrl_stm32_data *data = dev->driver_data;

	data->clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	/**
	 * @code{.codegen}
	 * # GPIO bus was already evaluated above (bus).
	 * codegen.outl('data->pclken_gpio.bus = STM32_CLOCK_BUS_{};'.format(
	 *     bus))
	 * @endcode{.codegen}
	 */
	/** @code{.codeins}@endcode */
	for (int port_idx = 0; port_idx < PINCTRL_STM32_GPIO_COUNT;
	     port_idx++) {
		pinctrl_stm32_gpio_enable_clock(dev, port_idx);
	}

	return 0;
}

/** @} device_driver_pinctrl_stm32 */
