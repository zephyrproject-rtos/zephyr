/*
 * Copyright (c) 2018 Philémon Jaermann
 * Copyright (c) 2019 Peter Nimac
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for stm32F446ZE Nucleo board */
/*
 * To enable Full USART 2 module found on the board add the required ifdef config
 * and add an entry to the nucleo_f446ze.dts file.
 *
 * WARNING! If you want full USART 2 functionality disable the I2S_2 SD pin PB15,
 * as it conflicts with USART 2 CTS signal pin! [refer to the datasheet for Nucleo-144 boards, UM1974 pg. 43 and datasheet for STM32F446ZE, RM0390]
 * Optionally you can reasign I2S_2 SD pin to pin PC1 (CN11) or, if you don't need
 * Arduino Uno v3 compatibility you can use the pin PC3 (CN8, Arduino pin A2).
 *
 */

static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_3 /* UART console over ST-Link port (USB debugging port) */
	{STM32_PIN_PD8, STM32F4_PINMUX_FUNC_PD8_USART3_TX},
	{STM32_PIN_PD9, STM32F4_PINMUX_FUNC_PD9_USART3_RX},
#endif	/* CONFIG_UART_3 */


#ifdef CONFIG_UART_6 /* On Nucleo board, port USART(_A) - CN10 */
	{STM32_PIN_PG14, STM32F4_PINMUX_FUNC_PG14_USART6_TX},
	{STM32_PIN_PG9, STM32F4_PINMUX_FUNC_PG9_USART6_RX},
#endif	/* CONFIG_UART_6 */


#ifdef CONFIG_I2C_1 /* On Nucleo board, port I2C(_A) - CN7 */
	{STM32_PIN_PB8, STM32F4_PINMUX_FUNC_PB8_I2C1_SCL},
	{STM32_PIN_PB9, STM32F4_PINMUX_FUNC_PB9_I2C1_SDA},
#endif /* CONFIG_I2C_1 */


#ifdef CONFIG_I2C_2 /* On Nucleo board, port I2C(_B) - CN9 */
	{STM32_PIN_PF2, STM32F4_PINMUX_FUNC_PF2_I2C2_SMBA},
	{STM32_PIN_PF1, STM32F4_PINMUX_FUNC_PF1_I2C2_SCL},
	{STM32_PIN_PF0, STM32F4_PINMUX_FUNC_PF0_I2C2_SDA},
#endif /* CONFIG_I2C_2 */


#ifdef CONFIG_I2S_2 /* On Nucleo board, port I2S_A - CN7 */
	{STM32_PIN_PB13, STM32F4_PINMUX_FUNC_PB13_I2S2_CK},
	{STM32_PIN_PB12, STM32F4_PINMUX_FUNC_PB12_I2S2_WS},
	{STM32_PIN_PB15, STM32F4_PINMUX_FUNC_PB15_I2S2_SD},
/*	{STM32_PIN_PD3, STM32F4_PINMUX_FUNC_PD3_I2S2_SD}, Additional data line that can be used (alternate pins for PD3 are PC1 and PC3) */
#endif /* CONFIG_I2S_2 */


#ifdef CONFIG_SPI_1
	{STM32_PIN_PD14, STM32F4_PINMUX_FUNC_PD14_SPI1_NSS},
	{STM32_PIN_PA5, STM32F4_PINMUX_FUNC_PA5_SPI1_SCK},
	{STM32_PIN_PA6, STM32F4_PINMUX_FUNC_PA6_SPI1_MISO},
	{STM32_PIN_PA7, STM32F4_PINMUX_FUNC_PA7_SPI1_MOSI},
#endif	/* CONFIG_SPI_1 */


#ifdef CONFIG_SPI_3 /* On Nucleo board, port SPI_B - CN7 */
	{STM32_PIN_PA4, STM32F4_PINMUX_FUNC_PA4_SPI3_NSS},
	{STM32_PIN_PB3, STM32F4_PINMUX_FUNC_PB3_SPI3_SCK},
	{STM32_PIN_PB4, STM32F4_PINMUX_FUNC_PB4_SPI3_MISO},
	{STM32_PIN_PB5, STM32F4_PINMUX_FUNC_PB5_SPI3_MOSI},
#endif	/* CONFIG_SPI_3 */


#ifdef CONFIG_ADC_1 /* On Nucleo board, port ADC_IN - CN10 */
	{STM32_PIN_PB1, STM32F4_PINMUX_FUNC_PB1_ADC12_IN9},
/*	{STM32_PIN_PC2, STM32F4_PINMUX_FUNC_PC2_ADC123_IN12}, */
/*	{STM32_PIN_PF4, STM32F4_PINMUX_FUNC_PF4_ADC3_IN14}, */
/* For default initialization, pins PC2 and PF4 are currently disabled */
#endif	/* CONFIG_ADC_1 */

#ifdef CONFIG_USB_DC_STM32
	{STM32_PIN_PA11, STM32F4_PINMUX_FUNC_PA11_OTG_FS_DM},
	{STM32_PIN_PA12, STM32F4_PINMUX_FUNC_PA12_OTG_FS_DP},
#endif	/* CONFIG_USB_DC_STM32 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
		CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
