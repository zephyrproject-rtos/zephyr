/*
 * Copyright (c) 2019 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for STEVAL-FCU001V1 */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_1
	{STM32_PIN_PA9, STM32F4_PINMUX_FUNC_PA9_USART1_TX},
	{STM32_PIN_PA10, STM32F4_PINMUX_FUNC_PA10_USART1_RX},
#endif	/* CONFIG_UART_1 */
#ifdef CONFIG_PWM_STM32_2
	{STM32_PIN_PA0, STM32F4_PINMUX_FUNC_PA0_PWM2_CH1},
#endif	/* CONFIG_PWM_STM32_2 */
#ifdef CONFIG_I2C_2
	{STM32_PIN_PB3, STM32F4_PINMUX_FUNC_PB3_I2C2_SDA},
	{STM32_PIN_PB10, STM32F4_PINMUX_FUNC_PB10_I2C2_SCL},
#endif	/* CONFIG_I2C_2	*/
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
		CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
