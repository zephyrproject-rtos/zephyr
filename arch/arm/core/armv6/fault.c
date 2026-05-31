/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ARMv6 (ARM1176 / VMSAv6) fault handlers. Decode the data/prefetch abort
 * status (DFSR/IFSR) and fault address (DFAR/IFAR) for diagnostics, then
 * route to the architecture fatal-error handler, which dumps the exception
 * frame. The CMSIS-Core(A) DBGDSCR / ARMv7 FSR helpers used by the Cortex-A/R
 * fault.c are not available on ARM1176, so the VMSAv6 encodings are decoded
 * directly here.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/exception.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* VMSAv6 DFSR/IFSR status field FS[3:0] (ARM1176 TRM, Fault Status Register). */
static const char *armv6_fsr_str(uint32_t fsr)
{
	switch (fsr & 0xfU) {
	case 0x1:
		return "alignment fault";
	case 0x4:
		return "instruction cache maintenance fault";
	case 0x5:
		return "translation fault (section)";
	case 0x7:
		return "translation fault (page)";
	case 0x8:
		return "external abort";
	case 0x9:
		return "domain fault (section)";
	case 0xb:
		return "domain fault (page)";
	case 0xd:
		return "permission fault (section)";
	case 0xf:
		return "permission fault (page)";
	default:
		return "unknown fault";
	}
}

void z_arm_fault_undef_instruction(struct arch_esf *esf)
{
	EXCEPTION_DUMP("***** UNDEFINED INSTRUCTION ABORT *****");
	z_arm_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}

void z_arm_fault_prefetch(struct arch_esf *esf)
{
	uint32_t ifsr, ifar;

	__asm__ volatile("mrc p15, 0, %0, c5, c0, 1" : "=r"(ifsr)); /* IFSR */
	__asm__ volatile("mrc p15, 0, %0, c6, c0, 2" : "=r"(ifar)); /* IFAR */

	EXCEPTION_DUMP("***** PREFETCH ABORT *****");
	EXCEPTION_DUMP("%s (IFSR 0x%08x, IFAR 0x%08x)", armv6_fsr_str(ifsr), ifsr, ifar);
	z_arm_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}

void z_arm_fault_data(struct arch_esf *esf)
{
	uint32_t dfsr, dfar;

	__asm__ volatile("mrc p15, 0, %0, c5, c0, 0" : "=r"(dfsr)); /* DFSR */
	__asm__ volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(dfar)); /* DFAR */

	EXCEPTION_DUMP("***** DATA ABORT *****");
	EXCEPTION_DUMP("%s on %s (DFSR 0x%08x, DFAR 0x%08x)", armv6_fsr_str(dfsr),
		       (dfsr & BIT(11)) ? "write" : "read", dfsr, dfar);
	z_arm_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}

void z_arm_fault_init(void)
{
}
