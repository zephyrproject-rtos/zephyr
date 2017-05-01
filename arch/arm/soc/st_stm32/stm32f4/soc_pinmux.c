/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include "soc.h"
#include <device.h>
#include <misc/util.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <drivers/clock_control/stm32_clock_control.h>

#define PAD(AF, func)				\
				[AF - 1] = func

#define PINMUX_PWM(port, chan, pin)		\
	PAD(STM32F4_PINMUX_FUNC_##pin##_PWM##port##_CH##chan,\
	    STM32F4X_PIN_CONFIG_AF_PUSH_UP),

#define PINMUX_UART(port, dir, pin)		\
	PAD(STM32F4_PINMUX_FUNC_##pin##_##port##_##dir, \
	    STM32F4X_PIN_CONFIG_AF_PUSH_UP),

/* Blank pinmux by default */
#define PINMUX_PWM2(pin, chan)
#define PINMUX_UART1(dir, pin)
#define PINMUX_UART2(dir, pin)

#ifdef CONFIG_PWM_STM32_2
	#undef PINMUX_PWM2
	#define PINMUX_PWM2(pin, chan)		PINMUX_PWM(2, chan, pin)
#endif

#ifdef CONFIG_UART_STM32_PORT_1
	#undef PINMUX_UART1
	#define PINMUX_UART1(dir, pin)		PINMUX_UART(USART1, dir, pin)
#endif

#ifdef CONFIG_UART_STM32_PORT_2
	#undef PINMUX_UART2
	#define PINMUX_UART2(dir, pin)		PINMUX_UART(USART2, dir, pin)
#endif

#define PINMUX_UART_TX(pin, port)		PINMUX_UART##port(TX, pin)
#define PINMUX_UART_RX(pin, port)		PINMUX_UART##port(RX, pin)

static const stm32_pin_func_t pin_pa0_funcs[] = {
	PINMUX_PWM2(PA0, 1)
};

static const stm32_pin_func_t pin_pa2_funcs[] = {
	PINMUX_UART_TX(PA2, 2)
};

static const stm32_pin_func_t pin_pa3_funcs[] = {
	PINMUX_UART_RX(PA3, 2)
};

static const stm32_pin_func_t pin_pa9_funcs[] = {
	PINMUX_UART_TX(PA9, 1)
};

static const stm32_pin_func_t pin_pa10_funcs[] = {
	PINMUX_UART_RX(PA10, 1)
};

static const stm32_pin_func_t pin_pb6_funcs[] = {
	PINMUX_UART_TX(PB6, 1)
};

static const stm32_pin_func_t pin_pb7_funcs[] = {
	PINMUX_UART_RX(PB7, 1)
};

/**
 * @brief pin configuration
 */
static const struct stm32_pinmux_conf pins[] = {
	STM32_PIN_CONF(STM32_PIN_PA0, pin_pa0_funcs),
	STM32_PIN_CONF(STM32_PIN_PA2, pin_pa2_funcs),
	STM32_PIN_CONF(STM32_PIN_PA3, pin_pa3_funcs),
	STM32_PIN_CONF(STM32_PIN_PA9, pin_pa9_funcs),
	STM32_PIN_CONF(STM32_PIN_PA10, pin_pa10_funcs),
	STM32_PIN_CONF(STM32_PIN_PB6, pin_pb6_funcs),
	STM32_PIN_CONF(STM32_PIN_PB7, pin_pb7_funcs),
};

int stm32_get_pin_config(int pin, int func)
{
	/* GPIO function is always available, to save space it is not
	 * listed in alternate functions array
	 */
	if (func == STM32_PINMUX_FUNC_GPIO) {
		return STM32F4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE;
	}

	/* analog function is another 'known' setting */
	if (func == STM32_PINMUX_FUNC_ANALOG) {
		return STM32F4X_PIN_CONFIG_ANALOG;
	}

	func -= 1;

	for (int i = 0; i < ARRAY_SIZE(pins); i++) {
		if (pins[i].pin == pin) {
			if (func > pins[i].nfuncs) {
				return -EINVAL;
			}

			return pins[i].funcs[func];
		}
	}

	return -EINVAL;
}
