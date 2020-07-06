/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_GIC_COMMON_PRIV_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_GIC_COMMON_PRIV_H_

/*
 * GIC Register Interface Base Addresses
 */
#define GIC_DIST_BASE	DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 0)
#define GIC_CPU_BASE	DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 1)

/*
 * GIC Distributor Interface
 */

/*
 * 0x000  Distributor Control Register
 * v1		ICDDCR
 * v2/v3	GICD_CTLR
 */
#define	GICD_CTLR		(GIC_DIST_BASE +   0x0)

/*
 * 0x004  Interrupt Controller Type Register
 * v1		ICDICTR
 * v2/v3	GICD_TYPER
 */
#define	GICD_TYPER		(GIC_DIST_BASE +   0x4)

/*
 * 0x008  Distributor Implementer Identification Register
 * v1		ICDIIDR
 * v2/v3	GICD_IIDR
 */
#define	GICD_IIDR		(GIC_DIST_BASE +   0x8)

/*
 * 0xF00  Software Generated Interrupt Register
 * v1		ICDSGIR
 * v2/v3	GICD_SGIR
 */
#define	GICD_SGIR		(GIC_DIST_BASE + 0xf00)


/* Offsets from GICD base or GICR(n) SGI_base */
#define GIC_DIST_IGROUPR		0x0080
#define GIC_DIST_ISENABLER		0x0100
#define GIC_DIST_ICENABLER		0x0180
#define GIC_DIST_ISPENDR		0x0200
#define GIC_DIST_ICPENDR		0x0280
#define GIC_DIST_ISACTIVER		0x0300
#define GIC_DIST_ICACTIVER		0x0380
#define GIC_DIST_IPRIORITYR		0x0400
#define GIC_DIST_ITARGETSR		0x0800
#define GIC_DIST_ICFGR			0x0c00
#define GIC_DIST_IGROUPMODR		0x0d00
#define GIC_DIST_SGIR			0x0f00

/* GICD GICR common access macros */
#define IGROUPR(base, n)		(base + GIC_DIST_IGROUPR + (n) * 4)
#define ISENABLER(base, n)		(base + GIC_DIST_ISENABLER + (n) * 4)
#define ICENABLER(base, n)		(base + GIC_DIST_ICENABLER + (n) * 4)
#define ISPENDR(base, n)		(base + GIC_DIST_ISPENDR + (n) * 4)
#define ICPENDR(base, n)		(base + GIC_DIST_ICPENDR + (n) * 4)
#define ICACTIVER(base, n)		(base + GIC_DIST_ICACTIVER + (n) * 4)
#define IPRIORITYR(base, n)		(base + GIC_DIST_IPRIORITYR + n)
#define ITARGETSR(base, n)		(base + GIC_DIST_ITARGETSR + (n) * 4)
#define ICFGR(base, n)			(base + GIC_DIST_ICFGR + (n) * 4)
#define IGROUPMODR(base, n)		(base + GIC_DIST_IGROUPMODR + (n) * 4)

/*
 * selects redistributor SGI_base for current core for PPI and SGI
 * selects distributor base for SPI
 * The macro translates to distributor base for GICv2 and GICv1
 */

#if CONFIG_GIC_VER <= 2
#define GET_DIST_BASE(intid)	GIC_DIST_BASE
#else
#define GET_DIST_BASE(intid)	((intid < GIC_SPI_INT_BASE) ? \
				(GIC_GET_RDIST(GET_CPUID) + GICR_SGI_BASE_OFF) \
				: GIC_DIST_BASE)
#endif

/* GICD_CTLR Interrupt group definitions */
#define GICD_CTLR_ENABLE_G0		0
#define GICD_CTLR_ENABLE_G1NS		1

#if CONFIG_GIC_VER > 2
#define GICD_CTLR_ENABLE_G1S		2
#endif

/* GICD_ICFGR */
#define GICD_ICFGR_MASK			BIT_MASK(2)
#define GICD_ICFGR_TYPE			BIT(1)
/* GICD_TYPER.ITLinesNumber 0:4 */
#define GICD_TYPER_ITLINESNUM_MASK	0x1f

#define GIC_NUM_INTR_PER_REG		32

#define GIC_NUM_CFG_PER_REG		16

#define GIC_NUM_PRI_PER_REG		4

#define GIC_NUM_TGT_PER_REG		4

#define GIC_INT_32X_MASK		0xffffffffU

/* GIC idle priority : value '0xff' will allow all interrupts */
#define GIC_IDLE_PRIO			0xff

/* Priority levels 0:255 */
#define GIC_PRI_MASK			0xff

/*
 * '0xa0'is used to initialize each interrtupt default priority.
 * This is an arbitrary value in current context.
 * Any value '0x80' to '0xff' will work for both NS and S state.
 * The values of individual interrupt and default has to be chosen
 * carefully if PMR and BPR based nesting and preemption has to be done.
 */
#define GIC_INT_DEF_PRI_X4		0xa0a0a0a0

/*
 * GIC distributor configuration.
 */
void gic_dist_init(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_GIC_COMMON_PRIV_H */
