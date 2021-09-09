/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC172X_VBAT_H
#define _MEC172X_VBAT_H

#include <stdint.h>
#include <stddef.h>

/* VBAT Registers Registers */
#define MCHP_VBAT_MEMORY_SIZE		128u

/* Offset 0x00 Power-Fail and Reset Status */
#define MCHP_VBATR_PFRS_OFS		0u
#define MCHP_VBATR_PFRS_MASK		0x7cu
#define MCHP_VBATR_PFRS_SYS_RST_POS	2u
#define MCHP_VBATR_PFRS_JTAG_POS	3u
#define MCHP_VBATR_PFRS_RESETI_POS	4u
#define MCHP_VBATR_PFRS_WDT_POS		5u
#define MCHP_VBATR_PFRS_SYSRESETREQ_POS 6u
#define MCHP_VBATR_PFRS_VBAT_RST_POS	7u

#define MCHP_VBATR_PFRS_SYS_RST		BIT(2)
#define MCHP_VBATR_PFRS_JTAG		BIT(3)
#define MCHP_VBATR_PFRS_RESETI		BIT(4)
#define MCHP_VBATR_PFRS_WDT		BIT(5)
#define MCHP_VBATR_PFRS_SYSRESETREQ	BIT(6)
#define MCHP_VBATR_PFRS_VBAT_RST	BIT(7)

/* Offset 0x08 32K Clock Source register */
#define MCHP_VBATR_CS_OFS		0x08u
#define MCHP_VBATR_CS_MASK		0x71f1u
#define MCHP_VBATR_CS_SO_EN_POS 0
#define MCHP_VBATR_CS_XTAL_EN_POS	8
#define MCHP_VBATR_CS_XTAL_SEL_POS	9
#define MCHP_VBATR_CS_XTAL_DHC_POS	10
#define MCHP_VBATR_CS_XTAL_CNTR_POS	11
#define MCHP_VBATR_CS_PCS_POS		16
#define MCHP_VBATR_CS_DI32_VTR_OFF_POS	18

/* Enable and start internal 32KHz Silicon Oscillator */
#define MCHP_VBATR_CS_SO_EN		BIT(0)
/* Enable and start the external crystal */
#define MCHP_VBATR_CS_XTAL_EN		BIT(8)
/* single ended crystal on XTAL2 instead of parallel across XTAL1 and XTAL2 */
#define MCHP_VBATR_CS_XTAL_SE		BIT(9)
/* disable XTAL high startup current */
#define MCHP_VBATR_CS_XTAL_DHC		BIT(10)
/* crystal amplifier gain control */
#define MCHP_VBATR_CS_XTAL_CNTR_MSK	0x1800u
#define MCHP_VBATR_CS_XTAL_CNTR_DG	0x0800u
#define MCHP_VBATR_CS_XTAL_CNTR_RG	0x1000u
#define MCHP_VBATR_CS_XTAL_CNTR_MG	0x1800u
/* Select source of peripheral 32KHz clock */
#define MCHP_VBATR_CS_PCS_MSK		0x30000u
/* 32K silicon OSC when chip powered by VBAT or VTR */
#define MCHP_VBATR_CS_PCS_VTR_VBAT_SO	0u
/* 32K external crystal when chip powered by VBAT or VTR */
#define MCHP_VBATR_CS_PCS_VTR_VBAT_XTAL 0x10000u
/* 32K input pin on VTR. Switch to Silicon OSC on VBAT */
#define MCHP_VBATR_CS_PCS_VTR_PIN_SO	0x20000u
/* 32K input pin on VTR. Switch to crystal on VBAT */
#define MCHP_VBATR_CS_PCS_VTR_PIN_XTAL	0x30000u
/* Disable internal 32K VBAT clock source when VTR is off */
#define MCHP_VBATR_CS_DI32_VTR_OFF	BIT(18)

/*
 * Monotonic Counter least significant word (32-bit), read-only.
 * Increments by one on read.
 */
#define MCHP_VBATR_MCNT_LSW_OFS		0x20u

/* Monotonic Counter most significant word (32-bit). Read-Write */
#define MCHP_VBATR_MCNT_MSW_OFS		0x24u

/* ROM Feature register */
#define MCHP_VBATR_ROM_FEAT_OFS		0x28u

/* Embedded Reset Debounce Enable register */
#define MCHP_VBATR_EMBRD_EN_OFS		0x34u
#define MCHP_VBATR_EMBRD_EN		BIT(0)

/** @brief VBAT Register Bank (VBATR) */
struct vbatr_regs {
	volatile uint32_t PFRS;
	uint32_t RSVD1[1];
	volatile uint32_t CLK32_SRC;
	uint32_t RSVD2[5];
	volatile uint32_t MCNT_LO;
	volatile uint32_t MCNT_HI;
	uint32_t RSVD3[3];
	volatile uint32_t EMBRD_EN;
};

#endif /* #ifndef _MEC172X_VBAT_H */
