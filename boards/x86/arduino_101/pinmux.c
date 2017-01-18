/* pinmux_board_arduino_101.c - Arduino 101 board pinmux driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <board.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <sys_io.h>
#include "pinmux/pinmux.h"

#include "pinmux_quark_mcu.h"

/*
 * This is the full pinmap that we have available on the board for configuration
 * including the ball position and the various modes that can be set.  In the
 * _pinmux_defaults we do not spend any time setting values that are using mode
 * A as the hardware brings up all devices by default in mode A.
 */

/* pin,	ball,	mode A,		mode B,		mode C		*/
/* 0	F02,	gpio_0,		ain_0,		spi_s_cs	*/   /* IO10 */
/* 1	G04,	gpio_1,		ain_1,		spi_s_miso	*/   /* IO12 */
/* 2	H05,	gpio_2,		ain_2,		spi_s_sck	*/   /* IO13 */
/* 3	J06,	gpio_3,		ain_3,		spi_s_mosi	*/   /* IO11 */
/* 4	K06,	gpio_4,		ain_4,		NA		*/
/* 5	L06,	gpio_5,		ain_5,		NA		*/
/* 6	H04,	gpio_6,		ain_6,		NA		*/
/* 7	G03,	gpio_7,		ain_7,		NA		*/
/* 8	L05,	gpio_ss_0,	ain_8,		uart1_cts	*/
/* 9	M05,	gpio_ss_1,	ain_9,		uart1_rts	*/    /* AD5 */
/* 10	K05,	gpio_ss_2,	ain_10				*/    /* AD0 */
/* 11	G01,	gpio_ss_3,	ain_11				*/    /* AD1 */
/* 12	J04,	gpio_ss_4,	ain_12				*/    /* AD2 */
/* 13	G02,	gpio_ss_5,	ain_13				*/    /* AD3 */
/* 14	F01,	gpio_ss_6,	ain_14				*/    /* AD4 */
/* 15	J05,	gpio_ss_7,	ain_15				*/
/* 16	L04,	gpio_ss_8,	ain_16,		uart1_txd	*/    /* IO1 */
/* 17	M04,	gpio_ss_9,	ain_17,		uart1_rxd	*/    /* IO0 */
/* 18	K04,	uart0_rx,	ain_18,		NA		*/
/* 19	B02,	uart0_tx,	gpio_31,	NA		*/
/* 20	C01,	i2c0_scl,	NA,		NA		*/
/* 21	C02,	i2c0_sda,	NA,		NA		*/
/* 22	D01,	i2c1_scl,	NA,		NA		*/
/* 23	D02,	i2c1_sda,	NA,		NA		*/
/* 24	E01,	i2c0_ss_sda,	NA,		NA		*/
/* 25	E02,	i2c0_ss_scl,	NA,		NA		*/
/* 26	B03,	i2c1_ss_sda,	NA,		NA		*/
/* 27	A03,	i2c1_ss_scl,	NA,		NA		*/
/* 28	C03,	spi0_ss_miso,	NA,		NA		*/
/* 29	E03,	spi0_ss_mosi,	NA,		NA		*/
/* 30	D03,	spi0_ss_sck,	NA,		NA		*/
/* 31	D04,	spi0_ss_cs0,	NA,		NA		*/
/* 32	C04,	spi0_ss_cs1,	NA,		NA		*/
/* 33	B04,	spi0_ss_cs2,	gpio_29,	NA		*/
/* 34	A04,	spi0_ss_cs3,	gpio_30,	NA		*/
/* 35	B05,	spi1_ss_miso,	NA,		NA		*/
/* 36	C05,	spi1_ss_mosi,	NA,		NA		*/
/* 37	D05,	spi1_ss_sck,	NA,		NA		*/
/* 38	E05,	spi1_ss_cs0,	NA,		NA		*/
/* 39	E04,	spi1_ss_cs1,	NA,		NA		*/
/* 40	A06,	spi1_ss_cs2,	uart0_cts,	NA		*/
/* 41	B06,	spi1_ss_cs3,	uart0_rts,	NA		*/
/* 42	C06,	gpio_8,		spi1_m_sck,	NA		*/   /* IO13 */
/* 43	D06,	gpio_9,		spi1_m_miso,	NA		*/   /* IO12 */
/* 44	E06,	gpio_10,	spi1_m_mosi,	NA		*/   /* IO11 */
/* 45	D07,	gpio_11,	spi1_m_cs0,	NA		*/
/* 46	C07,	gpio_12,	spi1_m_cs1,	NA		*/
/* 47	B07,	gpio_13,	spi1_m_cs2,	NA		*/
/* 48	A07,	gpio_14,	spi1_m_cs3,	NA		*/
/* 49	B08,	gpio_15,	i2s_rxd,	NA		*/    /* IO5 */
/* 50	A08,	gpio_16,	i2s_rscki,	NA		*/    /* IO8 */
/* 51	B09,	gpio_17,	i2s_rws,	NA		*/    /* IO3 */
/* 52	A09,	gpio_18,	i2s_tsck,	NA		*/    /* IO2 */
/* 53	C09,	gpio_19,	i2s_twsi,	NA		*/    /* IO4 */
/* 54	D09,	gpio_20,	i2s_txd,	NA		*/    /* IO7 */
/* 55	D08,	gpio_21,	spi0_m_sck,	NA		*/
/* 56	E07,	gpio_22,	spi0_m_miso,	NA		*/
/* 57	E09,	gpio_23,	spi0_m_mosi,	NA		*/
/* 58	E08,	gpio_24,	spi0_m_cs0,	NA		*/
/* 59	A10,	gpio_25,	spi0_m_cs1,	NA		*/
/* 60	B10,	gpio_26,	spi0_m_cs2,	NA		*/
/* 61	C10,	gpio_27,	spi0_m_cs3,	NA		*/
/* 62	D10,	gpio_28,	NA,		NA		*/
/* 63	E10,	gpio_ss_10,	pwm_0,		NA		*/    /* IO3 */
/* 64	D11,	gpio_ss_11,	pwm_1,		NA		*/    /* IO5 */
/* 65	C11,	gpio_ss_12,	pwm_2,		NA		*/    /* IO6 */
/* 66	B11,	gpio_ss_13,	pwm_3,		NA		*/    /* IO9 */
/* 67	D12,	gpio_ss_14,	clkout_32khz,	NA		*/
/* 68	C12,	gpio_ss_15,	clkout_16mhz,	NA		*/

/* Note:
 * 1. I2C pins on the shield are connected to i2c0_ss_sda and i2c_0_ss_scl,
 *    which are on the sensor subsystem. They are also tied to AD4 and AD5.
 *    Therefore, to use I2C, pin 9 (ain_9) and pin 14 (ain_14) both need to be set
 *    to PINMUX_FUNC_B, so they will not interfere with I2C operations.
 *    Also, there is no internal pull-up on I2c bus, and thus external
 *    pull-up resistors are needed.
 * 2. IO3/PWM0 is connected to pin 51 and 63.
 * 3. IO5/PWM1 is connected to pin 49 and 64.
 */

/*
 * On the QUARK_SE platform there are a minimum of 69 pins that can be possibly
 * set.  This would be a total of 5 registers to store the configuration as per
 * the bit description from above
 */
#define PINMUX_MAX_REGISTERS	5

static void _pinmux_defaults(uint32_t base)
{
	uint32_t mux_config[PINMUX_MAX_REGISTERS] = { 0, 0, 0, 0, 0 };
	int i = 0;

#if !defined(CONFIG_SPI_1) && !defined(CONFIG_SPI_CS_GPIO)
	PIN_CONFIG(mux_config,  0,  PINMUX_FUNC_B);
#endif
	PIN_CONFIG(mux_config,  1,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config,  2,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config,  3,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config,  4,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config,  5,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config,  7,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config,  8,  PINMUX_FUNC_C);
	PIN_CONFIG(mux_config,  9,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 14,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 16,  PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 17,  PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 40,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 41,  PINMUX_FUNC_B);
#ifdef CONFIG_SPI_1
	PIN_CONFIG(mux_config, 42,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 43,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 44,  PINMUX_FUNC_B);
#ifndef CONFIG_SPI_CS_GPIO
	PIN_CONFIG(mux_config, 45,  PINMUX_FUNC_B);
#endif
#endif
	PIN_CONFIG(mux_config, 55,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 56,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 57,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 63,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 64,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 65,  PINMUX_FUNC_B);
	PIN_CONFIG(mux_config, 66,  PINMUX_FUNC_B);

	for (i = 0; i < PINMUX_MAX_REGISTERS; i++) {
		sys_write32(mux_config[i], PINMUX_SELECT_REGISTER(base, i));
	}
}

static inline void _pinmux_pullups(uint32_t base_address)
{
	_quark_mcu_set_mux(base_address + PINMUX_PULLUP_OFFSET, 104,
			  PINMUX_PULLUP_ENABLE);
}

static int pinmux_initialize(struct device *port)
{
	ARG_UNUSED(port);

	_pinmux_defaults(PINMUX_BASE_ADDR);
	_pinmux_pullups(PINMUX_BASE_ADDR);

	return 0;
}

SYS_INIT(pinmux_initialize, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
