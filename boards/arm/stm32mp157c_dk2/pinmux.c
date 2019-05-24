/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for STM32MP157c_dk2 board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_3
	{ STM32_PIN_PB10, STM32MP1X_PINMUX_FUNC_PB10_USART3_TX },
	{ STM32_PIN_PB12, STM32MP1X_PINMUX_FUNC_PB12_USART3_RX },
#endif  /* CONFIG_UART_3 */
#ifdef CONFIG_UART_7
	{ STM32_PIN_PE7, STM32MP1X_PINMUX_FUNC_PE7_UART7_RX },
	{ STM32_PIN_PE8, STM32MP1X_PINMUX_FUNC_PE8_UART7_TX },
#endif  /* CONFIG_UART_7 */

};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
