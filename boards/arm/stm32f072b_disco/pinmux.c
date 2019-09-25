/*
 * Copyright (c) 2017 Clage GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>
#include "pinmux/pinmux.h"

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for STM32F072B-DISCO board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_1
	{STM32_PIN_PB6, STM32F0_PINMUX_FUNC_PB6_USART1_TX},
	{STM32_PIN_PB7, STM32F0_PINMUX_FUNC_PB7_USART1_RX},
#endif	/* CONFIG_UART_1 */
#ifdef CONFIG_I2C_1
	{STM32_PIN_PB8, STM32F0_PINMUX_FUNC_PB8_I2C1_SCL},
	{STM32_PIN_PB9, STM32F0_PINMUX_FUNC_PB9_I2C1_SDA},
#endif /* CONFIG_I2C_1 */
#ifdef CONFIG_I2C_2
	{STM32_PIN_PB10, STM32F0_PINMUX_FUNC_PB10_I2C2_SCL},
	{STM32_PIN_PB11, STM32F0_PINMUX_FUNC_PB11_I2C2_SDA},
#endif /* CONFIG_I2C_2 */
#ifdef CONFIG_SPI_1
	{STM32_PIN_PB3, STM32F0_PINMUX_FUNC_PB3_SPI1_SCK},
	{STM32_PIN_PB4, STM32F0_PINMUX_FUNC_PB4_SPI1_MISO},
	{STM32_PIN_PB5, STM32F0_PINMUX_FUNC_PB5_SPI1_MOSI},
#endif /* CONFIG_SPI_1 */
#ifdef CONFIG_CAN_1
	{STM32_PIN_PB8, STM32F0_PINMUX_FUNC_PB8_CAN_RX},
	{STM32_PIN_PB9, STM32F0_PINMUX_FUNC_PB9_CAN_TX},
#endif /* CONFIG_CAN_1 */

};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
