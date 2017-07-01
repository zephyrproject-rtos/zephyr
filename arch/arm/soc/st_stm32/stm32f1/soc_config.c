/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
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

#define _PINMUX_PWM(pin, port, chan)		\
	PAD(STM32F1_PINMUX_FUNC_##pin##_PWM##port##_CH##chan,\
	    STM32F10X_PIN_CONFIG_AF_PUSH_PULL),

#define _PINMUX_UART_TX(pin, port)		\
	PAD(STM32F1_PINMUX_FUNC_##pin##_##port##_TX, \
	    STM32F10X_PIN_CONFIG_AF_PUSH_PULL),

#define _PINMUX_UART_RX(pin, port)		\
	PAD(STM32F1_PINMUX_FUNC_##pin##_##port##_RX, \
	    STM32F10X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE),

#define _PINMUX_I2C(pin, port, line)		\
	PAD(STM32F1_PINMUX_FUNC_##pin##_I2C##port##_##line, \
	    STM32F10X_PIN_CONFIG_AF_OPEN_DRAIN),

/* Blank pinmux by default */
#define PINMUX_I2C1(pin, line)
#define PINMUX_I2C2(pin, line)
#define PINMUX_PWM1(pin, chan)
#define PINMUX_UART1_TX(pin)
#define PINMUX_UART1_RX(pin)
#define PINMUX_UART2_TX(pin)
#define PINMUX_UART2_RX(pin)
#define PINMUX_UART3_TX(pin)
#define PINMUX_UART3_RX(pin)

#ifdef CONFIG_I2C_1
	#undef PINMUX_I2C1
	#define PINMUX_I2C1(pin, line)		_PINMUX_I2C(pin, 1, line)
#endif

#ifdef CONFIG_I2C_2
	#undef PINMUX_I2C2
	#define PINMUX_I2C2(pin, line)		_PINMUX_I2C(pin, 2, line)
#endif

#ifdef CONFIG_PWM_STM32_1
	#undef PINMUX_PWM1
	#define PINMUX_PWM1(pin, chan)		_PINMUX_PWM(pin, 1, chan)
#endif

#ifdef CONFIG_UART_STM32_PORT_1
	#undef PINMUX_UART1_TX
	#undef PINMUX_UART1_RX
	#define PINMUX_UART1_TX(pin)		_PINMUX_UART_TX(pin, USART1)
	#define PINMUX_UART1_RX(pin)		_PINMUX_UART_RX(pin, USART1)
#endif

#ifdef CONFIG_UART_STM32_PORT_2
	#undef PINMUX_UART2_TX
	#undef PINMUX_UART2_RX
	#define PINMUX_UART2_TX(pin)		_PINMUX_UART_TX(pin, USART2)
	#define PINMUX_UART2_RX(pin)		_PINMUX_UART_RX(pin, USART2)
#endif

#ifdef CONFIG_UART_STM32_PORT_3
	#undef PINMUX_UART3_TX
	#undef PINMUX_UART3_RX
	#define PINMUX_UART3_TX(pin)		_PINMUX_UART_TX(pin, USART3)
	#define PINMUX_UART3_RX(pin)		_PINMUX_UART_RX(pin, USART3)
#endif

#define PINMUX_PWM(pin, pwm, chan)		PINMUX_##pwm(pin, chan)
#define PINMUX_UART_TX(pin, port)		PINMUX_##port##_TX(pin)
#define PINMUX_UART_RX(pin, port)		PINMUX_##port##_RX(pin)
#define PINMUX_I2C(pin, i2c, line)		PINMUX_##i2c(pin, line)

static const stm32_pin_func_t pin_pa2_funcs[] = {
	PINMUX_UART_TX(PA2, UART2)
};

static const stm32_pin_func_t pin_pa3_funcs[] = {
	PINMUX_UART_RX(PA3, UART2)
};

static const stm32_pin_func_t pin_pa8_funcs[] = {
	PINMUX_PWM(PA8, PWM1, 1)
};

static const stm32_pin_func_t pin_pa9_funcs[] = {
	PINMUX_UART_TX(PA9, UART1)
};

static const stm32_pin_func_t pin_pa10_funcs[] = {
	PINMUX_UART_RX(PA10, UART1)
};

static const stm32_pin_func_t pin_pb6_funcs[] = {
	PINMUX_I2C(PB6, I2C1, SCL)
};

static const stm32_pin_func_t pin_pb7_funcs[] = {
	PINMUX_I2C(PB7, I2C1, SDA)
};

static const stm32_pin_func_t pin_pb8_funcs[] = {
	PINMUX_I2C(PB8, I2C1, SCL)
};

static const stm32_pin_func_t pin_pb9_funcs[] = {
	PINMUX_I2C(PB9, I2C1, SDA)
};

static const stm32_pin_func_t pin_pb10_funcs[] = {
	PINMUX_UART_TX(PB10, UART3)
	PINMUX_I2C(PB10, I2C2, SCL)
};

static const stm32_pin_func_t pin_pb11_funcs[] = {
	PINMUX_UART_RX(PB11, UART3)
	PINMUX_I2C(PB11, I2C2, SDA)
};

/**
 * @brief pin configuration
 */
static struct stm32_pinmux_conf pins[] = {
	STM32_PIN_CONF(STM32_PIN_PA2, pin_pa2_funcs),
	STM32_PIN_CONF(STM32_PIN_PA3, pin_pa3_funcs),
	STM32_PIN_CONF(STM32_PIN_PA8, pin_pa8_funcs),
	STM32_PIN_CONF(STM32_PIN_PA9, pin_pa9_funcs),
	STM32_PIN_CONF(STM32_PIN_PA10, pin_pa10_funcs),
	STM32_PIN_CONF(STM32_PIN_PB6, pin_pb6_funcs),
	STM32_PIN_CONF(STM32_PIN_PB7, pin_pb7_funcs),
	STM32_PIN_CONF(STM32_PIN_PB8, pin_pb8_funcs),
	STM32_PIN_CONF(STM32_PIN_PB9, pin_pb9_funcs),
	STM32_PIN_CONF(STM32_PIN_PB10, pin_pb10_funcs),
	STM32_PIN_CONF(STM32_PIN_PB11, pin_pb11_funcs),
};

int stm32_get_pin_config(int pin, int func)
{
	/* GPIO function is always available, to save space it is not
	 * listed in alternate functions array
	 */
	if (func == STM32_PINMUX_FUNC_GPIO) {
		return STM32F10X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE;
	}

	/* analog function is another 'known' setting */
	if (func == STM32_PINMUX_FUNC_ANALOG) {
		return STM32F10X_PIN_CONFIG_ANALOG;
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
