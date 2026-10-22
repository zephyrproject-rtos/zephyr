/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_RZT2M_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_RZT2M_H_

#include <stdint.h>

#define MAX_FIFO_DEPTH      16

#define RDR(base)   ((volatile uint32_t *)(base))
#define TDR(base)   ((volatile uint32_t *)(base + 0x04))
#define CCR0(base)  ((volatile uint32_t *)(base + 0x08))
#define CCR1(base)  ((volatile uint32_t *)(base + 0x0c))
#define CCR2(base)  ((volatile uint32_t *)(base + 0x10))
#define CCR3(base)  ((volatile uint32_t *)(base + 0x14))
#define CCR4(base)  ((volatile uint32_t *)(base + 0x18))
#define FCR(base)   ((volatile uint32_t *)(base + 0x24))
#define CSR(base)   ((volatile uint32_t *)(base + 0x48))
#define FRSR(base)  ((volatile uint32_t *)(base + 0x50))
#define FTSR(base)  ((volatile uint32_t *)(base + 0x54))
#define CFCLR(base) ((volatile uint32_t *)(base + 0x68))
#define FFCLR(base) ((volatile uint32_t *)(base + 0x70))

#define CCR0_DEFAULT_VALUE 0x0
#define CCR1_DEFAULT_VALUE 0x00000010
#define CCR2_DEFAULT_VALUE 0xff00ff04
#define CCR3_DEFAULT_VALUE 0x00001203
#define CCR4_DEFAULT_VALUE 0x0

#define RDR_MASK_RDAT GENMASK(8, 0)

#define CCR0_MASK_RE    BIT(0)
#define CCR0_MASK_TE    BIT(4)
#define CCR0_MASK_DCME  BIT(9)
#define CCR0_MASK_IDSEL BIT(10)
#define CCR0_MASK_RIE   BIT(16)
#define CCR0_MASK_TIE   BIT(20)
#define CCR0_MASK_TEIE  BIT(21)
#define CCR0_MASK_SSE   BIT(24)

#define CCR1_MASK_CTSE   BIT(0)
#define CCR1_MASK_SPB2DT BIT(4)
#define CCR1_MASK_SPB2IO BIT(5)
#define CCR1_MASK_PE     BIT(8)
#define CCR1_MASK_PM     BIT(9)
#define CCR1_MASK_NFEN   BIT(28)

#define CCR2_MASK_BGDM  BIT(4)
#define CCR2_MASK_ABCS  BIT(5)
#define CCR2_MASK_ABCSE BIT(6)
#define CCR2_MASK_BRR   GENMASK(15, 8)
#define CCR2_MASK_BRME  BIT(16)
#define CCR2_MASK_CKS   GENMASK(21, 20)
#define CCR2_MASK_MDDR  GENMASK(31, 24)
#define CCR2_MASK_BAUD_SETTING                                                                     \
	(CCR2_MASK_BRME | CCR2_MASK_ABCSE | CCR2_MASK_ABCS | CCR2_MASK_BGDM | CCR2_MASK_CKS |      \
	 CCR2_MASK_BRR | CCR2_MASK_MDDR)

#define CCR3_MASK_STP   BIT(14)
#define CCR3_MASK_MP    BIT(19)
#define CCR3_MASK_FM    BIT(20)
#define CCR3_MASK_CKE   (BIT(24) | BIT(25))
#define CCR3_CKE_ENABLE BIT(24)
#define CCR3_CHR_7BIT   (BIT(8) | BIT(9))
#define CCR3_CHR_8BIT   BIT(9)

#define CCR4_MASK_ASEN BIT(16)
#define CCR4_MASK_ATEN BIT(17)

#define FCR_MASK_TFRST BIT(15)
#define FCR_MASK_RFRST BIT(23)
#define FCR_MASK_TTRG  GENMASK(12, 8)
#define FCR_MASK_RTRG  GENMASK(20, 16)
#define FCR_TTRG_15    (15 << 8)
#define FCR_RTRG_15    (15 << 16)

#define CSR_MASK_ORER BIT(24)
#define CSR_MASK_PER  BIT(27)
#define CSR_MASK_FER  BIT(28)
#define CSR_MASK_TDRE BIT(29)
#define CSR_MASK_TEND BIT(30)
#define CSR_MASK_RDRF BIT(31)

#define FRSR_MASK_DR BIT(0)
#define FRSR_R(val)  ((val >> 7) & 0x3f)

#define FTSR_T(val) (val & 0x3f)

#define CFCLR_MASK_ERSC  BIT(4)
#define CFCLR_MASK_DCMFC BIT(16)
#define CFCLR_MASK_DPERC BIT(17)
#define CFCLR_MASK_DFERC BIT(18)
#define CFCLR_MASK_ORERC BIT(24)
#define CFCLR_MASK_MFFC  BIT(26)
#define CFCLR_MASK_PERC  BIT(27)
#define CFCLR_MASK_FERC  BIT(28)
#define CFCLR_MASK_TDREC BIT(29)
#define CFCLR_MASK_RDRFC BIT(31)
#define CFCLR_ALL_FLAG_CLEAR                                                                       \
	(CFCLR_MASK_ERSC | CFCLR_MASK_DCMFC | CFCLR_MASK_DPERC | CFCLR_MASK_DFERC |                \
	 CFCLR_MASK_ORERC | CFCLR_MASK_MFFC | CFCLR_MASK_PERC | CFCLR_MASK_FERC |                  \
	 CFCLR_MASK_TDREC | CFCLR_MASK_RDRFC)

#define FFCLR_MASK_DRC BIT(0)

#define MSTPCRA                (volatile uint32_t *)(0x80280000 + 0x300)
#define MSTPCRA_MASK_SCIx(x)   BIT(x + 8)
#define BASE_TO_IFACE_ID(base) ((base & 0x1000000) ? 5 : ((base & 0xff00) >> 10) - 4)

#define CCR2_MDDR_128	BIT(31)
#define CCR2_CKS_0	0
#define CCR2_BRME_0	0
#define CCR2_BRR_243	(0xf3 << 8)
#define CCR2_BRR_39	(0x27 << 8)
#define CCR2_BGDM_1	BIT(4)

#define CCR2_BAUD_SETTING_9600   (CCR2_MDDR_128 | CCR2_BRR_243)
#define CCR2_BAUD_SETTING_115200 (CCR2_MDDR_128 | CCR2_BRR_39 | CCR2_BGDM_1)

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_RZT2M_H_ */
