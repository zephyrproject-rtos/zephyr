/*
 * Copyright (c) 2019 Benjamin Valentin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(const struct device *dev)
{
	const struct device *muxa = DEVICE_DT_GET(DT_NODELABEL(pinmux_a));
	const struct device *muxb = DEVICE_DT_GET(DT_NODELABEL(pinmux_b));
	const struct device *muxc = DEVICE_DT_GET(DT_NODELABEL(pinmux_c));
	const struct device *muxd = DEVICE_DT_GET(DT_NODELABEL(pinmux_d));

	__ASSERT_NO_MSG(device_is_ready(muxa));
	__ASSERT_NO_MSG(device_is_ready(muxb));
	__ASSERT_NO_MSG(device_is_ready(muxc));
	__ASSERT_NO_MSG(device_is_ready(muxd));

	ARG_UNUSED(dev);
	ARG_UNUSED(muxa);
	ARG_UNUSED(muxb);
	ARG_UNUSED(muxc);
	ARG_UNUSED(muxd);

#if (ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_uart) && CONFIG_UART_SAM0)
	/* NOTE: SERCOM1 UART is used by the UART driver tests */
	/* SERCOM1 ON RX=PC22, TX=PC23 */
	pinmux_pin_set(muxc, 22, PINMUX_FUNC_C);
	pinmux_pin_set(muxc, 23, PINMUX_FUNC_C);
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_uart) && CONFIG_UART_SAM0)
	/* SERCOM2 ON RX=PB24, TX=PB25 */
	pinmux_pin_set(muxb, 24, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 25, PINMUX_FUNC_D);
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
#if (ATMEL_SAM0_DT_SERCOM_CHECK(6, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(7, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
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
#if (ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_spi) && CONFIG_SPI_SAM0)
	/* SERCOM4 ON MOSI=PB27, MISO=PB29, SCK=PB26 */
	pinmux_pin_set(muxb, 26, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 27, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 29, PINMUX_FUNC_D);
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(6, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(7, atmel_sam0_spi) && CONFIG_SPI_SAM0)
#warning Pin mapping may not be configured
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
#warning Pin mapping may not be configured
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
#if (ATMEL_SAM0_DT_SERCOM_CHECK(6, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(7, atmel_sam0_i2c) && CONFIG_I2C_SAM0)
	pinmux_pin_set(muxd, 8, PINMUX_FUNC_C);
	pinmux_pin_set(muxd, 9, PINMUX_FUNC_C);
#endif

#if (ATMEL_SAM0_DT_TCC_CHECK(0, atmel_sam0_tcc_pwm) && CONFIG_PWM_SAM0_TCC)
	/* TCC0 on WO2=PC18 */
	pinmux_pin_set(muxc, 18, PINMUX_FUNC_F);
#endif

#ifdef CONFIG_USB_DC_SAM0
	/* USB DP on PA25, USB DM on PA24 */
	pinmux_pin_set(muxa, 25, PINMUX_FUNC_H);
	pinmux_pin_set(muxa, 24, PINMUX_FUNC_H);
#endif

#if (DT_NODE_HAS_STATUS(DT_NODELABEL(gmac), okay) && CONFIG_ETH_SAM_GMAC)
	pinmux_pin_set(muxa, 14, PINMUX_FUNC_L);	/* PA14 = GTXCK */
	pinmux_pin_set(muxa, 17, PINMUX_FUNC_L);	/* PA17 = GTXEN */
	pinmux_pin_set(muxa, 18, PINMUX_FUNC_L);	/* PA18 = GTX0 */
	pinmux_pin_set(muxa, 19, PINMUX_FUNC_L);	/* PA19 = GTX1 */
	pinmux_pin_set(muxc, 20, PINMUX_FUNC_L);	/* PC20 = GRXDV */
	pinmux_pin_set(muxa, 13, PINMUX_FUNC_L);	/* PA13 = GRX0 */
	pinmux_pin_set(muxa, 12, PINMUX_FUNC_L);	/* PA12 = GRX1 */
	pinmux_pin_set(muxa, 15, PINMUX_FUNC_L);	/* PA15 = GRXER */
	pinmux_pin_set(muxc, 11, PINMUX_FUNC_L);	/* PC11 = GMDC */
	pinmux_pin_set(muxc, 12, PINMUX_FUNC_L);	/* PC12 = GMDIO */
#endif

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
