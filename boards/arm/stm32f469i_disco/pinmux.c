/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for STM32F469I-DISCO board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_3
	{STM32_PIN_PB10, STM32F4_PINMUX_FUNC_PB10_USART3_TX},
	{STM32_PIN_PB11, STM32F4_PINMUX_FUNC_PB11_USART3_RX},
#endif	/* CONFIG_UART_3 */
#ifdef CONFIG_UART_6
	{STM32_PIN_PG14, STM32F4_PINMUX_FUNC_PG14_USART6_TX},
	{STM32_PIN_PG9, STM32F4_PINMUX_FUNC_PG9_USART6_RX},
#endif	/* CONFIG_UART_6 */
#ifdef CONFIG_I2C_1
	{STM32_PIN_PB8, STM32F4_PINMUX_FUNC_PB8_I2C1_SCL},
	{STM32_PIN_PB9, STM32F4_PINMUX_FUNC_PB9_I2C1_SDA},
#endif /* CONFIG_I2C_1 */
#ifdef CONFIG_SPI_2
	{STM32_PIN_PD3, STM32F4_PINMUX_FUNC_PD3_SPI2_SCK},
	{STM32_PIN_PB14, STM32F4_PINMUX_FUNC_PB14_SPI2_MISO},
	{STM32_PIN_PB15, STM32F4_PINMUX_FUNC_PB15_SPI2_MOSI},
#endif /* CONFIG_SPI_2 */
#ifdef CONFIG_USB_DC_STM32
	{STM32_PIN_PA11, STM32F4_PINMUX_FUNC_PA11_OTG_FS_DM},
	{STM32_PIN_PA12, STM32F4_PINMUX_FUNC_PA12_OTG_FS_DP},
#endif  /* CONFIG_USB_DC_STM32 */

};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
		CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
