/*
 * Copyright (c) 2024 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTS5817_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTS5817_CLOCK_H_

#define RTS_FP_CLK_SYS_PLL   0
#define RTS_FP_CLK_BUS       1
#define RTS_FP_CLK_SPI_CACHE 2
#define RTS_FP_CLK_SPI_SSOR  3
#define RTS_FP_CLK_SPI_SSI_M 4
#define RTS_FP_CLK_SPI_SSI_S 5
#define RTS_FP_CLK_SHA       6
#define RTS_FP_CLK_AES       7
#define RTS_FP_CLK_PKE       8
#define RTS_FP_CLK_I2C0      9
#define RTS_FP_CLK_I2C1      10
#define RTS_FP_CLK_TRNG      11
#define RTS_FP_CLK_I2C_S     12
#define RTS_FP_CLK_UART0     13
#define RTS_FP_CLK_UART1     14
#define RTS_FP_CLK_SIE       15
#define RTS_FP_CLK_PUF       16
#define RTS_FP_CLK_GE        17

#define RLX_CLK_NUM_SIZE (RTS_FP_CLK_GE + 1)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RTS5817_CLOCK_H_ */
