/*
 * Copyright (c) 2018 Sean Nyekjaer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(const struct device *dev)
{
	const struct device *muxa = device_get_binding(DT_LABEL(DT_NODELABEL(pinmux_a)));
	const struct device *muxb = device_get_binding(DT_LABEL(DT_NODELABEL(pinmux_b)));

	ARG_UNUSED(dev);

#if (ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_uart) && CONFIG_UART_SAM0)
	/* SERCOM3 on RX=PA25, TX=PA24 */
	pinmux_pin_set(muxa, 24, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 25, PINMUX_FUNC_C);
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_uart) && CONFIG_UART_SAM0)
	pinmux_pin_set(muxb,  8, PINMUX_FUNC_D);
	pinmux_pin_set(muxb,  9, PINMUX_FUNC_D);
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_spi) && CONFIG_SPI_SAM0)
	/* SPI SERCOM0 on MISO=PA04, MOSI=PA06, SCK=PA07 */
	pinmux_pin_set(muxa,  4, PINMUX_FUNC_D);
	pinmux_pin_set(muxa,  6, PINMUX_FUNC_D);
	pinmux_pin_set(muxa,  7, PINMUX_FUNC_D);
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
#if (ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
	/* SERCOM2 on SDA=PA08, SCL=PA09 */
	pinmux_pin_set(muxa, 8, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 9, PINMUX_FUNC_D);
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
#warning Pin mapping may not be configured
#endif

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
