/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(const struct device *dev)
{
	const struct device *muxa =
		device_get_binding(DT_LABEL(DT_NODELABEL(pinmux_a)));
	const struct device *muxb =
		device_get_binding(DT_LABEL(DT_NODELABEL(pinmux_b)));

	ARG_UNUSED(dev);
	/* Prevent an unused variable error if all peripherals are disabled */
	(void)muxa;
	(void)muxb;

#if ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
	/* SERCOM4 on RX=PB9/pad 1, TX=PB8/pad 0 */
	pinmux_pin_set(muxb, 9, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 8, PINMUX_FUNC_D);
#endif

#if ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif

#if ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
	/* SPI SERCOM0 on MISO=PA5/pad 1, MOSI=PA6/pad 2, SCK=PA7/pad 3 */
	pinmux_pin_set(muxa, 5, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 6, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 7, PINMUX_FUNC_D);
#endif

#if ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif

#if defined(CONFIG_USB_DC_SAM0)
	/* USB DP on PA25, USB DM on PA24 */
	pinmux_pin_set(muxa, 25, PINMUX_FUNC_G);
	pinmux_pin_set(muxa, 24, PINMUX_FUNC_G);
#endif

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
