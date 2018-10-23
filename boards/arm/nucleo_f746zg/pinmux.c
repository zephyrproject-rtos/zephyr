/*
 * Copyright (c) 2018 Adam Palmer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* NUCLEO-F746ZG pin configurations  */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_STM32_PORT_2
	{ STM32_PIN_PD5, STM32F7_PINMUX_FUNC_PD5_USART2_TX },
	{ STM32_PIN_PD6, STM32F7_PINMUX_FUNC_PD6_USART2_RX },
	{ STM32_PIN_PD4, STM32F7_PINMUX_FUNC_PD4_USART2_RTS },
	{ STM32_PIN_PD3, STM32F7_PINMUX_FUNC_PD3_USART2_CTS },
#endif
#ifdef CONFIG_UART_STM32_PORT_3
	{ STM32_PIN_PD8, STM32F7_PINMUX_FUNC_PD8_USART3_TX },
	{ STM32_PIN_PD9, STM32F7_PINMUX_FUNC_PD9_USART3_RX },
#endif
#ifdef CONFIG_UART_STM32_PORT_6
	{ STM32_PIN_PG14, STM32F7_PINMUX_FUNC_PG14_USART6_TX },
	{ STM32_PIN_PG9, STM32F7_PINMUX_FUNC_PG9_USART6_RX },
#endif
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
