/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignment for black_f407zg_pro board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_USB_DC_STM32
	{STM32_PIN_PA11, STM32F4_PINMUX_FUNC_PA11_OTG_FS_DM},
	{STM32_PIN_PA12, STM32F4_PINMUX_FUNC_PA12_OTG_FS_DP},
#endif	/* CONFIG_USB_DC_STM32 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay) && CONFIG_SPI
	{STM32_PIN_PB3, STM32F4_PINMUX_FUNC_PB3_SPI1_SCK},
	{STM32_PIN_PB4, STM32F4_PINMUX_FUNC_PB4_SPI1_MISO},
	{STM32_PIN_PB5, STM32F4_PINMUX_FUNC_PB5_SPI1_MOSI},
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi2), okay) && CONFIG_SPI
	{STM32_PIN_PB10, STM32F4_PINMUX_FUNC_PB10_SPI2_SCK},
	{STM32_PIN_PC2, STM32F4_PINMUX_FUNC_PC2_SPI2_MISO},
	{STM32_PIN_PC3, STM32F4_PINMUX_FUNC_PC3_SPI2_MOSI},
#endif
};


static int pinmux_black_f407zg_init(const struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_black_f407zg_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
