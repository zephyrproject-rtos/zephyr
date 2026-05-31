/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal ARMv6 (ARM1176) fault handlers for first bring-up.
 *
 * The Cortex-A/R fault.c decodes the ARMv7 DFSR/IFSR fault-status and the
 * DBGDSCR debug registers via CMSIS-Core(A) definitions that ARM1176 does
 * not have. For now any CPU fault is treated as fatal; full VMSAv6
 * DFSR/IFSR decode is a later step.
 *
 * TODO(armv6): decode VMSAv6 DFSR (c5) / IFSR and FAR/IFAR (c6).
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/exception.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* No FPU support yet, so an undefined instruction is never a lazy-FP trap. */
bool z_arm_fault_undef_instruction_fp(void)
{
	return false;
}

bool z_arm_fault_undef_instruction(struct arch_esf *esf)
{
	z_arm_fatal_error(K_ERR_CPU_EXCEPTION, esf);
	return true;
}

bool z_arm_fault_prefetch(struct arch_esf *esf)
{
	z_arm_fatal_error(K_ERR_CPU_EXCEPTION, esf);
	return true;
}

bool z_arm_fault_data(struct arch_esf *esf)
{
	z_arm_fatal_error(K_ERR_CPU_EXCEPTION, esf);
	return true;
}

void z_arm_fault_init(void)
{
}
