/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for STM32373C-EVAL board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_2
	{STM32_PIN_PD5, STM32F3_PINMUX_FUNC_PD5_USART2_TX},
	{STM32_PIN_PD6, STM32F3_PINMUX_FUNC_PD6_USART2_RX},
#endif	/* CONFIG_UART_2 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
