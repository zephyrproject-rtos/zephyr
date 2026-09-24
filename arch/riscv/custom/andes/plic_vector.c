/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/irq.h>

extern void _isr_wrapper(void);

/*
 * mtvec[0] handles exceptions and local interrupts, which need no
 * special mcause handling, so route it straight to the architecture
 * ISR entry point.
 */
Z_ISR_DECLARE_DIRECT(0, ISR_FLAG_DIRECT, _isr_wrapper);
