/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CORTEX_M_CPU_H
#define _CORTEX_M_CPU_H

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

#endif /* _CORTEX_M_CPU_H */
