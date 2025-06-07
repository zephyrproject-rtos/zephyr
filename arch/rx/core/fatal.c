/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fatal fault handling
 *
 * This module implements the routines necessary for handling fatal faults on
 * RX CPUs.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <kernel_arch_data.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_EXCEPTION_DEBUG
static void dump_rx_esf(const struct arch_esf *esf)
{
	LOG_ERR(" ACC_L: 0x%08x  ACC_H:  0x%08x", esf->acc_l, esf->acc_h);
	LOG_ERR(" r1:    0x%08x  r2:     0x%08x  r3:     0x%08x", esf->r1, esf->r2, esf->r3);
	LOG_ERR(" r4:    0x%08x  r5:     0x%08x  r6:     0x%08x", esf->r4, esf->r5, esf->r6);
	LOG_ERR(" r7:    0x%08x  r8:     0x%08x  r9:     0x%08x", esf->r7, esf->r8, esf->r9);
	LOG_ERR(" r10:   0x%08x  r11:    0x%08x  r12:    0x%08x", esf->r10, esf->r11, esf->r12);
	LOG_ERR(" r13:   0x%08x  r14:    0x%08x  r15:    0x%08x", esf->r13, esf->r14, esf->r15);
	LOG_ERR(" PC:    0x%08x  PSW:    0x%08x", esf->entry_point, esf->psw);
}
#endif

void z_rx_fatal_error(unsigned int reason, const struct arch_esf *esf)
{
#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		dump_rx_esf(esf);
	}
#endif /* CONFIG_EXCEPTION_DEBUG */

	z_fatal_error(reason, esf);
}
FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);

	__asm__("brk");

	CODE_UNREACHABLE;
}
