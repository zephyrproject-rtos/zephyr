/*
 * Copyright (c) 2018 Sean Nyekjaer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <pinmux.h>

static int board_pinmux_init(struct device *dev)
{
	struct device *muxa = device_get_binding(CONFIG_PINMUX_SAM0_A_LABEL);
	struct device *muxb = device_get_binding(CONFIG_PINMUX_SAM0_B_LABEL);

	ARG_UNUSED(dev);

#if CONFIG_UART_SAM0_SERCOM0_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if CONFIG_UART_SAM0_SERCOM1_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if CONFIG_UART_SAM0_SERCOM2_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if CONFIG_UART_SAM0_SERCOM3_BASE_ADDRESS
	/* SERCOM3 on RX=PA25, TX=PA24 */
	pinmux_pin_set(muxa, 24, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 25, PINMUX_FUNC_C);
#endif
#if CONFIG_UART_SAM0_SERCOM4_BASE_ADDRESS
	pinmux_pin_set(muxb,  8, PINMUX_FUNC_D);
	pinmux_pin_set(muxb,  9, PINMUX_FUNC_D);
#endif
#if CONFIG_UART_SAM0_SERCOM5_BASE_ADDRESS
#error Pin mapping is not configured
#endif

#if CONFIG_SPI_SAM0_SERCOM0_BASE_ADDRESS
	/* SPI SERCOM0 on MISO=PA04, MOSI=PA06, SCK=PA07 */
	pinmux_pin_set(muxa,  4, PINMUX_FUNC_D);
	pinmux_pin_set(muxa,  6, PINMUX_FUNC_D);
	pinmux_pin_set(muxa,  7, PINMUX_FUNC_D);
#endif
#if CONFIG_SPI_SAM0_SERCOM1_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if CONFIG_SPI_SAM0_SERCOM2_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if CONFIG_SPI_SAM0_SERCOM3_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if CONFIG_SPI_SAM0_SERCOM4_BASE_ADDRESS
#error Pin mapping is not configured
#endif
#if CONFIG_SPI_SAM0_SERCOM5_BASE_ADDRESS
#error Pin mapping is not configured
#endif

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
