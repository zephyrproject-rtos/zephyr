/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef NATIVE_SIMULATOR_NATIVE_SRC_NSI_CPU0_INTERRUPTS_H
#define NATIVE_SIMULATOR_NATIVE_SRC_NSI_CPU0_INTERRUPTS_H

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


#endif /* NATIVE_SIMULATOR_NATIVE_SRC_NSI_CPU0_INTERRUPTS_H */
