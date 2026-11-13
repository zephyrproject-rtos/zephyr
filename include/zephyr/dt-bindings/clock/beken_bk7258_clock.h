/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BEKEN_BK7258_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BEKEN_BK7258_CLOCK_H_

/*
 * Clock identifiers for the beken,bk7258-cgu clock controller.
 *
 * A gated clock is numbered by its gate bit in the SYS block's two device
 * clock enable registers, as listed in the Beken Armino SDK,
 * middleware/soc/bk7258/soc/sys_reg.h. Names follow that register map, which
 * counts peripherals from 0 like the devicetree nodes do; the SDK's
 * dev_clk_pwr_id_t counts some from 1 and calls gate 2 UART1.
 *
 * The SDK names gates 28 and 40 both "jpeg". Its encoder enables 28, and the
 * chip has one encoder and one decoder, so 40 is the decoder's.
 */

/* SYS word 0x0c, SYS_CPU_DEVICE_CLK_ENABLE_*_CKEN_POS: bit = id */
#define BK7258_CLK_I2C0     0
#define BK7258_CLK_SPI0     1
#define BK7258_CLK_UART0    2
#define BK7258_CLK_PWM0     3
#define BK7258_CLK_TIM0     4
#define BK7258_CLK_SADC     5
#define BK7258_CLK_IRDA     6
#define BK7258_CLK_EFUSE    7
#define BK7258_CLK_I2C1     8
#define BK7258_CLK_SPI1     9
#define BK7258_CLK_UART1    10
#define BK7258_CLK_UART2    11
#define BK7258_CLK_PWM1     12
#define BK7258_CLK_TIM1     13
#define BK7258_CLK_TIM2     14
#define BK7258_CLK_OTP      15
#define BK7258_CLK_I2S0     16
#define BK7258_CLK_USB      17
#define BK7258_CLK_CAN      18
#define BK7258_CLK_PSRAM    19
#define BK7258_CLK_QSPI0    20
#define BK7258_CLK_QSPI1    21
#define BK7258_CLK_SDIO     22
#define BK7258_CLK_AUXS     23
#define BK7258_CLK_BTDM     24
#define BK7258_CLK_XVR      25
#define BK7258_CLK_MAC      26
#define BK7258_CLK_PHY      27
#define BK7258_CLK_JPEG_ENC 28
#define BK7258_CLK_DISP     29
#define BK7258_CLK_AUD      30
#define BK7258_CLK_WDT      31

/* SYS word 0x0d, SYS_RESERVER_REG0XD_*_CKEN_POS: bit = id - 32 */
#define BK7258_CLK_H264     32
#define BK7258_CLK_I2S1     33
#define BK7258_CLK_I2S2     34
#define BK7258_CLK_YUV      35
#define BK7258_CLK_SLCD     36
#define BK7258_CLK_LIN      37
#define BK7258_CLK_SCR      38
#define BK7258_CLK_ENET     39
#define BK7258_CLK_JPEG_DEC 40
#define BK7258_CLK_CIS_AUXS 41

/* Clocks without a gate: their rate can be read, but they cannot be switched */
#define BK7258_CLK_XTAL 64 /* the crystal */
#define BK7258_CLK_CORE 65 /* the core clock, shared by all CPUs */
#define BK7258_CLK_CPU0 66 /* CPU0: the core clock, or half of it */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_BEKEN_BK7258_CLOCK_H_ */
