/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for NUCLEO-F429ZI board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_STM32_PORT_3
	{STM32_PIN_PD8, STM32F4_PINMUX_FUNC_PD8_USART3_TX},
	{STM32_PIN_PD9, STM32F4_PINMUX_FUNC_PD9_USART3_RX},
#endif /* #ifdef CONFIG_UART_STM32_PORT_3 */
#ifdef CONFIG_PWM_STM32_2
	{STM32_PIN_PA0, STM32F4_PINMUX_FUNC_PA0_PWM2_CH1},
#endif /* CONFIG_PWM_STM32_2 */
#ifdef CONFIG_ETH_STM32_HAL
	{STM32_PIN_PC1, STM32F4_PINMUX_FUNC_PC1_ETH},
	{STM32_PIN_PC4, STM32F4_PINMUX_FUNC_PC4_ETH},
	{STM32_PIN_PC5, STM32F4_PINMUX_FUNC_PC5_ETH},

	{STM32_PIN_PA1, STM32F4_PINMUX_FUNC_PA1_ETH},
	{STM32_PIN_PA2, STM32F4_PINMUX_FUNC_PA2_ETH},
	{STM32_PIN_PA7, STM32F4_PINMUX_FUNC_PA7_ETH},

	{STM32_PIN_PG11, STM32F4_PINMUX_FUNC_PG11_ETH},
	{STM32_PIN_PG13, STM32F4_PINMUX_FUNC_PG13_ETH},
	{STM32_PIN_PB13, STM32F4_PINMUX_FUNC_PB13_ETH},
#endif /* CONFIG_ETH_STM32_HAL */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
		CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
