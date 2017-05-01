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
#define PINMUX_UART3(dir, pin)
#define PINMUX_UART4(dir, pin)
#define PINMUX_UART5(dir, pin)
#define PINMUX_UART6(dir, pin)
#define PINMUX_UART7(dir, pin)
#define PINMUX_UART8(dir, pin)
#define PINMUX_UART9(dir, pin)
#define PINMUX_UART10(dir, pin)

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

#ifdef CONFIG_UART_STM32_PORT_3
	#undef PINMUX_UART3
	#define PINMUX_UART3(dir, pin)		PINMUX_UART(USART3, dir, pin)
#endif

#ifdef CONFIG_UART_STM32_PORT_4
	#undef PINMUX_UART4
	#define PINMUX_UART4(dir, pin)		PINMUX_UART(UART4, dir, pin)
#endif

#ifdef CONFIG_UART_STM32_PORT_5
	#undef PINMUX_UART5
	#define PINMUX_UART5(dir, pin)		PINMUX_UART(UART5, dir, pin)
#endif

#ifdef CONFIG_UART_STM32_PORT_6
	#undef PINMUX_UART6
	#define PINMUX_UART6(dir, pin)		PINMUX_UART(USART6, dir, pin)
#endif

#ifdef CONFIG_UART_STM32_PORT_7
	#undef PINMUX_UART7
	#define PINMUX_UART7(dir, pin)		PINMUX_UART(UART7, dir, pin)
#endif

#ifdef CONFIG_UART_STM32_PORT_8
	#undef PINMUX_UART8
	#define PINMUX_UART8(dir, pin)		PINMUX_UART(UART8, dir, pin)
#endif

#ifdef CONFIG_UART_STM32_PORT_9
	#undef PINMUX_UART9
	#define PINMUX_UART9(dir, pin)		PINMUX_UART(UART9, dir, pin)
#endif

#ifdef CONFIG_UART_STM32_PORT_10
	#undef PINMUX_UART10
	#define PINMUX_UART10(dir, pin)		PINMUX_UART(UART10, dir, pin)
#endif

#define PINMUX_UART_TX(pin, port)		PINMUX_UART##port(TX, pin)
#define PINMUX_UART_RX(pin, port)		PINMUX_UART##port(RX, pin)

/* Port A */
static const stm32_pin_func_t pin_pa0_funcs[] = {
	PINMUX_PWM2(PA0, 1)
	PINMUX_UART_TX(PA0, 4)
};

static const stm32_pin_func_t pin_pa1_funcs[] = {
	PINMUX_UART_RX(PA1, 4)
};

static const stm32_pin_func_t pin_pa2_funcs[] = {
	PINMUX_UART_TX(PA2, 2)
};

static const stm32_pin_func_t pin_pa3_funcs[] = {
	PINMUX_UART_RX(PA3, 2)
};

static const stm32_pin_func_t pin_pa8_funcs[] = {
	PINMUX_UART_RX(PA8, 7)
};

static const stm32_pin_func_t pin_pa9_funcs[] = {
	PINMUX_UART_TX(PA9, 1)
};

static const stm32_pin_func_t pin_pa10_funcs[] = {
	PINMUX_UART_RX(PA10, 1)
};

static const stm32_pin_func_t pin_pa11_funcs[] = {
	PINMUX_UART_TX(PA11, 6)
	PINMUX_UART_RX(PA11, 4)
};

static const stm32_pin_func_t pin_pa12_funcs[] = {
	PINMUX_UART_RX(PA12, 6)
	PINMUX_UART_TX(PA12, 4)
};

static const stm32_pin_func_t pin_pa15_funcs[] = {
	PINMUX_UART_TX(PA15, 1)
	PINMUX_UART_TX(PA15, 7)
};

/* Port B */
static const stm32_pin_func_t pin_pb3_funcs[] = {
	PINMUX_UART_RX(PB3, 1)
	PINMUX_UART_RX(PB3, 7)
};

static const stm32_pin_func_t pin_pb4_funcs[] = {
	PINMUX_UART_TX(PB4, 7)
};

static const stm32_pin_func_t pin_pb5_funcs[] = {
	PINMUX_UART_RX(PB5, 5)
};

static const stm32_pin_func_t pin_pb6_funcs[] = {
	PINMUX_UART_TX(PB6, 1)
	PINMUX_UART_TX(PB6, 5)
};

static const stm32_pin_func_t pin_pb7_funcs[] = {
	PINMUX_UART_RX(PB7, 1)
};

static const stm32_pin_func_t pin_pb8_funcs[] = {
	PINMUX_UART_RX(PB8, 5)
};

static const stm32_pin_func_t pin_pb9_funcs[] = {
	PINMUX_UART_TX(PB9, 5)
};

static const stm32_pin_func_t pin_pb10_funcs[] = {
	PINMUX_UART_TX(PB10, 3)
};

static const stm32_pin_func_t pin_pb11_funcs[] = {
	PINMUX_UART_RX(PB11, 3)
};

static const stm32_pin_func_t pin_pb12_funcs[] = {
	PINMUX_UART_RX(PB12, 5)
};

static const stm32_pin_func_t pin_pb13_funcs[] = {
	PINMUX_UART_TX(PB13, 5)
};

/* Port C */
static const stm32_pin_func_t pin_pc5_funcs[] = {
	PINMUX_UART_RX(PC5, 3)
};

static const stm32_pin_func_t pin_pc6_funcs[] = {
	PINMUX_UART_TX(PC6, 6)
};

static const stm32_pin_func_t pin_pc7_funcs[] = {
	PINMUX_UART_RX(PC7, 6)
};

static const stm32_pin_func_t pin_pc10_funcs[] = {
	PINMUX_UART_TX(PC10, 3)
};

static const stm32_pin_func_t pin_pc11_funcs[] = {
	PINMUX_UART_RX(PC11, 3)
	PINMUX_UART_RX(PC11, 4)
};

static const stm32_pin_func_t pin_pc12_funcs[] = {
	PINMUX_UART_TX(PC12, 5)
};

/* Port D */
static const stm32_pin_func_t pin_pd0_funcs[] = {
	PINMUX_UART_RX(PD0, 4)
};

static const stm32_pin_func_t pin_pd2_funcs[] = {
	PINMUX_UART_RX(PD2, 5)
};

static const stm32_pin_func_t pin_pd5_funcs[] = {
	PINMUX_UART_TX(PD5, 2)
};

static const stm32_pin_func_t pin_pd6_funcs[] = {
	PINMUX_UART_RX(PD6, 2)
};

static const stm32_pin_func_t pin_pd8_funcs[] = {
	PINMUX_UART_TX(PD8, 3)
};

static const stm32_pin_func_t pin_pd9_funcs[] = {
	PINMUX_UART_RX(PD9, 3)
};

static const stm32_pin_func_t pin_pd10_funcs[] = {
	PINMUX_UART_TX(PD10, 4)
};

static const stm32_pin_func_t pin_pd14_funcs[] = {
	PINMUX_UART_RX(PD14, 9)
};

static const stm32_pin_func_t pin_pd15_funcs[] = {
	PINMUX_UART_TX(PD15, 9)
};

/* Port E */
static const stm32_pin_func_t pin_pe0_funcs[] = {
	PINMUX_UART_RX(PE0, 8)
};

static const stm32_pin_func_t pin_pe1_funcs[] = {
	PINMUX_UART_TX(PE1, 8)
};

static const stm32_pin_func_t pin_pe2_funcs[] = {
	PINMUX_UART_RX(PE2, 10)
};

static const stm32_pin_func_t pin_pe3_funcs[] = {
	PINMUX_UART_TX(PE3, 10)
};

static const stm32_pin_func_t pin_pe7_funcs[] = {
	PINMUX_UART_RX(PE7, 7)
};

static const stm32_pin_func_t pin_pe8_funcs[] = {
	PINMUX_UART_TX(PE8, 7)
};

/* Port F */
static const stm32_pin_func_t pin_pf6_funcs[] = {
	PINMUX_UART_RX(PF6, 7)
};

static const stm32_pin_func_t pin_pf7_funcs[] = {
	PINMUX_UART_TX(PF7, 7)
};

static const stm32_pin_func_t pin_pf8_funcs[] = {
	PINMUX_UART_RX(PF8, 8)
};

static const stm32_pin_func_t pin_pf9_funcs[] = {
	PINMUX_UART_TX(PF9, 8)
};

/* Port G */
static const stm32_pin_func_t pin_pg0_funcs[] = {
	PINMUX_UART_RX(PG0, 9)
};

static const stm32_pin_func_t pin_pg1_funcs[] = {
	PINMUX_UART_TX(PG1, 9)
};

static const stm32_pin_func_t pin_pg9_funcs[] = {
	PINMUX_UART_RX(PG9, 6)
};

static const stm32_pin_func_t pin_pg11_funcs[] = {
	PINMUX_UART_RX(PG11, 10)
};

static const stm32_pin_func_t pin_pg12_funcs[] = {
	PINMUX_UART_TX(PG12, 10)
};

static const stm32_pin_func_t pin_pg14_funcs[] = {
	PINMUX_UART_TX(PG14, 6)
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
