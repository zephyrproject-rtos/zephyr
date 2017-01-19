/* ARM Cortex-M GCC specific inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ASM_INLINE_GCC_H
#define _ASM_INLINE_GCC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The file must not be included directly
 * Include asm_inline.h instead
 */

#ifndef _ASMLANGUAGE

/**
 *
 * @brief Obtain value of IPSR register
 *
 * Obtain and return current value of IPSR register.
 *
 * @return the contents of the IPSR register
 */
static ALWAYS_INLINE uint32_t _IpsrGet(void)
{
	uint32_t vector;

	__asm__ volatile("mrs %0, IPSR\n\t" : "=r"(vector));
	return vector;
}

/**
 *
 * @brief Set the value of the Main Stack Pointer register
 *
 * Store the value of <msp> in MSP register.
 *
 * @return N/A
 */
static ALWAYS_INLINE void _MspSet(uint32_t msp /* value to store in MSP */
				  )
{
	__asm__ volatile("msr MSP, %0\n\t" :  : "r"(msp));
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ASM_INLINE_GCC_H */
