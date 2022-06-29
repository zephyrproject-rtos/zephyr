/*
 * Copyright (c) 2020 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This header defines replacements for inline
 * ARM Cortex-M CMSIS intrinsics.
 */

#ifndef BOARDS_POSIX_NRF52_BSIM_CMSIS_H
#define BOARDS_POSIX_NRF52_BSIM_CMSIS_H

#ifdef __cplusplus
extern "C" {
#endif

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

void __CLREX(void);

uint32_t __STREXB(uint8_t value, volatile uint8_t *ptr);

uint32_t __LDREXB(volatile uint8_t *ptr);

uint32_t __LDREXW(volatile uint32_t *ptr);

uint32_t __STREXW(uint32_t value, volatile uint32_t *ptr);

uint16_t __LDREXH(volatile uint16_t *ptr);

uint32_t __STREXH(uint16_t value, volatile uint16_t *ptr);

void __enable_irq(void);

void __disable_irq(void);

uint32_t __get_PRIMASK(void);

void __set_PRIMASK(uint32_t primask);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_NRF52_BSIM_CMSIS_H */
