/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(struct device *dev)
{
	struct device *muxa = device_get_binding(DT_LABEL(DT_NODELABEL(pinmux_a)));
	struct device *muxb = device_get_binding(DT_LABEL(DT_NODELABEL(pinmux_b)));

	ARG_UNUSED(dev);

#if (ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_uart) && CONFIG_UART_SAM0)
	/* SERCOM0 on RX=PA11, TX=PA10 */
	pinmux_pin_set(muxa, 11, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 10, PINMUX_FUNC_C);
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_uart) && CONFIG_UART_SAM0)
	/* SERCOM5 on RX=PB23, TX=PB22 */
	pinmux_pin_set(muxb, 23, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 22, PINMUX_FUNC_D);
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_spi) && CONFIG_SPI_SAM0)
	/* SPI SERCOM4 on MISO=PA12/pad 0, MOSI=PB10/pad 2, SCK=PB11/pad 3 */
	pinmux_pin_set(muxa, 12, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 10, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 11, PINMUX_FUNC_D);
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif

#if ATMEL_SAM0_DT_TCC_CHECK(2, atmel_sam0_tcc_pwm) &&                          \
	defined(CONFIG_PWM_SAM0_TCC)
	/* LED0 on PA17/TCC2/WO[1] */
	pinmux_pin_set(muxa, 17, PINMUX_FUNC_E);
#endif

#ifdef CONFIG_USB_DC_SAM0
	/* USB DP on PA25, USB DM on PA24 */
	pinmux_pin_set(muxa, 25, PINMUX_FUNC_G);
	pinmux_pin_set(muxa, 24, PINMUX_FUNC_G);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(dac0), okay) && defined(CONFIG_DAC_SAM0)
	/* DAC on PA02 */
	pinmux_pin_set(muxa, 2, PINMUX_FUNC_B);
#endif
	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
