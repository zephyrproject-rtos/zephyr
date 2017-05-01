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

#define _PINMUX_PWM(pin, port, chan)		\
	PAD(STM32F4_PINMUX_FUNC_##pin##_PWM##port##_CH##chan,\
	    STM32F4X_PIN_CONFIG_AF_PUSH_UP),

#define _PINMUX_UART(pin, port, dir)		\
	PAD(STM32F4_PINMUX_FUNC_##pin##_##port##_##dir, \
	    STM32F4X_PIN_CONFIG_AF_PUSH_UP),

/* Blank pinmux by default */
#define PINMUX_PWM2(pin, chan)
#define PINMUX_UART1(pin, dir)
#define PINMUX_UART2(pin, dir)
#define PINMUX_UART3(pin, dir)
#define PINMUX_UART4(pin, dir)
#define PINMUX_UART5(pin, dir)
#define PINMUX_UART6(pin, dir)
#define PINMUX_UART7(pin, dir)
#define PINMUX_UART8(pin, dir)
#define PINMUX_UART9(pin, dir)
#define PINMUX_UART10(pin, dir)

#ifdef CONFIG_PWM_STM32_2
	#undef PINMUX_PWM2
	#define PINMUX_PWM2(pin, chan)		_PINMUX_PWM(pin, 2, chan)
#endif

#ifdef CONFIG_UART_STM32_PORT_1
	#undef PINMUX_UART1
	#define PINMUX_UART1(pin, dir)		_PINMUX_UART(pin, USART1, dir)
#endif

#ifdef CONFIG_UART_STM32_PORT_2
	#undef PINMUX_UART2
	#define PINMUX_UART2(pin, dir)		_PINMUX_UART(pin, USART2, dir)
#endif

#ifdef CONFIG_UART_STM32_PORT_3
	#undef PINMUX_UART3
	#define PINMUX_UART3(pin, dir)		_PINMUX_UART(pin, USART3, dir)
#endif

#ifdef CONFIG_UART_STM32_PORT_4
	#undef PINMUX_UART4
	#define PINMUX_UART4(pin, dir)		_PINMUX_UART(pin, UART4, dir)
#endif

#ifdef CONFIG_UART_STM32_PORT_5
	#undef PINMUX_UART5
	#define PINMUX_UART5(pin, dir)		_PINMUX_UART(pin, UART5, dir)
#endif

#ifdef CONFIG_UART_STM32_PORT_6
	#undef PINMUX_UART6
	#define PINMUX_UART6(pin, dir)		_PINMUX_UART(pin, USART6, dir)
#endif

#ifdef CONFIG_UART_STM32_PORT_7
	#undef PINMUX_UART7
	#define PINMUX_UART7(pin, dir)		_PINMUX_UART(pin, UART7, dir)
#endif

#ifdef CONFIG_UART_STM32_PORT_8
	#undef PINMUX_UART8
	#define PINMUX_UART8(pin, dir)		_PINMUX_UART(pin, UART8, dir)
#endif

#ifdef CONFIG_UART_STM32_PORT_9
	#undef PINMUX_UART9
	#define PINMUX_UART9(pin, dir)		_PINMUX_UART(pin, UART9, dir)
#endif

#ifdef CONFIG_UART_STM32_PORT_10
	#undef PINMUX_UART10
	#define PINMUX_UART10(pin, dir)		_PINMUX_UART(pin, UART10, dir)
#endif

#define PINMUX_PWM(pin, pwm, chan)		PINMUX_##pwm(pin, chan)
#define PINMUX_UART(pin, port, dir)		PINMUX_##port(pin, dir)

/* Port A */
static const stm32_pin_func_t pin_pa0_funcs[] = {
	PINMUX_PWM(PA0, PWM2, 1)
	PINMUX_UART(PA0, UART4, TX)
};

static const stm32_pin_func_t pin_pa1_funcs[] = {
	PINMUX_UART(PA1, UART4, RX)
};

static const stm32_pin_func_t pin_pa2_funcs[] = {
	PINMUX_UART(PA2, UART2, TX)
};

static const stm32_pin_func_t pin_pa3_funcs[] = {
	PINMUX_UART(PA3, UART2, RX)
};

static const stm32_pin_func_t pin_pa8_funcs[] = {
	PINMUX_UART(PA8, UART7, RX)
};

static const stm32_pin_func_t pin_pa9_funcs[] = {
	PINMUX_UART(PA9, UART1, TX)
};

static const stm32_pin_func_t pin_pa10_funcs[] = {
	PINMUX_UART(PA10, UART1, RX)
};

static const stm32_pin_func_t pin_pa11_funcs[] = {
	PINMUX_UART(PA11, UART6, TX)
	PINMUX_UART(PA11, UART4, RX)
};

static const stm32_pin_func_t pin_pa12_funcs[] = {
	PINMUX_UART(PA12, UART6, RX)
	PINMUX_UART(PA12, UART4, TX)
};

static const stm32_pin_func_t pin_pa15_funcs[] = {
	PINMUX_UART(PA15, UART1, TX)
	PINMUX_UART(PA15, UART7, TX)
};

/* Port B */
static const stm32_pin_func_t pin_pb3_funcs[] = {
	PINMUX_UART(PB3, UART1, RX)
	PINMUX_UART(PB3, UART7, RX)
};

static const stm32_pin_func_t pin_pb4_funcs[] = {
	PINMUX_UART(PB4, UART7, TX)
};

static const stm32_pin_func_t pin_pb5_funcs[] = {
	PINMUX_UART(PB5, UART5, RX)
};

static const stm32_pin_func_t pin_pb6_funcs[] = {
	PINMUX_UART(PB6, UART1, TX)
	PINMUX_UART(PB6, UART5, TX)
};

static const stm32_pin_func_t pin_pb7_funcs[] = {
	PINMUX_UART(PB7, UART1, RX)
};

static const stm32_pin_func_t pin_pb8_funcs[] = {
	PINMUX_UART(PB8, UART5, RX)
};

static const stm32_pin_func_t pin_pb9_funcs[] = {
	PINMUX_UART(PB9, UART5, TX)
};

static const stm32_pin_func_t pin_pb10_funcs[] = {
	PINMUX_UART(PB10, UART3, TX)
};

static const stm32_pin_func_t pin_pb11_funcs[] = {
	PINMUX_UART(PB11, UART3, RX)
};

static const stm32_pin_func_t pin_pb12_funcs[] = {
	PINMUX_UART(PB12, UART5, RX)
};

static const stm32_pin_func_t pin_pb13_funcs[] = {
	PINMUX_UART(PB13, UART5, TX)
};

/* Port C */
static const stm32_pin_func_t pin_pc5_funcs[] = {
	PINMUX_UART(PC5, UART3, RX)
};

static const stm32_pin_func_t pin_pc6_funcs[] = {
	PINMUX_UART(PC6, UART6, TX)
};

static const stm32_pin_func_t pin_pc7_funcs[] = {
	PINMUX_UART(PC7, UART6, RX)
};

static const stm32_pin_func_t pin_pc10_funcs[] = {
	PINMUX_UART(PC10, UART3, TX)
};

static const stm32_pin_func_t pin_pc11_funcs[] = {
	PINMUX_UART(PC11, UART3, RX)
	PINMUX_UART(PC11, UART4, RX)
};

static const stm32_pin_func_t pin_pc12_funcs[] = {
	PINMUX_UART(PC12, UART5, TX)
};

/* Port D */
static const stm32_pin_func_t pin_pd0_funcs[] = {
	PINMUX_UART(PD0, UART4, RX)
};

static const stm32_pin_func_t pin_pd2_funcs[] = {
	PINMUX_UART(PD2, UART5, RX)
};

static const stm32_pin_func_t pin_pd5_funcs[] = {
	PINMUX_UART(PD5, UART2, TX)
};

static const stm32_pin_func_t pin_pd6_funcs[] = {
	PINMUX_UART(PD6, UART2, RX)
};

static const stm32_pin_func_t pin_pd8_funcs[] = {
	PINMUX_UART(PD8, UART3, TX)
};

static const stm32_pin_func_t pin_pd9_funcs[] = {
	PINMUX_UART(PD9, UART3, RX)
};

static const stm32_pin_func_t pin_pd10_funcs[] = {
	PINMUX_UART(PD10, UART4, TX)
};

static const stm32_pin_func_t pin_pd14_funcs[] = {
	PINMUX_UART(PD14, UART9, RX)
};

static const stm32_pin_func_t pin_pd15_funcs[] = {
	PINMUX_UART(PD15, UART9, TX)
};

/* Port E */
static const stm32_pin_func_t pin_pe0_funcs[] = {
	PINMUX_UART(PE0, UART8, RX)
};

static const stm32_pin_func_t pin_pe1_funcs[] = {
	PINMUX_UART(PE1, UART8, TX)
};

static const stm32_pin_func_t pin_pe2_funcs[] = {
	PINMUX_UART(PE2, UART10, RX)
};

static const stm32_pin_func_t pin_pe3_funcs[] = {
	PINMUX_UART(PE3, UART10, TX)
};

static const stm32_pin_func_t pin_pe7_funcs[] = {
	PINMUX_UART(PE7, UART7, RX)
};

static const stm32_pin_func_t pin_pe8_funcs[] = {
	PINMUX_UART(PE8, UART7, TX)
};

/* Port F */
static const stm32_pin_func_t pin_pf6_funcs[] = {
	PINMUX_UART(PF6, UART7, RX)
};

static const stm32_pin_func_t pin_pf7_funcs[] = {
	PINMUX_UART(PF7, UART7, TX)
};

static const stm32_pin_func_t pin_pf8_funcs[] = {
	PINMUX_UART(PF8, UART8, RX)
};

static const stm32_pin_func_t pin_pf9_funcs[] = {
	PINMUX_UART(PF9, UART8, TX)
};

/* Port G */
static const stm32_pin_func_t pin_pg0_funcs[] = {
	PINMUX_UART(PG0, UART9, RX)
};

static const stm32_pin_func_t pin_pg1_funcs[] = {
	PINMUX_UART(PG1, UART9, TX)
};

static const stm32_pin_func_t pin_pg9_funcs[] = {
	PINMUX_UART(PG9, UART6, RX)
};

static const stm32_pin_func_t pin_pg11_funcs[] = {
	PINMUX_UART(PG11, UART10, RX)
};

static const stm32_pin_func_t pin_pg12_funcs[] = {
	PINMUX_UART(PG12, UART10, TX)
};

static const stm32_pin_func_t pin_pg14_funcs[] = {
	PINMUX_UART(PG14, UART6, TX)
};

/**
 * @brief pin configuration
 */
static const struct stm32_pinmux_conf pins[] = {
	/* Port A */
	STM32_PIN_CONF(STM32_PIN_PA0, pin_pa0_funcs),
	STM32_PIN_CONF(STM32_PIN_PA1, pin_pa1_funcs),
	STM32_PIN_CONF(STM32_PIN_PA2, pin_pa2_funcs),
	STM32_PIN_CONF(STM32_PIN_PA3, pin_pa3_funcs),
	STM32_PIN_CONF(STM32_PIN_PA8, pin_pa8_funcs),
	STM32_PIN_CONF(STM32_PIN_PA9, pin_pa9_funcs),
	STM32_PIN_CONF(STM32_PIN_PA10, pin_pa10_funcs),
	STM32_PIN_CONF(STM32_PIN_PA11, pin_pa11_funcs),
	STM32_PIN_CONF(STM32_PIN_PA12, pin_pa12_funcs),
	STM32_PIN_CONF(STM32_PIN_PA15, pin_pa15_funcs),

	/* Port B */
	STM32_PIN_CONF(STM32_PIN_PB3, pin_pb3_funcs),
	STM32_PIN_CONF(STM32_PIN_PB4, pin_pb4_funcs),
	STM32_PIN_CONF(STM32_PIN_PB5, pin_pb5_funcs),
	STM32_PIN_CONF(STM32_PIN_PB6, pin_pb6_funcs),
	STM32_PIN_CONF(STM32_PIN_PB7, pin_pb7_funcs),
	STM32_PIN_CONF(STM32_PIN_PB8, pin_pb8_funcs),
	STM32_PIN_CONF(STM32_PIN_PB9, pin_pb9_funcs),
	STM32_PIN_CONF(STM32_PIN_PB10, pin_pb10_funcs),
	STM32_PIN_CONF(STM32_PIN_PB11, pin_pb11_funcs),
	STM32_PIN_CONF(STM32_PIN_PB12, pin_pb12_funcs),
	STM32_PIN_CONF(STM32_PIN_PB13, pin_pb13_funcs),

	/* Port C */
	STM32_PIN_CONF(STM32_PIN_PC5, pin_pc5_funcs),
	STM32_PIN_CONF(STM32_PIN_PC6, pin_pc6_funcs),
	STM32_PIN_CONF(STM32_PIN_PC7, pin_pc7_funcs),
	STM32_PIN_CONF(STM32_PIN_PC10, pin_pc10_funcs),
	STM32_PIN_CONF(STM32_PIN_PC11, pin_pc11_funcs),
	STM32_PIN_CONF(STM32_PIN_PC12, pin_pc12_funcs),

	/* Port D */
	STM32_PIN_CONF(STM32_PIN_PD0, pin_pd0_funcs),
	STM32_PIN_CONF(STM32_PIN_PD2, pin_pd2_funcs),
	STM32_PIN_CONF(STM32_PIN_PD5, pin_pd5_funcs),
	STM32_PIN_CONF(STM32_PIN_PD6, pin_pd6_funcs),
	STM32_PIN_CONF(STM32_PIN_PD8, pin_pd8_funcs),
	STM32_PIN_CONF(STM32_PIN_PD9, pin_pd9_funcs),
	STM32_PIN_CONF(STM32_PIN_PD10, pin_pd10_funcs),
	STM32_PIN_CONF(STM32_PIN_PD14, pin_pd14_funcs),
	STM32_PIN_CONF(STM32_PIN_PD15, pin_pd15_funcs),

	/* Port E */
	STM32_PIN_CONF(STM32_PIN_PE0, pin_pe0_funcs),
	STM32_PIN_CONF(STM32_PIN_PE1, pin_pe1_funcs),
	STM32_PIN_CONF(STM32_PIN_PE2, pin_pe2_funcs),
	STM32_PIN_CONF(STM32_PIN_PE3, pin_pe3_funcs),
	STM32_PIN_CONF(STM32_PIN_PE7, pin_pe7_funcs),
	STM32_PIN_CONF(STM32_PIN_PE8, pin_pe8_funcs),

	/* Port F */
	STM32_PIN_CONF(STM32_PIN_PF6, pin_pf6_funcs),
	STM32_PIN_CONF(STM32_PIN_PF7, pin_pf7_funcs),
	STM32_PIN_CONF(STM32_PIN_PF8, pin_pf8_funcs),
	STM32_PIN_CONF(STM32_PIN_PF9, pin_pf9_funcs),

	/* Port G */
	STM32_PIN_CONF(STM32_PIN_PG0, pin_pg0_funcs),
	STM32_PIN_CONF(STM32_PIN_PG1, pin_pg1_funcs),
	STM32_PIN_CONF(STM32_PIN_PG9, pin_pg9_funcs),
	STM32_PIN_CONF(STM32_PIN_PG11, pin_pg11_funcs),
	STM32_PIN_CONF(STM32_PIN_PG12, pin_pg12_funcs),
	STM32_PIN_CONF(STM32_PIN_PG14, pin_pg14_funcs),
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
