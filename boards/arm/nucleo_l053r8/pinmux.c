/*
 * Copyright (c) 2018 Anthony Kreft <anthony.kreft@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for NUCLEO-L053R8 board */
static const struct pin_config pinconf[] = {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart1), okay) && CONFIG_SERIAL
	{STM32_PIN_PB6, STM32L0_PINMUX_FUNC_PB6_USART1_TX},
	{STM32_PIN_PB7, STM32L0_PINMUX_FUNC_PB7_USART1_RX},
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart2), okay) && CONFIG_SERIAL
	{STM32_PIN_PA2, STM32L0_PINMUX_FUNC_PA2_USART2_TX},
	{STM32_PIN_PA3, STM32L0_PINMUX_FUNC_PA3_USART2_RX},
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay) && CONFIG_I2C
	{STM32_PIN_PB8, STM32L0_PINMUX_FUNC_PB8_I2C1_SCL},
	{STM32_PIN_PB9, STM32L0_PINMUX_FUNC_PB9_I2C1_SDA},
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay) && CONFIG_SPI
	{STM32_PIN_PA5, STM32L0_PINMUX_FUNC_PA5_SPI1_SCK},
	{STM32_PIN_PA6, STM32L0_PINMUX_FUNC_PA6_SPI1_MISO},
	{STM32_PIN_PA7, STM32L0_PINMUX_FUNC_PA7_SPI1_MOSI},
#endif
};

static int pinmux_stm32_init(const struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
