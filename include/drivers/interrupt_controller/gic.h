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

#include <zephyr/types.h>
#include <device.h>

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

#if CONFIG_GIC_VER >= 2
/*
 * 0x380  Interrupt Clear-Active Registers
 * v2/v3	GICD_ICACTIVERn
 */
#define	GICD_ICACTIVERn		(GIC_DIST_BASE + 0x380)
#endif

/*
 * 0xF00  Software Generated Interrupt Register
 * v1		ICDSGIR
 * v2/v3	GICD_SGIR
 */
#define	GICD_SGIR		(GIC_DIST_BASE + 0xf00)

/*
 * Helper Constants
 */

#if defined(CONFIG_GIC_V3)
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
#endif

/* GICD_ICFGR */
#define GICD_ICFGR_MASK			BIT_MASK(2)
#define GICD_ICFGR_TYPE			BIT(1)

/* GICD_TYPER.ITLinesNumber 0:4 */
#define GICD_TYPER_ITLINESNUM_MASK	0x1f

/*
 * Common Helper Constants
 */
#define GIC_SGI_INT_BASE		0
#define GIC_PPI_INT_BASE		16

#define GIC_IS_SGI(intid)		(((intid) >= GIC_SGI_INT_BASE) && \
					 ((intid) < GIC_PPI_INT_BASE))


#define GIC_SPI_INT_BASE		32

#define GIC_NUM_INTR_PER_REG		32

#define GIC_NUM_CFG_PER_REG		16

#define GIC_NUM_PRI_PER_REG		4

#define GIC_NUM_TGT_PER_REG		4

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

/* GIC special interrupt id */
#define GIC_INTID_SPURIOUS		1023

/* Fixme: update from platform specific define or dt */
#define GIC_NUM_CPU_IF			1
/* Fixme: arch support need to provide api/macro in SMP implementation */
#define GET_CPUID			0

#ifndef _ASMLANGUAGE

/*
 * GIC Driver Interface Functions
 */

/**
 * @brief Initialise ARM GIC driver
 *
 * @return 0 if successful
 */
int arm_gic_init(void);

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

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_GIC_H_ */
