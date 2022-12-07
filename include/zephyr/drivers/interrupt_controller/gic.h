/*
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for ARM Generic Interrupt Controller
 *
 * The Generic Interrupt Controller (GIC) is the default interrupt controller
 * for the ARM A and R profile cores.  This driver is used by the ARM arch
 * implementation to handle interrupts.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_GIC_H_

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
 * 0x080  Interrupt Group Registers
 * v1		ICDISRn
 * v2/v3	GICD_IGROUPRn
 */
#define	GICD_IGROUPRn		(GIC_DIST_BASE +  0x80)

/*
 * 0x100  Interrupt Set-Enable Registers
 * v1		ICDISERn
 * v2/v3	GICD_ISENABLERn
 */
#define	GICD_ISENABLERn		(GIC_DIST_BASE + 0x100)

/*
 * 0x180  Interrupt Clear-Enable Registers
 * v1		ICDICERn
 * v2/v3	GICD_ICENABLERn
 */
#define	GICD_ICENABLERn		(GIC_DIST_BASE + 0x180)

/*
 * 0x200  Interrupt Set-Pending Registers
 * v1		ICDISPRn
 * v2/v3	GICD_ISPENDRn
 */
#define	GICD_ISPENDRn		(GIC_DIST_BASE + 0x200)

/*
 * 0x280  Interrupt Clear-Pending Registers
 * v1		ICDICPRn
 * v2/v3	GICD_ICPENDRn
 */
#define	GICD_ICPENDRn		(GIC_DIST_BASE + 0x280)

/*
 * 0x300  Interrupt Set-Active Registers
 * v1		ICDABRn
 * v2/v3	GICD_ISACTIVERn
 */
#define	GICD_ISACTIVERn		(GIC_DIST_BASE + 0x300)

#if CONFIG_GIC_VER >= 2
/*
 * 0x380  Interrupt Clear-Active Registers
 * v2/v3	GICD_ICACTIVERn
 */
#define	GICD_ICACTIVERn		(GIC_DIST_BASE + 0x380)
#endif

/*
 * 0x400  Interrupt Priority Registers
 * v1		ICDIPRn
 * v2/v3	GICD_IPRIORITYRn
 */
#define	GICD_IPRIORITYRn	(GIC_DIST_BASE + 0x400)

/*
 * 0x800  Interrupt Processor Targets Registers
 * v1		ICDIPTRn
 * v2/v3	GICD_ITARGETSRn
 */
#define	GICD_ITARGETSRn		(GIC_DIST_BASE + 0x800)

/*
 * 0xC00  Interrupt Configuration Registers
 * v1		ICDICRn
 * v2/v3	GICD_ICFGRn
 */
#define	GICD_ICFGRn		(GIC_DIST_BASE + 0xc00)

/*
 * 0xF00  Software Generated Interrupt Register
 * v1		ICDSGIR
 * v2/v3	GICD_SGIR
 */
#define	GICD_SGIR		(GIC_DIST_BASE + 0xf00)

/*
 * GIC CPU Interface
 */

#if CONFIG_GIC_VER <= 2

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
#define GICC_CTLR_ENABLEGRP0	BIT(0)
#define GICC_CTLR_ENABLEGRP1	BIT(1)

#define GICC_CTLR_ENABLE_MASK	(GICC_CTLR_ENABLEGRP0 | GICC_CTLR_ENABLEGRP1)

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
#define GICD_SGIR_TGTFILT(x)		((x) << 24)
#define GICD_SGIR_TGTFILT_CPULIST	GICD_SGIR_TGTFILT(0b00)
#define GICD_SGIR_TGTFILT_ALLBUTREQ	GICD_SGIR_TGTFILT(0b01)
#define GICD_SGIR_TGTFILT_REQONLY	GICD_SGIR_TGTFILT(0b10)

#define GICD_SGIR_CPULIST(x)		((x) << 16)
#define GICD_SGIR_CPULIST_CPU(n)	GICD_SGIR_CPULIST(BIT(n))
#define GICD_SGIR_CPULIST_MASK		0xff

#define GICD_SGIR_NSATT			BIT(15)

#define GICD_SGIR_SGIINTID(x)		(x)

#endif /* CONFIG_GIC_VER <= 2 */


/* GICD_ICFGR */
#define GICD_ICFGR_MASK			BIT_MASK(2)
#define GICD_ICFGR_TYPE			BIT(1)

/* GICD_TYPER.ITLinesNumber 0:4 */
#define GICD_TYPER_ITLINESNUM_MASK	0x1f

/* GICD_TYPER.IDbits */
#define GICD_TYPER_IDBITS(typer)	((((typer) >> 19) & 0x1f) + 1)

/*
 * Common Helper Constants
 */
#define GIC_SGI_INT_BASE		0
#define GIC_PPI_INT_BASE		16

#define GIC_IS_SGI(intid)		(((intid) >= GIC_SGI_INT_BASE) && \
					 ((intid) < GIC_PPI_INT_BASE))


#define GIC_SPI_INT_BASE		32

#define GIC_SPI_MAX_INTID		1019

#define GIC_IS_SPI(intid)		(((intid) >= GIC_SPI_INT_BASE) && \
					((intid) <= GIC_SPI_MAX_INTID))

#define GIC_NUM_INTR_PER_REG		32

#define GIC_NUM_CFG_PER_REG		16

#define GIC_NUM_PRI_PER_REG		4

/* GIC idle priority : value '0xff' will allow all interrupts */
#define GIC_IDLE_PRIO			0xff

/* Priority levels 0:255 */
#define GIC_PRI_MASK			0xff

/*
 * '0xa0'is used to initialize each interrupt default priority.
 * This is an arbitrary value in current context.
 * Any value '0x80' to '0xff' will work for both NS and S state.
 * The values of individual interrupt and default has to be chosen
 * carefully if PMR and BPR based nesting and preemption has to be done.
 */
#define GIC_INT_DEF_PRI_X4		0xa0a0a0a0

/* GIC special interrupt id */
#define GIC_INTID_SPURIOUS		1023

/* Fixme: update from platform specific define or dt */
#define GIC_NUM_CPU_IF			CONFIG_MP_MAX_NUM_CPUS

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <zephyr/device.h>

/*
 * GIC Driver Interface Functions
 */

/**
 * @brief Enable interrupt
 *
 * @param irq interrupt ID
 */
void arm_gic_irq_enable(unsigned int irq);

/**
 * @brief Disable interrupt
 *
 * @param irq interrupt ID
 */
void arm_gic_irq_disable(unsigned int irq);

/**
 * @brief Check if an interrupt is enabled
 *
 * @param irq interrupt ID
 * @return Returns true if interrupt is enabled, false otherwise
 */
bool arm_gic_irq_is_enabled(unsigned int irq);

/**
 * @brief Set interrupt priority
 *
 * @param irq interrupt ID
 * @param prio interrupt priority
 * @param flags interrupt flags
 */
void arm_gic_irq_set_priority(
	unsigned int irq, unsigned int prio, unsigned int flags);

/**
 * @brief Get active interrupt ID
 *
 * @return Returns the ID of an active interrupt
 */
unsigned int arm_gic_get_active(void);

/**
 * @brief Signal end-of-interrupt
 *
 * @param irq interrupt ID
 */
void arm_gic_eoi(unsigned int irq);

#ifdef CONFIG_SMP
/**
 * @brief Initialize GIC of secondary cores
 */
void arm_gic_secondary_init(void);
#endif

/**
 * @brief raise SGI to target cores
 *
 * @param sgi_id      SGI ID 0 to 15
 * @param target_aff  target affinity in mpidr form.
 *                    Aff level 1 2 3 will be extracted by api.
 * @param target_list bitmask of target cores
 */
void gic_raise_sgi(unsigned int sgi_id, uint64_t target_aff,
		   uint16_t target_list);

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_GIC_H_ */
