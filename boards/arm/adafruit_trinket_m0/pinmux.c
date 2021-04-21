/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(const struct device *dev)
{
	const struct device *muxa = DEVICE_DT_GET(DT_NODELABEL(pinmux_a));

	__ASSERT_NO_MSG(device_is_ready(muxa));

	ARG_UNUSED(dev);

#if (ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_uart) && CONFIG_UART_SAM0)
	/* SERCOM0 on RX=PA7/pad 3, TX=PA6/pad 2 */
	pinmux_pin_set(muxa, 7, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 6, PINMUX_FUNC_D);
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_uart) && CONFIG_UART_SAM0)
	/* SERCOM2 on RX=PA9/pad 1, TX=PA8/pad 0 */
	pinmux_pin_set(muxa, 9, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 8, PINMUX_FUNC_D);
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_spi) && CONFIG_SPI_SAM0)
	/* SPI SERCOM0 on MISO=PA9/pad 1, MOSI=PA6/pad 2, SCK=PA7/pad 3 */
	pinmux_pin_set(muxa, 9, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 6, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 7, PINMUX_FUNC_D);
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_spi) && CONFIG_SPI_SAM0)
	/* SPI SERCOM1 on MOSI=PA0/pad 0, SCK=PA1/pad 1 */
	pinmux_pin_set(muxa, 0, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 1, PINMUX_FUNC_D);
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif

#if ATMEL_SAM0_DT_TCC_CHECK(0, atmel_sam0_tcc_pwm) && \
	defined(CONFIG_PWM_SAM0_TCC)
	/* LED0 on PA10/TCC0/WO[2] */
	pinmux_pin_set(muxa, 10, PINMUX_FUNC_F);
#endif

#ifdef CONFIG_USB_DC_SAM0
	/* USB DP on PA25, USB DM on PA24 */
	pinmux_pin_set(muxa, 25, PINMUX_FUNC_G);
	pinmux_pin_set(muxa, 24, PINMUX_FUNC_G);
#endif

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
