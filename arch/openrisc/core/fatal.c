/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

FUNC_NORETURN void z_openrisc_fatal_error(unsigned int reason,
					  const struct arch_esf *esf)
{
#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		LOG_ERR("epcr: 0x%08x esr: 0x%08x", esf->epcr, esf->esr);
		LOG_ERR("  r3: 0x%08x  r4: 0x%08x  r5: 0x%08x  r6: 0x%08x",
			esf->r3, esf->r4, esf->r5, esf->r6);
		LOG_ERR("  r7: 0x%08x  r8: 0x%08x",
			esf->r7, esf->r8);
		LOG_ERR(" r11: 0x%08x r12: 0x%08x",
			esf->r11, esf->r12);
		LOG_ERR(" r13: 0x%08x r15: 0x%08x r17: 0x%08x r19: 0x%08x",
			esf->r13, esf->r15, esf->r17, esf->r19);
		LOG_ERR(" r21: 0x%08x r23: 0x%08x r25: 0x%08x r27: 0x%08x",
			esf->r21, esf->r23, esf->r25, esf->r27);
		LOG_ERR(" r29: 0x%08x r31: 0x%08x",
			esf->r29, esf->r31);
	}
#endif /* CONFIG_EXCEPTION_DEBUG */
	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

static char *reason_str(unsigned int reason)
{
	switch (reason) {
	case 0x2:
		return "Bus Error";
	case 0x3:
		return "Data Page Fault";
	case 0x4:
		return "Instruction Page Fault";
	case 0x5:
		return "Tick Timer";
	case 0x6:
		return "Alignment Exception";
	case 0x7:
		return "Illegal Instruction";
	case 0x8:
		return "External Interrupt";
	case 0x9:
		return "D-TLB Miss";
	case 0xA:
		return "I-TLB Miss";
	case 0xB:
		return "Range Exception";
	case 0xC:
		return "Syscall";
	case 0xD:
		return "Floating Point Exception";
	case 0xE:
		return "Trap";
	default:
		return "unknown";
	}
}

void z_openrisc_fault(struct arch_esf *esf, unsigned int reason)
{
	LOG_ERR("");
	LOG_ERR(" reason: %d, %s", reason, reason_str(reason));

	z_openrisc_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}
