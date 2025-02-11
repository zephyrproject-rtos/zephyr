/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RA_H_

#define RA_PLL_SOURCE_HOCO     0
#define RA_PLL_SOURCE_MOCO     1
#define RA_PLL_SOURCE_LOCO     2
#define RA_PLL_SOURCE_MAIN_OSC 3
#define RA_PLL_SOURCE_SUBCLOCK 4
#define RA_PLL_SOURCE_DISABLE  0xff

#define RA_CLOCK_SOURCE_HOCO     0
#define RA_CLOCK_SOURCE_MOCO     1
#define RA_CLOCK_SOURCE_LOCO     2
#define RA_CLOCK_SOURCE_MAIN_OSC 3
#define RA_CLOCK_SOURCE_SUBCLOCK 4
#define RA_CLOCK_SOURCE_PLL      5
#define RA_CLOCK_SOURCE_PLL1P    RA_CLOCK_SOURCE_PLL
#define RA_CLOCK_SOURCE_PLL2     6
#define RA_CLOCK_SOURCE_PLL2P    RA_CLOCK_SOURCE_PLL2
#define RA_CLOCK_SOURCE_PLL1Q    7
#define RA_CLOCK_SOURCE_PLL1R    8
#define RA_CLOCK_SOURCE_PLL2Q    9
#define RA_CLOCK_SOURCE_PLL2R    10
#define RA_CLOCK_SOURCE_DISABLE  0xff

#define RA_SYS_CLOCK_DIV_1   0
#define RA_SYS_CLOCK_DIV_2   1
#define RA_SYS_CLOCK_DIV_4   2
#define RA_SYS_CLOCK_DIV_8   3
#define RA_SYS_CLOCK_DIV_16  4
#define RA_SYS_CLOCK_DIV_32  5
#define RA_SYS_CLOCK_DIV_64  6
#define RA_SYS_CLOCK_DIV_128 7 /* available for CLKOUT only */
#define RA_SYS_CLOCK_DIV_3   8
#define RA_SYS_CLOCK_DIV_6   9
#define RA_SYS_CLOCK_DIV_12  10

/* PLL divider options. */
#define RA_PLL_DIV_1  0
#define RA_PLL_DIV_2  1
#define RA_PLL_DIV_3  2
#define RA_PLL_DIV_4  3
#define RA_PLL_DIV_5  4
#define RA_PLL_DIV_6  5
#define RA_PLL_DIV_8  7
#define RA_PLL_DIV_9  8
#define RA_PLL_DIV_16 15

/* USB clock divider options. */
#define RA_USB_CLOCK_DIV_1 0
#define RA_USB_CLOCK_DIV_2 1
#define RA_USB_CLOCK_DIV_3 2
#define RA_USB_CLOCK_DIV_4 3
#define RA_USB_CLOCK_DIV_5 4
#define RA_USB_CLOCK_DIV_6 5
#define RA_USB_CLOCK_DIV_8 7

/* USB60 clock divider options. */
#define RA_USB60_CLOCK_DIV_1 0
#define RA_USB60_CLOCK_DIV_2 1
#define RA_USB60_CLOCK_DIV_3 5
#define RA_USB60_CLOCK_DIV_4 2
#define RA_USB60_CLOCK_DIV_5 6
#define RA_USB60_CLOCK_DIV_6 3
#define RA_USB60_CLOCK_DIV_8 4

/* OCTA clock divider options. */
#define RA_OCTA_CLOCK_DIV_1 0
#define RA_OCTA_CLOCK_DIV_2 1
#define RA_OCTA_CLOCK_DIV_4 2
#define RA_OCTA_CLOCK_DIV_6 3
#define RA_OCTA_CLOCK_DIV_8 4

/* CANFD clock divider options. */
#define RA_CANFD_CLOCK_DIV_1 0
#define RA_CANFD_CLOCK_DIV_2 1
#define RA_CANFD_CLOCK_DIV_3 5
#define RA_CANFD_CLOCK_DIV_4 2
#define RA_CANFD_CLOCK_DIV_5 6
#define RA_CANFD_CLOCK_DIV_6 3
#define RA_CANFD_CLOCK_DIV_8 4

/* SCI clock divider options. */
#define RA_SCI_CLOCK_DIV_1 0
#define RA_SCI_CLOCK_DIV_2 1
#define RA_SCI_CLOCK_DIV_3 5
#define RA_SCI_CLOCK_DIV_4 2
#define RA_SCI_CLOCK_DIV_5 6
#define RA_SCI_CLOCK_DIV_6 3
#define RA_SCI_CLOCK_DIV_8 4

/* SPI clock divider options. */
#define RA_SPI_CLOCK_DIV_1 0
#define RA_SPI_CLOCK_DIV_2 1
#define RA_SPI_CLOCK_DIV_3 5
#define RA_SPI_CLOCK_DIV_4 2
#define RA_SPI_CLOCK_DIV_5 6
#define RA_SPI_CLOCK_DIV_6 3
#define RA_SPI_CLOCK_DIV_8 4

/* CEC clock divider options. */
#define RA_CEC_CLOCK_DIV_1 0
#define RA_CEC_CLOCK_DIV_2 1

/* I3C clock divider options. */
#define RA_I3C_CLOCK_DIV_1 0
#define RA_I3C_CLOCK_DIV_2 1
#define RA_I3C_CLOCK_DIV_3 5
#define RA_I3C_CLOCK_DIV_4 2
#define RA_I3C_CLOCK_DIV_5 6
#define RA_I3C_CLOCK_DIV_6 3
#define RA_I3C_CLOCK_DIV_8 4

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RA_H_ */
