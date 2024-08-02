/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fatal fault handling
 *
 * This module implements the routines necessary for handling fatal faults on
 * ARCv2 CPUs.
 */

#include <zephyr/kernel.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/logging/log.h>
#include <kernel_arch_data.h>
#include <zephyr/arch/arc/v2/exception.h>
#include <err_dump_handling.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_EXCEPTION_DEBUG
static void dump_arc_esf(const struct arch_esf *esf)
{
	ARC_EXCEPTION_DUMP(" r0: 0x%" PRIxPTR "  r1: 0x%" PRIxPTR "  r2: 0x%" PRIxPTR
		"  r3: 0x%" PRIxPTR "", esf->r0, esf->r1, esf->r2, esf->r3);
	ARC_EXCEPTION_DUMP(" r4: 0x%" PRIxPTR "  r5: 0x%" PRIxPTR "  r6: 0x%" PRIxPTR
		"  r7: 0x%" PRIxPTR "", esf->r4, esf->r5, esf->r6, esf->r7);
	ARC_EXCEPTION_DUMP(" r8: 0x%" PRIxPTR "  r9: 0x%" PRIxPTR " r10: 0x%" PRIxPTR
		" r11: 0x%" PRIxPTR "", esf->r8, esf->r9, esf->r10, esf->r11);
	ARC_EXCEPTION_DUMP("r12: 0x%" PRIxPTR " r13: 0x%" PRIxPTR "  pc: 0x%" PRIxPTR "",
		esf->r12, esf->r13, esf->pc);
	ARC_EXCEPTION_DUMP(" blink: 0x%" PRIxPTR " status32: 0x%" PRIxPTR "",
		esf->blink, esf->status32);
#ifdef CONFIG_ARC_HAS_ZOL
	ARC_EXCEPTION_DUMP("lp_end: 0x%" PRIxPTR " lp_start: 0x%" PRIxPTR
			" lp_count: 0x%" PRIxPTR "", esf->lp_end, esf->lp_start, esf->lp_count);
#endif /* CONFIG_ARC_HAS_ZOL */
}
#endif

void z_arc_fatal_error(unsigned int reason, const struct arch_esf *esf)
{
#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		dump_arc_esf(esf);
	}
#endif /* CONFIG_EXCEPTION_DEBUG */

	z_fatal_error(reason, esf);
}

FUNC_NORETURN void arch_syscall_oops(void *ssf_ptr)
{
	/* TODO: convert ssf_ptr contents into an esf, they are not the same */
	ARG_UNUSED(ssf_ptr);

	z_arc_fatal_error(K_ERR_KERNEL_OOPS, NULL);
	CODE_UNREACHABLE;
}

FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);

	__asm__("brk");

	CODE_UNREACHABLE;
}
