/*
 * Copyright (c) 2018 Madani Lainani.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <pinmux.h>

static int board_pinmux_init(struct device *dev)
{
#if DT_I2C_SAM0_SERCOM0_BASE_ADDRESS
	struct device *muxa = device_get_binding(DT_PINMUX_SAM0_A_LABEL);
#endif
	struct device *muxb = device_get_binding(DT_PINMUX_SAM0_B_LABEL);

	ARG_UNUSED(dev);

#if DT_I2C_SAM0_SERCOM0_BASE_ADDRESS
	/* I2C SERCOM0 on SDA=PA08/PAD[0], SCL=PA09/PAD[1] */
	pinmux_pin_set(muxa, 8, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 9, PINMUX_FUNC_C);
#endif

#if DT_SPI_SAM0_SERCOM1_BASE_ADDRESS
	/* SPI SERCOM1 on MOSI=PA16/PAD[0], SCK=PA17/PAD[1], MISO=PA19/PAD[3] */
	pinmux_pin_set(muxa, 16, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 17, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 19, PINMUX_FUNC_C);
#endif

#if DT_UART_SAM0_SERCOM5_BASE_ADDRESS
	/* SERCOM5 on TX=PB22, RX=PB23 */
	pinmux_pin_set(muxb, 22, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 23, PINMUX_FUNC_D);
#endif

#if DT_UART_SAM0_SERCOM0_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_UART_SAM0_SERCOM1_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_UART_SAM0_SERCOM2_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_UART_SAM0_SERCOM3_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_UART_SAM0_SERCOM4_BASE_ADDRESS
#error Pin mapping is not configured
#endif

#if DT_SPI_SAM0_SERCOM0_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_SPI_SAM0_SERCOM2_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_SPI_SAM0_SERCOM3_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_SPI_SAM0_SERCOM4_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_SPI_SAM0_SERCOM5_BASE_ADDRESS
#error Pin mapping is not configured
#endif

#if DT_I2C_SAM0_SERCOM1_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_I2C_SAM0_SERCOM2_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_I2C_SAM0_SERCOM3_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_I2C_SAM0_SERCOM4_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if DT_I2C_SAM0_SERCOM5_BASE_ADDRESS
#error Pin mapping is not configured
#endif

#ifdef DT_USB_DC_SAM0
	/* USB DP on PA25, USB DM on PA24 */
	pinmux_pin_set(muxa, 25, PINMUX_FUNC_G);
	pinmux_pin_set(muxa, 24, PINMUX_FUNC_G);
#endif

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
