/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_CPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_CPU_H_

#include <zephyr/arch/arm/cortex_m/nvic.h>
#include <zephyr/sys/util.h>

/*
 * Exception/interrupt priority constants.
 *
 * Factored out of <zephyr/arch/arm/cortex_m/exception.h> so that low-level
 * headers (e.g. <zephyr/arch/arm/asm_inline_gcc.h>) can obtain
 * _EXC_IRQ_DEFAULT_PRIO without pulling in the full exception header, which in
 * turn drags in the Cortex-M context-switch machinery and <kernel/thread.h>.
 * These are constant expressions and are therefore usable from assembler.
 */

/* for assembler, only works with constants */
#define Z_EXC_PRIO(pri) (((pri) << (8 - NUM_IRQ_PRIO_BITS)) & 0xff)

/*
 * In architecture variants with non-programmable fault exceptions
 * (e.g. Cortex-M Baseline variants), hardware ensures processor faults
 * are given the highest interrupt priority level. SVCalls are assigned
 * the highest configurable priority level (level 0); note, however, that
 * this interrupt level may be shared with HW interrupts.
 *
 * In Cortex variants with programmable fault exception priorities we
 * assign the highest interrupt priority level (level 0) to processor faults
 * with configurable priority.
 * The highest priority level may be shared with either Zero-Latency IRQs (if
 * support for the feature is enabled) or with SVCall priority level.
 * Regular HW IRQs are always assigned priority levels lower than the priority
 * levels for SVCalls, Zero-Latency IRQs and processor faults.
 *
 * PendSV IRQ (which is used in Cortex-M variants to implement thread
 * context-switching) is assigned the lowest IRQ priority level.
 */
#if defined(CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS)
#define _EXCEPTION_RESERVED_PRIO 1
#else
#define _EXCEPTION_RESERVED_PRIO 0
#endif

#define _EXC_FAULT_PRIO             0
#define _EXC_ZERO_LATENCY_IRQS_PRIO 0
#define _EXC_SVC_PRIO               COND_CODE_1(CONFIG_ZERO_LATENCY_IRQS,		\
				  (CONFIG_ZERO_LATENCY_LEVELS), (0))
#define _IRQ_PRIO_OFFSET            (_EXCEPTION_RESERVED_PRIO + _EXC_SVC_PRIO)
#define IRQ_PRIO_LOWEST             (BIT(NUM_IRQ_PRIO_BITS) - (_IRQ_PRIO_OFFSET) - 1)

#define _EXC_IRQ_DEFAULT_PRIO Z_EXC_PRIO(_IRQ_PRIO_OFFSET)

/* Use lowest possible priority level for PendSV */
#define _EXC_PENDSV_PRIO      0xff
#define _EXC_PENDSV_PRIO_MASK Z_EXC_PRIO(_EXC_PENDSV_PRIO)

#ifdef _ASMLANGUAGE

#define _SCS_BASE_ADDR _PPB_INT_SCS

/* ICSR defines */
#define _SCS_ICSR           (_SCS_BASE_ADDR + 0xd04)
#define _SCS_ICSR_PENDSV    (1 << 28)
#define _SCS_ICSR_UNPENDSV  (1 << 27)
#define _SCS_ICSR_RETTOBASE (1 << 11)

#define _SCS_MPU_CTRL (_SCS_BASE_ADDR + 0xd94)

/* CONTROL defines */
#define _CONTROL_FPCA_Msk (1 << 2)

/* EXC_RETURN defines */
#define _EXC_RETURN_SPSEL_Msk (1 << 2)
#define _EXC_RETURN_FTYPE_Msk (1 << 4)

/*
 * Cortex-M Exception Stack Frame Layouts
 *
 * When an exception is taken, the processor automatically pushes
 * registers to the current stack. The layout depends on whether
 * the FPU is active.
 */

/* Basic hardware-saved exception stack frame (no FPU context):
 *	R0-R3		(4 x 4B = 16B)
 *	R12		(4B)
 *	LR		(4B)
 *	Return address	(4B)
 *	RETPSR		(4B)
 *--------------------------
 *      Total: 32 bytes
 */
#define _EXC_HW_SAVED_BASIC_SF_SIZE		(32)
#define _EXC_HW_SAVED_BASIC_SF_RETADDR_OFFSET	(24)
#define _EXC_HW_SAVED_BASIC_SF_XPSR_OFFSET	(28)

/* Extended hardware saved stack frame consists of:
 *	R0-R3		(16B)
 *	R12		(4B)
 *	LR (R14)	(4B)
 *	Return address	(4B)
 *	RETPSR		(4B)
 *	S0-S15		(16 x 4B = 64B)
 *	FPSCR		(4B)
 *	Reserved	(4B)
 *--------------------------
 *      Total: 104 bytes
 */
#define _EXC_HW_SAVED_EXTENDED_SF_SIZE	(104)

#else
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CP10 Access Bits */
#define CPACR_CP10_Pos         20U
#define CPACR_CP10_Msk         (3UL << CPACR_CP10_Pos)
#define CPACR_CP10_NO_ACCESS   (0UL << CPACR_CP10_Pos)
#define CPACR_CP10_PRIV_ACCESS (1UL << CPACR_CP10_Pos)
#define CPACR_CP10_RESERVED    (2UL << CPACR_CP10_Pos)
#define CPACR_CP10_FULL_ACCESS (3UL << CPACR_CP10_Pos)

/* CP11 Access Bits */
#define CPACR_CP11_Pos         22U
#define CPACR_CP11_Msk         (3UL << CPACR_CP11_Pos)
#define CPACR_CP11_NO_ACCESS   (0UL << CPACR_CP11_Pos)
#define CPACR_CP11_PRIV_ACCESS (1UL << CPACR_CP11_Pos)
#define CPACR_CP11_RESERVED    (2UL << CPACR_CP11_Pos)
#define CPACR_CP11_FULL_ACCESS (3UL << CPACR_CP11_Pos)

#ifdef CONFIG_PM_S2RAM

struct __cpu_context {
	/* GPRs are saved onto the stack */
	uint32_t msp;
	uint32_t psp;
	uint32_t primask;
	uint32_t control;

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* Registers present only on ARMv7-M and ARMv8-M Mainline */
	uint32_t faultmask;
	uint32_t basepri;
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	/* Registers present only on certain ARMv8-M implementations */
	uint32_t msplim;
	uint32_t psplim;
#endif /* CONFIG_CPU_CORTEX_M_HAS_SPLIM */
};

typedef struct __cpu_context _cpu_context_t;

#endif /* CONFIG_PM_S2RAM */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_CPU_H_ */
