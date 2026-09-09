/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>

/* Set in rv_exception_handler */
volatile int _trap_active;

extern void rv_exception_handler(void);

/* Install rv_exception_handler at mtvec[0] */
Z_ISR_DECLARE_DIRECT(0, ISR_FLAG_DIRECT, rv_exception_handler);
