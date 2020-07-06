/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_GICV1V2_PRIV_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_GICV1V2_PRIV_H_

#include <zephyr/types.h>
#include <device.h>

#define GIC_CPU_BASE	DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 1)

/*
 * GIC CPU Interface
 */

/*
 * 0x0000  CPU Interface Control Register
 * v1		ICCICR
 * v2/v3	GICC_CTLR
 */
#define GICC_CTLR		(GIC_CPU_BASE +    0x0)

/*
 * 0x0004  Interrupt Priority Mask Register
 * v1		ICCPMR
 * v2/v3	GICC_PMR
 */
#define GICC_PMR		(GIC_CPU_BASE +    0x4)

/*
 * 0x0008  Binary Point Register
 * v1		ICCBPR
 * v2/v3	GICC_BPR
 */
#define GICC_BPR		(GIC_CPU_BASE +    0x8)

/*
 * 0x000C  Interrupt Acknowledge Register
 * v1		ICCIAR
 * v2/v3	GICC_IAR
 */
#define GICC_IAR		(GIC_CPU_BASE +    0xc)

/*
 * 0x0010  End of Interrupt Register
 * v1		ICCEOIR
 * v2/v3	GICC_EOIR
 */
#define GICC_EOIR		(GIC_CPU_BASE +   0x10)

/*
 * Helper Constants
 */

/* GICC_CTLR */
#define GICC_CTLR_ENABLE_G0	BIT(0)

#define GICC_CTLR_ENABLE_G1	BIT(1)

#define GICC_CTLR_ENABLE_MASK	(GICC_CTLR_ENABLE_G0 | GICC_CTLR_ENABLE_G1)

#if defined(CONFIG_GIC_V2)

#define GICC_CTLR_FIQBYPDISGRP0	BIT(5)
#define GICC_CTLR_IRQBYPDISGRP0	BIT(6)
#define GICC_CTLR_FIQBYPDISGRP1	BIT(7)
#define GICC_CTLR_IRQBYPDISGRP1	BIT(8)

#define GICC_CTLR_BYPASS_MASK	(GICC_CTLR_FIQBYPDISGRP0 | \
				 GICC_CTLR_IRQBYPDISGRP1 | \
				 GICC_CTLR_FIQBYPDISGRP1 | \
				 GICC_CTLR_IRQBYPDISGRP1)

#endif /* CONFIG_GIC_V2 */

/* GICD_SGIR */
#define GICD_SGIR_TGTFILT(x)		(x << 24)
#define GICD_SGIR_TGTFILT_CPULIST	GICD_SGIR_TGTFILT(0b00)
#define GICD_SGIR_TGTFILT_ALLBUTREQ	GICD_SGIR_TGTFILT(0b01)
#define GICD_SGIR_TGTFILT_REQONLY	GICD_SGIR_TGTFILT(0b10)

#define GICD_SGIR_CPULIST(x)		(x << 16)
#define GICD_SGIR_CPULIST_CPU(n)	GICD_SGIR_CPULIST(BIT(n))

#define GICD_SGIR_NSATT			BIT(15)

#define GICD_SGIR_SGIINTID(x)		(x)

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_GICV1V2_PRIV_H_ */
