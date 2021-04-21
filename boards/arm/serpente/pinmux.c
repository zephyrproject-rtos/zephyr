/*
 * Copyright (c) 2020 Alexander Falb <fal3xx@gmail.com>
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

	/* sercom 3 is always spi - it is the onboard flash */
	/* SPI SERCOM3 on MISO=PA18, MOSI=PA16, SCK=PA17, CS=PA15*/
	pinmux_pin_set(muxa, 18, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 16, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 17, PINMUX_FUNC_D);


#if ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_spi) && defined(CONFIG_SPI_SAM0)
	/* SPI SERCOM0 on MISO=PA6, MOSI=PA4, SCK=PA5 */
	pinmux_pin_set(muxa, 6, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 4, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 5, PINMUX_FUNC_D);
#endif

#if ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
	/* SERCOM0 on RX=PA5, TX=PA4 */
	pinmux_pin_set(muxa, 5, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 4, PINMUX_FUNC_C);
#endif

#if ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_i2c) && defined(CONFIG_I2C_SAM0)
	/* SERCOM2 on SDA=PA08, SCL=PA09 */
	pinmux_pin_set(muxa, 4, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 5, PINMUX_FUNC_D);
#endif

#if ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_uart) && defined(CONFIG_UART_SAM0)
	/* SERCOM0 on RX=PA9, TX=PA8 */
	pinmux_pin_set(muxa, 9, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 8, PINMUX_FUNC_C);
#endif

#ifdef CONFIG_USB_DC_SAM0
	/* USB DP on PA25, USB DM on PA24 */
	pinmux_pin_set(muxa, 25, PINMUX_FUNC_G);
	pinmux_pin_set(muxa, 24, PINMUX_FUNC_G);
#endif

#if (ATMEL_SAM0_DT_TCC_CHECK(0, atmel_sam0_tcc_pwm) && CONFIG_PWM_SAM0_TCC)
	/* TCC0 on WO4=PA22 (red),  WO3=PA19 (green),  WO5=PA23 (blue) */
	pinmux_pin_set(muxa, 22, PINMUX_FUNC_F);
	pinmux_pin_set(muxa, 19, PINMUX_FUNC_F);
	pinmux_pin_set(muxa, 23, PINMUX_FUNC_F);
#endif

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
