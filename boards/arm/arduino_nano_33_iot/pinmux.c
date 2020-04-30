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

#if defined(CONFIG_UART_SAM0)
#if ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_uart)
	/* SERCOM3 on RX=PA23/pad 1, TX=PA22/pad 0 */
	pinmux_pin_set(muxa, 22, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 23, PINMUX_FUNC_C);
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_uart)
	/* SERCOM5 on RX=PB23/pad 3, TX=PB22/pad 2 */
	pinmux_pin_set(muxb, 23, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 22, PINMUX_FUNC_D);
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_uart)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_uart)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_uart)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_uart)
#warning Pin mapping may not be configured
#endif
#endif

#if defined(CONFIG_SPI_SAM0)
#if ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_spi)
	/* SPI SERCOM1 on MISO=PA19/pad 3, MOSI=PA16/pad 0, SCK=PA17/pad 1 */
	pinmux_pin_set(muxa, 16, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 17, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 19, PINMUX_FUNC_D);
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_spi)
	/* SPI SERCOM2 on MISO=PA13/pad 1, MOSI=PA12/pad 0, SCK=PA15/pad 3 */
	pinmux_pin_set(muxa, 12, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 13, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 15, PINMUX_FUNC_C);
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_spi)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_spi)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_spi)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_spi)
#warning Pin mapping may not be configured
#endif
#endif /* CONFIG_SPI_SAM0 */

#if defined(CONFIG_I2C_SAM0)
#if ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_i2c)
	/* SDA on PB8/pad 0 SCL on PB9/pad 1 */
	pinmux_pin_set(muxb, 8, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 9, PINMUX_FUNC_D);
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_i2c)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_i2c)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_i2c)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_i2c)
#warning Pin mapping may not be configured
#endif
#if ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_i2c)
#warning Pin mapping may not be configured
#endif
#endif

#if defined(CONFIG_PWM_SAM0_TCC)
#if ATMEL_SAM0_DT_TCC_CHECK(2, atmel_sam0_tcc_pwm)
	/* LED0 on PA17/TCC2/WO[1] */
	pinmux_pin_set(muxa, 17, PINMUX_FUNC_E);
#endif
#endif

	if (IS_ENABLED(CONFIG_USB_DC_SAM0)) {
		/* USB DP on PA25, USB DM on PA24 */
		pinmux_pin_set(muxa, 25, PINMUX_FUNC_G);
		pinmux_pin_set(muxa, 24, PINMUX_FUNC_G);
	}

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
