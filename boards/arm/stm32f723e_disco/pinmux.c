/*
 * Copyright (c) 2018 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for STM32F723E-DISCO board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_STM32_PORT_2
	{STM32_PIN_PA2, STM32F7_PINMUX_FUNC_PA2_USART2_TX},
	{STM32_PIN_PA3, STM32F7_PINMUX_FUNC_PA3_USART2_RX},
#endif	/* CONFIG_UART_STM32_PORT_2 */
#ifdef CONFIG_UART_STM32_PORT_6
	{STM32_PIN_PC6, STM32F7_PINMUX_FUNC_PC6_USART6_TX},
	{STM32_PIN_PC7, STM32F7_PINMUX_FUNC_PC7_USART6_RX},
#endif	/* CONFIG_UART_STM32_PORT_6 */
#ifdef CONFIG_I2C_1
	{STM32_PIN_PB8, STM32F7_PINMUX_FUNC_PB8_I2C1_SCL},
	{STM32_PIN_PB9, STM32F7_PINMUX_FUNC_PB9_I2C1_SDA},
#endif /* CONFIG_I2C_1 */
#ifdef CONFIG_I2C_2
	{STM32_PIN_PH4, STM32F7_PINMUX_FUNC_PH4_I2C2_SCL},
	{STM32_PIN_PH5, STM32F7_PINMUX_FUNC_PH5_I2C2_SDA},
#endif /* CONFIG_I2C_2 */
#ifdef CONFIG_I2C_3
	{STM32_PIN_PA8, STM32F7_PINMUX_FUNC_PA8_I2C3_SCL},
	{STM32_PIN_PH8, STM32F7_PINMUX_FUNC_PH8_I2C3_SDA},
#endif /* CONFIG_I2C_3 */
#ifdef CONFIG_USB_DC_STM32
	{STM32_PIN_PA11, STM32F7_PINMUX_FUNC_PA11_OTG_FS_DM},
	{STM32_PIN_PA12, STM32F7_PINMUX_FUNC_PA12_OTG_FS_DP},
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
