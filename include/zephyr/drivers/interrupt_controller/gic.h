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

#define GIC_DIST_BASE DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 0)
#define GIC_CPU_BASE  DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 1)

/*
 * GIC Distributor Interface
 */

/**
 * @brief GIC Distributor Control Register
 *
 * v1		ICDDCR
 * v2/v3	GICD_CTLR
 */
#define GICD_CTLR (GIC_DIST_BASE + 0x0)

/**
 * @brief GIC Distributor Interrupt Controller Type Register
 *
 * v1		ICDICTR
 * v2/v3	GICD_TYPER
 */
#define GICD_TYPER (GIC_DIST_BASE + 0x4)

/**
 * @brief GIC Distributor Implementer Identification Register
 *
 * v1		ICDIIDR
 * v2/v3	GICD_IIDR
 */
#define GICD_IIDR (GIC_DIST_BASE + 0x8)

/**
 * @brief GIC Distributor Interrupt Group Registers
 *
 * v1		ICDISRn
 * v2/v3	GICD_IGROUPRn
 */
#define GICD_IGROUPRn (GIC_DIST_BASE + 0x80)

/**
 * @brief GIC Distributor Interrupt Group Registers for Extended SPI Range
 *
 * v3.1		GICD_IGROUPRnE
 */
#define GICD_IGROUPRnE (GIC_DIST_BASE + 0x1000)

/**
 * @brief GIC Distributor Interrupt Set-Enable Registers
 *
 * v1		ICDISERn
 * v2/v3	GICD_ISENABLERn
 */
#define GICD_ISENABLERn (GIC_DIST_BASE + 0x100)

/**
 * @brief GIC Distributor Interrupt Set-Enable Registers for Extended SPI Range
 *
 * v3.1		GICD_ISENABLERnE
 */
#define GICD_ISENABLERnE (GIC_DIST_BASE + 0x1200)

/**
 * @brief GIC Distributor Interrupt Clear-Enable Registers
 *
 * v1		ICDICERn
 * v2/v3	GICD_ICENABLERn
 */
#define GICD_ICENABLERn (GIC_DIST_BASE + 0x180)

/**
 * @brief GIC Distributor Interrupt Clear-Enable Registers for Extended SPI Range
 *
 * v3.1		GICD_ICENABLERnE
 */
#define GICD_ICENABLERnE (GIC_DIST_BASE + 0x1400)

/**
 * @brief GIC Distributor Interrupt Set-Pending Registers
 *
 * v1		ICDISPRn
 * v2/v3	GICD_ISPENDRn
 */
#define GICD_ISPENDRn (GIC_DIST_BASE + 0x200)

/**
 * @brief GIC Distributor Interrupt Set-Pending Registers for Extended SPI Range
 *
 * v3.1		GICD_ISPENDRnE
 */
#define GICD_ISPENDRnE (GIC_DIST_BASE + 0x1600)

/**
 * @brief GIC Distributor Interrupt Clear-Pending Registers
 *
 * v1		ICDICPRn
 * v2/v3	GICD_ICPENDRn
 */
#define GICD_ICPENDRn (GIC_DIST_BASE + 0x280)

/**
 * @brief GIC Distributor Interrupt Clear-Pending Registers for Extended SPI Range
 *
 * v3.1		GICD_ICPENDRnE
 */
#define GICD_ICPENDRnE (GIC_DIST_BASE + 0x1800)

/**
 * @brief GIC Distributor Interrupt Set-Active Registers
 *
 * v1		ICDABRn
 * v2/v3	GICD_ISACTIVERn
 */
#define GICD_ISACTIVERn (GIC_DIST_BASE + 0x300)

/**
 * @brief GIC Distributor Interrupt Set-Active Registers for Extended SPI Range
 *
 * v3.1		GICD_ISACTIVERnE
 */
#define GICD_ISACTIVERnE (GIC_DIST_BASE + 0x1A00)

#if (CONFIG_GIC_VER >= 2) || defined(__DOXYGEN__)
/**
 * @brief GIC Distributor Interrupt Clear-Active Registers
 *
 * v2/v3	GICD_ICACTIVERn
 */
#define GICD_ICACTIVERn (GIC_DIST_BASE + 0x380)

/**
 * @brief GIC Distributor Interrupt Clear-Active Registers for Extended SPI Range
 *
 * v3.1		GICD_ICACTIVERnE
 */
#define GICD_ICACTIVERnE (GIC_DIST_BASE + 0x1C00)
#endif /* CONFIG_GIC_VER >= 2 */

/**
 * @brief GIC Distributor Interrupt Priority Registers
 *
 * v1		ICDIPRn
 * v2/v3	GICD_IPRIORITYRn
 */
#define GICD_IPRIORITYRn (GIC_DIST_BASE + 0x400)

/**
 * @brief GIC Distributor Interrupt Priority Registers for Extended SPI Range
 *
 * v3.1		GICD_IPRIORITYRnE
 */
#define GICD_IPRIORITYRnE (GIC_DIST_BASE + 0x2000)

/**
 * @brief GIC Distributor Interrupt Processor Targets Registers
 *
 * v1		ICDIPTRn
 * v2/v3	GICD_ITARGETSRn
 */
#define GICD_ITARGETSRn (GIC_DIST_BASE + 0x800)

/**
 * @brief GIC Distributor Interrupt Configuration Registers
 *
 * v1		ICDICRn
 * v2/v3	GICD_ICFGRn
 */
#define GICD_ICFGRn (GIC_DIST_BASE + 0xc00)

/**
 * @brief GIC Distributor Interrupt Configuration Registers for Extended SPI Range
 *
 * v3.1		GICD_ICFGRnE
 */
#define GICD_ICFGRnE (GIC_DIST_BASE + 0x3000)

/**
 * @brief GIC Distributor Software Generated Interrupt Register
 *
 * v1		ICDSGIR
 * v2/v3	GICD_SGIR
 */
#define GICD_SGIR (GIC_DIST_BASE + 0xf00)

/**
 * @brief Disable Security (DS) bit in @ref GICD_CTLR
 *
 * v3/v3.1	See ARM GIC architecture spec, GICD_CTLR
 */
#define GICD_CTLR_DS BIT(6)

/*
 * GIC CPU Interface
 */

#if (CONFIG_GIC_VER <= 2) || defined(__DOXYGEN__)

/**
 * @brief GIC CPU Interface Control Register
 *
 * v1		ICCICR
 * v2		GICC_CTLR
 */
#define GICC_CTLR (GIC_CPU_BASE + 0x0)

/**
 * @brief GIC CPU Interface Interrupt Priority Mask Register
 *
 * v1		ICCPMR
 * v2		GICC_PMR
 */
#define GICC_PMR (GIC_CPU_BASE + 0x4)

/**
 * @brief GIC CPU Interface Binary Point Register
 *
 * v1		ICCBPR
 * v2		GICC_BPR
 */
#define GICC_BPR (GIC_CPU_BASE + 0x8)

/**
 * @brief GIC CPU Interface Interrupt Acknowledge Register
 *
 * v1		ICCIAR
 * v2		GICC_IAR
 */
#define GICC_IAR (GIC_CPU_BASE + 0xc)

/**
 * @brief GIC CPU Interface End of Interrupt Register
 *
 * v1		ICCEOIR
 * v2		GICC_EOIR
 */
#define GICC_EOIR (GIC_CPU_BASE + 0x10)

/*
 * Helper Constants
 */

/** @cond INTERNAL_HIDDEN */

/* GICC_CTLR */
#define GICC_CTLR_ENABLEGRP0 BIT(0)
#define GICC_CTLR_ENABLEGRP1 BIT(1)

#define GICC_CTLR_ENABLE_MASK (GICC_CTLR_ENABLEGRP0 | GICC_CTLR_ENABLEGRP1)

#if defined(CONFIG_GIC_V2)

#define GICC_CTLR_FIQBYPDISGRP0 BIT(5)
#define GICC_CTLR_IRQBYPDISGRP0 BIT(6)
#define GICC_CTLR_FIQBYPDISGRP1 BIT(7)
#define GICC_CTLR_IRQBYPDISGRP1 BIT(8)

#define GICC_CTLR_BYPASS_MASK                                                                      \
	(GICC_CTLR_FIQBYPDISGRP0 | GICC_CTLR_IRQBYPDISGRP1 | GICC_CTLR_FIQBYPDISGRP1 |             \
	 GICC_CTLR_IRQBYPDISGRP1)

#endif /* CONFIG_GIC_V2 */

/* GICD_SGIR */
#define GICD_SGIR_TGTFILT(x)        ((x) << 24)
#define GICD_SGIR_TGTFILT_CPULIST   GICD_SGIR_TGTFILT(0b00)
#define GICD_SGIR_TGTFILT_ALLBUTREQ GICD_SGIR_TGTFILT(0b01)
#define GICD_SGIR_TGTFILT_REQONLY   GICD_SGIR_TGTFILT(0b10)

#define GICD_SGIR_CPULIST(x)     ((x) << 16)
#define GICD_SGIR_CPULIST_CPU(n) GICD_SGIR_CPULIST(BIT(n))
#define GICD_SGIR_CPULIST_MASK   0xff

#define GICD_SGIR_NSATT BIT(15)

#define GICD_SGIR_SGIINTID(x) (x)

/** @endcond  */

#endif /* CONFIG_GIC_VER <= 2 */

/** @cond INTERNAL_HIDDEN */

/* GICD_ICFGR */
#define GICD_ICFGR_MASK BIT_MASK(2)
#define GICD_ICFGR_TYPE BIT(1)

/* GICD_TYPER.ITLinesNumber 0:4 */
#define GICD_TYPER_ITLINESNUM_MASK 0x1f

/* GICD_TYPER.ESPI [8] */
#define GICD_TYPER_ESPI_MASK BIT(8)

/* GICD_TYPER.IDbits */
#define GICD_TYPER_IDBITS(typer) ((((typer) >> 19) & 0x1f) + 1)

/** @endcond  */

/*
 * Common Helper Constants
 */

/** Base ID of SGI interrupts */
#define GIC_SGI_INT_BASE 0

/** Base ID of PPI interrupts */
#define GIC_PPI_INT_BASE 16

#define GIC_IS_SGI(intid) (((intid) >= GIC_SGI_INT_BASE) && ((intid) < GIC_PPI_INT_BASE))

/** Base ID of SPI interrupts */
#define GIC_SPI_INT_BASE 32

/** Max ID of SPI interrupts */
#define GIC_SPI_MAX_INTID 1019

/** @cond INTERNAL_HIDDEN */

#define GIC_IS_SPI(intid) (((intid) >= GIC_SPI_INT_BASE) && ((intid) <= GIC_SPI_MAX_INTID))

#define GIC_NUM_INTR_PER_REG 32

#define GIC_NUM_CFG_PER_REG 16

#define GIC_NUM_PRI_PER_REG 4

/** @endcond  */

/** GIC idle priority : value '0xff' will allow all interrupts */
#define GIC_IDLE_PRIO 0xff

/** Priority levels 0:255 */
#define GIC_PRI_MASK 0xff

/**
 * @brief Default priority initializer helper macro
 *
 * '0xa0'is used to initialize each interrupt default priority.
 * This is an arbitrary value in current context.
 * Any value '0x80' to '0xff' will work for both NS and S state.
 * The values of individual interrupt and default has to be chosen
 * carefully if PMR and BPR based nesting and preemption has to be done.
 */
#define GIC_INT_DEF_PRI_X4 0xa0a0a0a0

/** GIC special interrupt id */
#define GIC_INTID_SPURIOUS 1023

/**
 * @brief Max number of supported CPUs in a SMP environment
 *
 * Fixme: update from platform specific define or dt
 * DEPRECATED - Use CONFIG_MP_MAX_NUM_CPUS instead.
 */
#define GIC_NUM_CPU_IF CONFIG_MP_MAX_NUM_CPUS

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
 * @brief Check if an interrupt is pending
 *
 * @param irq interrupt ID
 * @return Returns true if interrupt is pending, false otherwise
 */
bool arm_gic_irq_is_pending(unsigned int irq);

/**
 * @brief Set interrupt as pending
 *
 * @param irq interrupt ID
 */
void arm_gic_irq_set_pending(unsigned int irq);

/**
 * @brief Clear the pending irq
 *
 * @param irq interrupt ID
 */
void arm_gic_irq_clear_pending(unsigned int irq);

/**
 * @brief Set interrupt priority
 *
 * @param irq interrupt ID
 * @param prio interrupt priority
 * @param flags interrupt flags
 */
void arm_gic_irq_set_priority(unsigned int irq, unsigned int prio, unsigned int flags);

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

#if defined(CONFIG_SMP) || defined(__DOXYGEN__)
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
void gic_raise_sgi(unsigned int sgi_id, uint64_t target_aff, uint16_t target_list);

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_GIC_H_ */
