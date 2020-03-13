/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* NUCLEO-H743ZI pin configurations */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_3
	{ STM32_PIN_PD8, STM32H7_PINMUX_FUNC_PD8_USART3_TX },
	{ STM32_PIN_PD9, STM32H7_PINMUX_FUNC_PD9_USART3_RX },
#endif /* CONFIG_UART_3 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
