/*
 * Copyright (c) 2018 Bryan O'Donoghue
 * Copyright (c) 2019 Benjamin Valentin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>

static int board_pinmux_init(struct device *dev)
{
	struct device *muxa = device_get_binding(DT_ATMEL_SAM0_PINMUX_PINMUX_A_LABEL);
	struct device *muxb = device_get_binding(DT_ATMEL_SAM0_PINMUX_PINMUX_B_LABEL);
	struct device *muxc = device_get_binding(DT_ATMEL_SAM0_PINMUX_PINMUX_C_LABEL);

	ARG_UNUSED(dev);

#if DT_ATMEL_SAM0_UART_SERCOM_0_BASE_ADDRESS
	/* SERCOM0 on RX=PA5, TX=PA4 */
	pinmux_pin_set(muxa, 4, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 5, PINMUX_FUNC_D);
#endif
#if DT_ATMEL_SAM0_UART_SERCOM_1_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_UART_SERCOM_2_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_UART_SERCOM_3_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_UART_SERCOM_4_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_UART_SERCOM_5_BASE_ADDRESS
	/* SERCOM5 on RX=PA23, TX=PA22 */
	pinmux_pin_set(muxa, 22, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 23, PINMUX_FUNC_D);
#endif

#if DT_ATMEL_SAM0_SPI_SERCOM_0_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_SPI_SERCOM_1_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_SPI_SERCOM_2_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_SPI_SERCOM_3_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_SPI_SERCOM_4_BASE_ADDRESS
	/**
	 * SPI SERCOM4 on
	 *  MISO = PC19/pad 0,
	 *  CS   = PB31/pad 1,
	 *  MOSI = PB30/pad 2,
	 *  SCK  = PC18/pad 3
	 */
	pinmux_pin_set(muxc, 19, PINMUX_FUNC_F);
	pinmux_pin_set(muxb, 31, PINMUX_FUNC_F);
	pinmux_pin_set(muxb, 30, PINMUX_FUNC_F);
	pinmux_pin_set(muxc, 18, PINMUX_FUNC_F);
#endif
#if DT_ATMEL_SAM0_SPI_SERCOM_5_BASE_ADDRESS
	pinmux_pin_set(muxb,  2, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 22, PINMUX_FUNC_D);
	pinmux_pin_set(muxb, 23, PINMUX_FUNC_D);
#endif

#if DT_ATMEL_SAM0_I2C_SERCOM_0_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_I2C_SERCOM_1_BASE_ADDRESS
	/* SERCOM1 on SDA=PA16, SCL=PA17 */
	pinmux_pin_set(muxa, 16, PINMUX_FUNC_C);
	pinmux_pin_set(muxa, 17, PINMUX_FUNC_C);
#endif
#if DT_ATMEL_SAM0_I2C_SERCOM_2_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_I2C_SERCOM_3_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_I2C_SERCOM_4_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif
#if DT_ATMEL_SAM0_I2C_SERCOM_5_BASE_ADDRESS
#warning Pin mapping may not be configured
#endif

#ifdef CONFIG_USB_DC_SAM0
	/* USB DP on PA25, USB DM on PA24 */
	pinmux_pin_set(muxa, 25, PINMUX_FUNC_G);
	pinmux_pin_set(muxa, 24, PINMUX_FUNC_G);
#endif
	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
