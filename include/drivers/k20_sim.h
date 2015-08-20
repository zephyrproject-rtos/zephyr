/* Freescale K20 microprocessor SIM Module register definitions */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module defines the SIM (System Integration Module) Registers for the
K20 Family of microprocessors
 */

#ifndef _K20SIM_H_
#define _K20SIM_H_

#include <stdint.h>
#include <misc/__assert.h>

#define SIM_OCS32K_SYS 0
#define SIM_OCS32K_RTS 2
#define SIM_OCS32K_LPO 3 /* 1kHz */

typedef union {
	uint32_t value; /* reset = 0x8000 F03F */
	struct {
		uint32_t res_0_11 : 12 __packed;
		uint32_t ramSize : 4 __packed;
		uint32_t res_16_17 : 2 __packed;
		uint32_t osc32kSel : 2 __packed;
		uint32_t res_20_28 : 9 __packed;
		uint32_t usbVoltStby : 1 __packed;
		uint32_t usbStopStby : 1 __packed;
		uint32_t usbRegEn : 1 __packed;
	} field;
} SIM_SOPT1_t; /* 0x000 */

typedef union {
	uint32_t value;
	struct {
		uint32_t res_0_23 : 24 __packed;
		uint32_t usbRegWriteEn : 1 __packed;
		uint32_t usbVoltWriteEn : 1 __packed;
		uint32_t usbStopWriteEn : 1 __packed;
		uint32_t res_27_31 : 5 __packed;
	} field;
} SIM_SOPT1CFG_t; /* 0x004 */

typedef union {
	uint32_t value;
	struct {
		uint32_t res_0_3 : 4 __packed;
		uint32_t rtcClkOutSel : 1 __packed;
		uint32_t clkOutSel : 3 __packed;
		uint32_t flexBusSL : 2 __packed;
		uint32_t res_10 : 1 __packed;
		uint32_t ptd7Pad : 1 __packed;
		uint32_t traceClkSel : 1 __packed;
		uint32_t res_13_15 : 3 __packed;
		uint32_t fllPllClkSel : 1 __packed;
		uint32_t res_17 : 1 __packed;
		uint32_t usbSrc : 1 __packed;
		uint32_t res_19_31 : 13 __packed;
	} field;
} SIM_SOPT2_t; /* 0x1004 */

typedef union {
	uint32_t value;
	struct {
		uint32_t ftm0Flt0 : 1 __packed;
		uint32_t ftm0Flt1 : 1 __packed;
		uint32_t ftm0Flt2 : 1 __packed;
		uint32_t res_3 : 1 __packed;
		uint32_t ftm1Flt0 : 1 __packed;
		uint32_t res_5_7 : 3 __packed;
		uint32_t ftm2Flt0 : 1 __packed;
		uint32_t res_9_17 : 9 __packed;
		uint32_t ftm1Ch0Src : 2 __packed;
		uint32_t ftm2Ch0Src : 2 __packed;
		uint32_t res_22_23 : 2 __packed;
		uint32_t ftm0ClkSel : 1 __packed;
		uint32_t ftm1ClkSel : 1 __packed;
		uint32_t ftm2ClkSel : 1 __packed;
		uint32_t res_27 : 1 __packed;
		uint32_t ftm0Trg0Src : 1 __packed;
		uint32_t ftm0Trg1Src : 1 __packed;
		uint32_t res_30_31 : 2 __packed;
	} field;
} SIM_SOPT4_t; /* 0x100C */

typedef union {
	uint32_t value;
	struct {
		uint32_t uart0TxSrc : 2 __packed;
		uint32_t uart0RxSrc : 2 __packed;
		uint32_t uart1TxSrc : 2 __packed;
		uint32_t uart1RxSrc : 2 __packed;
		uint32_t res_8_31 : 24 __packed;
	} field;
} SIM_SOPT5_t; /* 0x1010 */

typedef union {
	uint32_t value;
	struct {
		uint32_t uart0TxSrc : 2 __packed;
		uint32_t uart0RxSrc : 2 __packed;
		uint32_t uart1TxSrc : 2 __packed;
		uint32_t uart1RxSrc : 2 __packed;
		uint32_t res_8_31 : 24 __packed;
	} field;
} SIM_SCGC1_t; /* 0x1028*/

#define SIM_UART_CLK_ENABLE(uart) (uint32_t)(1 << (10 + uart))
typedef union {
	uint32_t value;
	struct {
		uint32_t res_0 : 1 __packed;
		uint32_t ewmClkEn_0 : 1 __packed;
		uint32_t cmtClkEn_0 : 1 __packed;
		uint32_t res_3_5 : 3 __packed;
		uint32_t i2c0ClkEn : 1 __packed;
		uint32_t i2c1ClkEn : 1 __packed;
		uint32_t res_8_9 : 2 __packed;
		uint32_t uart0ClkEn : 1 __packed;
		uint32_t uart1ClkEn : 1 __packed;
		uint32_t uart2ClkEn : 1 __packed;
		uint32_t uart3ClkEn : 1 __packed;
		uint32_t res_14_17 : 4 __packed;
		uint32_t usbClkEn : 1 __packed;
		uint32_t cmpClkEn : 1 __packed;
		uint32_t vrefClkEn : 1 __packed;
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
		uint32_t portA_ClkEn : 1 __packed;
		uint32_t portB_ClkEn : 1 __packed;
		uint32_t portC_ClkEn : 1 __packed;
		uint32_t portD_ClkEn : 1 __packed;
		uint32_t portE_ClkEn : 1 __packed;
		uint32_t res_14_31 : 18 __packed;
	} field;
} SIM_SCGC5_t; /* 0x1038 */

#define SIM_CLKDIV1_OUTDIV1_MASK 0xF0000000
#define SIM_CLKDIV1_OUTDIV1_SHIFT 28
#define SIM_CLKDIV1_OUTDIV2_MASK 0x0F000000
#define SIM_CLKDIV1_OUTDIV2_SHIFT 24
#define SIM_CLKDIV1_OUTDIV3_MASK 0x00F00000
#define SIM_CLKDIV1_OUTDIV3_SHIFT 20
#define SIM_CLKDIV1_OUTDIV4_MASK 0x000F0000
#define SIM_CLKDIV1_OUTDIV4_SHIFT 16
#define SIM_CLKDIV(value) ((value) - 1)

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

typedef volatile struct {
	SIM_SOPT1_t sopt1;		       /* 0x0000 */
	SIM_SOPT1CFG_t sopt1Cfg;	       /* 0x0004 */
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
} K20_SIM_t;

static ALWAYS_INLINE void _k20_sim_uart_clk_enable(K20_SIM_t *sim_p,
						   uint8_t which)
{
	sim_p->scgc4.value |= SIM_UART_CLK_ENABLE(which);
}

#endif /* _K20SIM_H_ */
