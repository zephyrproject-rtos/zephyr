/*
 * Copyright (c) 2019 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_2
	{STM32_PIN_PA2, STM32L1X_PINMUX_FUNC_PA2_USART2_TX},
	{STM32_PIN_PA3, STM32L1X_PINMUX_FUNC_PA3_USART2_RX},
#endif	/* CONFIG_UART_2 */
#ifdef CONFIG_I2C_1
	{STM32_PIN_PB8, STM32L1X_PINMUX_FUNC_PB8_I2C1_SCL},
	{STM32_PIN_PB9, STM32L1X_PINMUX_FUNC_PB9_I2C1_SDA},
#endif /* CONFIG_I2C_1 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
