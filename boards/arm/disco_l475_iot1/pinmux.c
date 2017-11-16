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

/* Include pinmux configuration generated file */
#include <st_stm32_pinmux_init.h>

/* pin assignments for Disco L475 IOT1 board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_SPI_1
	{STM32_PIN_PA5, STM32L4X_PINMUX_FUNC_PA5_SPI1_SCK},
	{STM32_PIN_PA6, STM32L4X_PINMUX_FUNC_PA6_SPI1_MISO},
	{STM32_PIN_PA7, STM32L4X_PINMUX_FUNC_PA7_SPI1_MOSI},
#endif /* CONFIG_SPI_1 */
#ifdef CONFIG_SPI_3
	/* SPI3 is used for BT/WIFI, Sub GHZ communication */
	{STM32_PIN_PC10, STM32L4X_PINMUX_FUNC_PC10_SPI3_SCK},
	{STM32_PIN_PC11, STM32L4X_PINMUX_FUNC_PC11_SPI3_MISO},
	{STM32_PIN_PC12, STM32L4X_PINMUX_FUNC_PC12_SPI3_MOSI},
#endif /* CONFIG_SPI_3 */
#ifdef CONFIG_PWM_STM32_2
	{STM32_PIN_PA15, STM32L4X_PINMUX_FUNC_PA15_PWM2_CH1},
#endif /* CONFIG_PWM_STM32_2 */
#ifdef CONFIG_USB_STM32
	{STM32_PIN_PA9, STM32L4X_PINMUX_FUNC_PA9_OTG_5V_VBUS},
	{STM32_PIN_PA10, STM32L4X_PINMUX_FUNC_PA10_OTG_FS_ID},
	{STM32_PIN_PA11, STM32L4X_PINMUX_FUNC_PA11_OTG_FS_DM},
	{STM32_PIN_PA12, STM32L4X_PINMUX_FUNC_PA12_OTG_FS_DP},
#endif /* CONFIG_USB_STM32 */

};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	/* Parse pinconf array provided above */
	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	/* Parse st_stm32_pinmux_pinconf array provided */
	/* in dts based generated file st_stm32_pinmux_init.h */
	stm32_setup_pins(st_stm32_pinmux_pinconf,
			 ARRAY_SIZE(st_stm32_pinmux_pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
