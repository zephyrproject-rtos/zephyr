/*
 * pinmux_board_quark_d2000_crb.c - Quark D2000 Customer Reference
 * Board pinmux driver
 */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file pinmux operations for Quark_D2000
 */

#include <nanokernel.h>
#include <board.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <sys_io.h>
#include "pinmux/pinmux.h"

#include "pinmux_quark_mcu.h"

/***************************
 * PINMUX mapping
 *
 * The following lines detail the possible options for the pinmux and their
 * associated pins and ball points.
 * This is the full pinmap that we have available on the board for configuration
 * including the ball position and the various modes that can be set.  In the
 * _pinmux_defaults we do not spend any time setting values that are using mode
 * A as the hardware brings up all devices by default in mode A.
 */
	/* pin, ball, mode A, mode B,      mode C       */
	/*  0   F00, gpio_0,  ai_0,        spi_m_ss0    */
	/*  1   F01, gpio_1,  ai_1,        spi_m_ss1    */
	/*  2   F02, gpio_2,  ai_2,        spi_m_ss2    */
	/*  3   F03, gpio_3,  ai_3,        spi_m_ss3    */
	/*  4   F04, gpio_4,  ai_4,        rtc_clk_out  */
	/*  5   F05, gpio_5,  ai_5,        sys_clk_out  */
	/*  6   F06, gpio_6,  ai_6,        i2c_scl      */
	/*  7   F07, gpio_7,  ai_7,        i2c_sda      */
	/*  8   F08, gpio_8,  ai_8,        spi_s_sclk   */
	/*  9   F09, gpio_9,  ai_9,        spi_s_sdin   */
	/* 10   F10, gpio_10, ai_10,       spi_s_sdout  */
	/* 11   F11, gpio_11, ai_11,       spi_s_scs    */
	/* 12   F12, gpio_12, ai_12,       uart_a_txd   */
	/* 13   F13, gpio_13, ai_13,       uart_a_rxd   */
	/* 14   F14, gpio_14, ai_14,       uart_a_rts   */
	/* 15   F15, gpio_15, ai_15,       uart_a_cts   */
	/* 16   F16, gpio_16, ai_16,       spi_m_sclk   */
	/* 17   F17, gpio_17, ai_17,       spi_m_mosi   */
	/* 18   F18, gpio_18, ai_18,       spi_m_miso   */
	/* 19   F19, tdo,     gpio_19,     pwm0         */
	/* 20   F20, trst_n,  gpio_20,     uart_b_txd   */
	/* 21   F21, tck,     gpio_21,     uart_b_rxd   */
	/* 22   F22, tms,     gpio_22,     uart_b_rts   */
	/* 23   F23, tdi,     gpio_23,     uart_b_cts   */
	/* 24   F24, gpio_24, lpd_sig_out, pwm1         */

/******** End PINMUX mapping **************************/

#define PINMUX_MAX_REGISTERS 2

static void _pinmux_defaults(uint32_t base)
{
	uint32_t mux_config[PINMUX_MAX_REGISTERS] = { 0, 0 };
	int i = 0;

	PIN_CONFIG(mux_config,  0, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config,  3, PINMUX_FUNC_B);
	PIN_CONFIG(mux_config,  4, PINMUX_FUNC_B);
	PIN_CONFIG(mux_config,  6, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config,  7, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 12, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 13, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 14, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 15, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 16, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 17, PINMUX_FUNC_C);
	PIN_CONFIG(mux_config, 18, PINMUX_FUNC_C);

	for (i = 0; i < PINMUX_MAX_REGISTERS; i++) {
		sys_write32(mux_config[i], PINMUX_SELECT_REGISTER(base, i));
	}
}

static int pinmux_initialize(struct device *port)
{
	ARG_UNUSED(port);

	_pinmux_defaults(PINMUX_BASE_ADDR);

	/* Enable the UART RX pin to receive input */
	_quark_mcu_set_mux(PINMUX_BASE_ADDR + PINMUX_INPUT_OFFSET, 5, 0x1);

	return 0;
}

SYS_INIT(pinmux_initialize, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
