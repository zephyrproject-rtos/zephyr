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

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_NRF52_BSIM_CMSIS_H */
