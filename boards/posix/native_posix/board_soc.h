/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file Extra definitions provided by the board to soc.h
 *
 * Background:
 * The POSIC ARCH/SOC/board layering is different than in normal archs
 * The "SOC" does not provide almost any of the typical SOC functionality
 * but that is left for the "board" to define it
 * Device code may rely on the soc.h defining some things (like the interrupts
 * numbers)
 * Therefore this file is included from the inf_clock soc.h to allow a board
 * to define that kind of SOC related snippets
 */

#ifndef _POSIX_SP_BOARD_SOC_H
#define _POSIX_SP_BOARD_SOC_H

#ifdef __cplusplus
extern "C" {
#endif

#define TIMER_TICK_IRQ 0
#define OFFLOAD_SW_IRQ 1
#define COUNTER_EVENT_IRQ 2

/*
 * This interrupt will awake the CPU if IRQs are not locked,
 * This interrupt does not have an associated status bit or handler
 */
#define PHONY_WEAK_IRQ 0xFFFE
/*
 * This interrupt will awake the CPU even if IRQs are locked,
 * This interrupt does not have an associated status bit or handler
 * (the lock is only ignored when the interrupt is raised from the HW models,
 * SW threads should not try to use this)
 */
#define PHONY_HARD_IRQ 0xFFFF


#ifdef __cplusplus
}
#endif

#endif /* _POSIX_SP_BOARD_SOC_H */
