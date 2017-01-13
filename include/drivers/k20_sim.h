/* Freescale K20 microprocessor SIM Module register definitions */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief SIM Registers Definitions for the K20 Microprocessor
 *
 * This module defines the SIM (System Integration Module) Registers for the
 * K20 Family of microprocessors
 */

#ifndef _K20SIM_H_
#define _K20SIM_H_

#include <stdint.h>
#include <misc/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIM_OCS32K_SYS 0
#define SIM_OCS32K_RTS 2
#define SIM_OCS32K_LPO 3 /* 1kHz */

typedef union {
	uint32_t value; /* reset = 0x8000 F03F */
	struct {
		uint32_t res_0_11 : 12 __packed;
		uint32_t ram_size : 4 __packed;
		uint32_t res_16_17 : 2 __packed;
		uint32_t osc32k_sel : 2 __packed;
		uint32_t res_20_28 : 9 __packed;
		uint32_t usb_volt_stby : 1 __packed;
		uint32_t usb_stop_stby : 1 __packed;
		uint32_t usb_reg_en : 1 __packed;
	} field;
} SIM_SOPT1_t; /* 0x000 */

typedef union {
	uint32_t value;
	struct {
		uint32_t res_0_23 : 24 __packed;
		uint32_t usb_reg_write_en : 1 __packed;
		uint32_t usb_volt_write_en : 1 __packed;
		uint32_t usb_stop_write_en : 1 __packed;
		uint32_t res_27_31 : 5 __packed;
	} field;
} SIM_SOPT1CFG_t; /* 0x004 */

typedef union {
	uint32_t value;
	struct {
		uint32_t res_0_3 : 4 __packed;
		uint32_t rtc_clk_out_sel : 1 __packed;
		uint32_t clk_out_sel : 3 __packed;
		uint32_t flex_bus_sl : 2 __packed;
		uint32_t res_10 : 1 __packed;
		uint32_t ptd7pad : 1 __packed;
		uint32_t trace_clk_sel : 1 __packed;
		uint32_t res_13_15 : 3 __packed;
		uint32_t fll_pll_clk_sel : 1 __packed;
		uint32_t res_17 : 1 __packed;
		uint32_t usb_src : 1 __packed;
		uint32_t res_19_31 : 13 __packed;
	} field;
} SIM_SOPT2_t; /* 0x1004 */

typedef union {
	uint32_t value;
	struct {
		uint32_t ftm0_flt0 : 1 __packed;
		uint32_t ftm0_flt1 : 1 __packed;
		uint32_t ftm0_flt2 : 1 __packed;
		uint32_t res_3 : 1 __packed;
		uint32_t ftm1_flt0 : 1 __packed;
		uint32_t res_5_7 : 3 __packed;
		uint32_t ftm2_flt0 : 1 __packed;
		uint32_t res_9_17 : 9 __packed;
		uint32_t ftm1_ch0_src : 2 __packed;
		uint32_t ftm2_ch0_src : 2 __packed;
		uint32_t res_22_23 : 2 __packed;
		uint32_t ftm0_clk_sel : 1 __packed;
		uint32_t ftm1_clk_sel : 1 __packed;
		uint32_t ftm2_clk_sel : 1 __packed;
		uint32_t res_27 : 1 __packed;
		uint32_t ftm0_trg0_src : 1 __packed;
		uint32_t ftm0_trg1_src : 1 __packed;
		uint32_t res_30_31 : 2 __packed;
	} field;
} SIM_SOPT4_t; /* 0x100C */

typedef union {
	uint32_t value;
	struct {
		uint32_t uart0_tx_src : 2 __packed;
		uint32_t uart0_rx_src : 2 __packed;
		uint32_t uart1_tx_src : 2 __packed;
		uint32_t uart1_rx_src : 2 __packed;
		uint32_t res_8_31 : 24 __packed;
	} field;
} SIM_SOPT5_t; /* 0x1010 */

typedef union {
	uint32_t value;
	struct {
		uint32_t uart0_tx_src : 2 __packed;
		uint32_t uart0_rx_src : 2 __packed;
		uint32_t uart1_tx_src : 2 __packed;
		uint32_t uart1_rx_src : 2 __packed;
		uint32_t res_8_9 : 2 __packed;
		uint32_t uart4_clk_en : 1 __packed;
		uint32_t uart5_clk_en : 1 __packed;
		uint32_t res_12_31: 20 __packed;
	} field;
} SIM_SCGC1_t; /* 0x1028*/

typedef union {
	uint32_t value;
	struct {
		uint32_t res_0 : 1 __packed;
		uint32_t ewm_clk_en_0 : 1 __packed;
		uint32_t cmt_clk_en_0 : 1 __packed;
		uint32_t res_3_5 : 3 __packed;
		uint32_t i2c0_clk_en : 1 __packed;
		uint32_t i2c1_clk_en : 1 __packed;
		uint32_t res_8_9 : 2 __packed;
		uint32_t uart0_clk_en : 1 __packed;
		uint32_t uart1_clk_en : 1 __packed;
		uint32_t uart2_clk_en : 1 __packed;
		uint32_t uart3_clk_en : 1 __packed;
		uint32_t res_14_17 : 4 __packed;
		uint32_t usb_clk_en : 1 __packed;
		uint32_t cmp_clk_en : 1 __packed;
		uint32_t vref_clk_en : 1 __packed;
		uint32_t res_21_31 : 11 __packed;
	} field;
} SIM_SCGC4_t; /* 0x1034 */

#define SIM_SCGC5_PORTA_CLK_EN (1 << 9)
#define SIM_SCGC5_PORTB_CLK_EN (1 << 10)
#define SIM_SCGC5_PORTC_CLK_EN (1 << 11)
#define SIM_SCGC5_PORTD_CLK_EN (1 << 12)
#define SIM_SCGC5_PORTE_CLK_EN (1 << 13)

typedef union {
	uint32_t value; /* reset 0 */
	struct {
		uint32_t lptimer : 1 __packed;
		uint32_t res_1_4 : 4 __packed;
		uint32_t tsi : 1 __packed;
		uint32_t res_6_8 : 3 __packed;
		uint32_t port_a_clk_en : 1 __packed;
		uint32_t port_b_clk_en : 1 __packed;
		uint32_t port_c_clk_en : 1 __packed;
		uint32_t port_d_clk_en : 1 __packed;
		uint32_t port_e_clk_en : 1 __packed;
		uint32_t res_14_31 : 18 __packed;
	} field;
} SIM_SCGC5_t; /* 0x1038 */

typedef union {
	uint32_t value; /* reset 0x0001 0000 */
	struct {
		uint32_t res_0_15 : 16 __packed;
		uint32_t outdiv4 : 4 __packed;
		uint32_t outdiv3 : 4 __packed;
		uint32_t outdiv2 : 4 __packed;
		uint32_t outdiv1 : 4 __packed;
	} field;
} SIM_CLKDIV1_t; /* 0x1044 */

/* K20 Microntroller SIM module register structure */

struct K20_SIM {
	SIM_SOPT1_t sopt1;		       /* 0x0000 */
	SIM_SOPT1CFG_t sopt1_cfg;	       /* 0x0004 */
	uint8_t res0008_1003[0x1003 - 0x8];    /* 0x0008-0x1003 Reserved */
	SIM_SOPT2_t sopt2;		       /* 0x1004 */
	uint32_t res1008;		       /* 0x1008 Reserved */
	SIM_SOPT4_t sopt4;		       /* 0x100C */
	SIM_SOPT5_t sopt5;		       /* 0x1010 */
	uint32_t res1014;		       /* 0x1014 Reserved */
	uint32_t sopt7;			       /* 0x1018 */
	uint8_t res101c_1027[0x1027 - 0x101c]; /* Reserved */
	SIM_SCGC1_t scgc1;		       /* 0x1028 */
	uint32_t scgc2;			       /* 0x102C */
	uint32_t scgc3;			       /* 0x1030 */
	SIM_SCGC4_t scgc4;		       /* 0x1034 */
	SIM_SCGC5_t scgc5;		       /* 0x1038 */
	uint32_t scgc6;			       /* 0x103C */
	uint32_t scgc7;			       /* 0x1040 */
	SIM_CLKDIV1_t clkdiv1;		       /* 0x1044 */
	uint32_t clkdiv2;		       /* 0x1048 */
	uint8_t res104c_1063[0x1063 - 0x104c]; /* Reserved */
};

#ifdef __cplusplus
}
#endif

#endif /* _K20SIM_H_ */
