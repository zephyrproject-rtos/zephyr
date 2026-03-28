/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2020 Oticon A/S
 * Copyright (c) 2009-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This header defines replacements for inline
 * ARM Cortex-M CMSIS intrinsics.
 */

#ifndef BOARDS_POSIX_NRF52_BSIM_CMSIS_INSTR_H
#define BOARDS_POSIX_NRF52_BSIM_CMSIS_INSTR_H

/* Implement the following ARM intrinsics as no-op:
 * - ARM Data Synchronization Barrier
 * - ARM Data Memory Synchronization Barrier
 * - ARM Instruction Synchronization Barrier
 * - ARM No Operation
 */
#ifndef __DMB
#define __DMB()
#endif

#ifndef __DSB
#define __DSB()
#endif

#ifndef __ISB
#define __ISB()
#endif

#ifndef __NOP
#define __NOP()
#endif

void __WFE(void);
void __WFI(void);
void __SEV(void);

uint32_t __STREXB(uint8_t value, volatile uint8_t *ptr);
uint32_t __STREXH(uint16_t value, volatile uint16_t *ptr);
uint32_t __STREXW(uint32_t value, volatile uint32_t *ptr);
uint8_t __LDREXB(volatile uint8_t *ptr);
uint16_t __LDREXH(volatile uint16_t *ptr);
uint32_t __LDREXW(volatile uint32_t *ptr);
void __CLREX(void);

/**
 * \brief Model of an ARM CLZ instruction
 */
static inline unsigned char __CLZ(uint32_t value)
{
	if (value == 0) {
		return 32;
	} else {
		return __builtin_clz(value);
	}
}

#endif /* BOARDS_POSIX_NRF52_BSIM_CMSIS_INSTR_H */
