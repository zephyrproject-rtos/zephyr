/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <sys_io.h>
#include "pinmux/pinmux.h"

#include "pinmux_stm32.h"

/* pin assignments for NUCLEO-F334RB board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_STM32_PORT_1
	{STM32_PIN_PA9,  STM32F3_PINMUX_FUNC_PA9_USART1_TX},
	{STM32_PIN_PA10, STM32F3_PINMUX_FUNC_PA10_USART1_RX},
#endif	/* CONFIG_UART_STM32_PORT_1 */
#ifdef CONFIG_UART_STM32_PORT_2
	{STM32_PIN_PA2, STM32F3_PINMUX_FUNC_PA2_USART2_TX},
	{STM32_PIN_PA3, STM32F3_PINMUX_FUNC_PA3_USART2_RX},
#endif	/* CONFIG_UART_STM32_PORT_2 */
#ifdef CONFIG_UART_STM32_PORT_3
	{STM32_PIN_PB10, STM32F3_PINMUX_FUNC_PB10_USART3_TX},
	{STM32_PIN_PB11, STM32F3_PINMUX_FUNC_PB11_USART3_RX},
#endif	/* CONFIG_UART_STM32_PORT_3 */
#ifdef CONFIG_PWM_STM32_1
	{STM32_PIN_PA8, STM32F3_PINMUX_FUNC_PA8_PWM1_CH1},
#endif /* CONFIG_PWM_STM32_1 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
