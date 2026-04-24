/*
 * Copyright (c) 2025 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore public exception handling
 *
 * TriCore-specific kernel exception handling interface.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_EXCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TriCore Exception Stack Frame is the lower context*/
struct arch_esf {
	uint32_t pcxi;
	uint32_t a11;
	uint32_t a2;
	uint32_t a3;
	uint32_t d0;
	uint32_t d1;
	uint32_t d2;
	uint32_t d3;
	uint32_t a4;
	uint32_t a5;
	uint32_t a6;
	uint32_t a7;
	uint32_t d4;
	uint32_t d5;
	uint32_t d6;
	uint32_t d7;
};

#define TRICORE_TRAP_MMU                          0
#define TRICORE_TRAP_INTERNAL_PROTECTION_TRAPS    1
#define TRICORE_TRAP_INSTRUCTION_ERRORS           2
#define TRICORE_TRAP_CONTEXT_MANAGEMENT           3
#define TRICORE_TRAP_SYSTEM_BUS_PERIPHERAL_ERRORS 4
#define TRICORE_TRAP_ASSERTION_TRAPS              5
#define TRICORE_TRAP_SYSTEM_CALL                  6
#define TRICORE_TRAP_NMI                          7

/* Trap identification numbers (TIN) */
#define TRICORE_TRAP0_VAF 0
#define TRICORE_TRAP0_VAP 1

#define TRICORE_TRAP1_PRIV 1
#define TRICORE_TRAP1_MPR  2
#define TRICORE_TRAP1_MPW  3
#define TRICORE_TRAP1_MPX  4
#define TRICORE_TRAP1_MPP  5
#define TRICORE_TRAP1_MPN  6
#define TRICORE_TRAP1_GRWP 7

#define TRICORE_TRAP2_IOPC 1
#define TRICORE_TRAP2_UOPC 2
#define TRICORE_TRAP2_OPD  3
#define TRICORE_TRAP2_ALN  4
#define TRICORE_TRAP2_MEM  5
#define TRICORE_TRAP2_CSE  6

#define TRICORE_TRAP3_FCD  1
#define TRICORE_TRAP3_CDO  2
#define TRICORE_TRAP3_CDU  3
#define TRICORE_TRAP3_FCU  4
#define TRICORE_TRAP3_CSU  5
#define TRICORE_TRAP3_CTYP 6
#define TRICORE_TRAP3_NEST 7

#define TRICORE_TRAP4_PSE 1
#define TRICORE_TRAP4_DSE 2
#define TRICORE_TRAP4_DAE 3
#define TRICORE_TRAP4_CAE 4
#define TRICORE_TRAP4_PIE 5
#define TRICORE_TRAP4_DIE 6
#define TRICORE_TRAP4_TAE 7

#define TRICORE_TRAP5_OVF  1
#define TRICORE_TRAP5_SOVF 2

#ifdef __cplusplus
}
#endif
#endif
#endif
