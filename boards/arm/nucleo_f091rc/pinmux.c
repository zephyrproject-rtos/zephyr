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

/* pin assignments for NUCLEO-F091RC board */
static const struct pin_config pinconf[] = {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay) && CONFIG_I2C
	{STM32_PIN_PB8, STM32F0_PINMUX_FUNC_PB8_I2C1_SCL},
	{STM32_PIN_PB9, STM32F0_PINMUX_FUNC_PB9_I2C1_SDA},
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay) && CONFIG_I2C
	{STM32_PIN_PA11, STM32F0_PINMUX_FUNC_PA11_I2C2_SCL},
	{STM32_PIN_PA12, STM32F0_PINMUX_FUNC_PA12_I2C2_SDA},
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay) && CONFIG_SPI
	{STM32_PIN_PA5, STM32F0_PINMUX_FUNC_PA5_SPI1_SCK},
	{STM32_PIN_PA6, STM32F0_PINMUX_FUNC_PA6_SPI1_MISO},
	{STM32_PIN_PA7, STM32F0_PINMUX_FUNC_PA7_SPI1_MOSI},
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi2), okay) && CONFIG_SPI
	{STM32_PIN_PB13, STM32F0_PINMUX_FUNC_PB13_SPI2_SCK},
	{STM32_PIN_PB14, STM32F0_PINMUX_FUNC_PB14_SPI2_MISO},
	{STM32_PIN_PB15, STM32F0_PINMUX_FUNC_PB15_SPI2_MOSI},
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc1), okay) && CONFIG_ADC
	{STM32_PIN_PA0, STM32F0_PINMUX_FUNC_PA0_ADC_IN0},
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
