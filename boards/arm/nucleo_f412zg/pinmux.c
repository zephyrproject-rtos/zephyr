/*
 * Copyright (c) 2017 Florian Vaussard, HEIG-VD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for NUCLEO-F412ZG board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_3
	{STM32_PIN_PD8, STM32F4_PINMUX_FUNC_PD8_USART3_TX},
	{STM32_PIN_PD9, STM32F4_PINMUX_FUNC_PD9_USART3_RX},
#endif /* #ifdef CONFIG_UART_3 */
#ifdef CONFIG_UART_6
	{STM32_PIN_PG14, STM32F4_PINMUX_FUNC_PG14_USART6_TX},
	{STM32_PIN_PG9, STM32F4_PINMUX_FUNC_PG9_USART6_RX},
#endif /* CONFIG_UART_6 */
#ifdef CONFIG_PWM_STM32_2
	{STM32_PIN_PA0, STM32F4_PINMUX_FUNC_PA0_PWM2_CH1},
#endif /* CONFIG_PWM_STM32_2 */
#ifdef CONFIG_USB_DC_STM32
	{STM32_PIN_PA11, STM32F4_PINMUX_FUNC_PA11_OTG_FS_DM},
	{STM32_PIN_PA12, STM32F4_PINMUX_FUNC_PA12_OTG_FS_DP},
#endif  /* CONFIG_USB_DC_STM32 */
#ifdef CONFIG_I2C_1
	{STM32_PIN_PB8, STM32F4_PINMUX_FUNC_PB8_I2C1_SCL},
	{STM32_PIN_PB9, STM32F4_PINMUX_FUNC_PB9_I2C1_SDA},
#endif
#ifdef CONFIG_SPI_1
#ifdef CONFIG_SPI_STM32_USE_HW_SS
	{STM32_PIN_PA4, STM32F4_PINMUX_FUNC_PA4_SPI1_NSS},
#endif /* CONFIG_SPI_STM32_USE_HW_SS */
	{STM32_PIN_PA5, STM32F4_PINMUX_FUNC_PA5_SPI1_SCK},
	{STM32_PIN_PA6, STM32F4_PINMUX_FUNC_PA6_SPI1_MISO},
	{STM32_PIN_PA7, STM32F4_PINMUX_FUNC_PA7_SPI1_MOSI},
#endif	/* CONFIG_SPI_1 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
		CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
