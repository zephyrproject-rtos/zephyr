/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv6 default reboot stub.
 *
 * ARM1176 has no architectural reset path; rebooting is platform-specific
 * (watchdog, PMC, or a VideoCore mailbox call on the BCM2835 family).
 * Provide a __weak no-op so callers of sys_reboot() link cleanly, and SoC
 * code can override with a real reset sequence where one exists.
 *
 * The vector table is copied to 0x0 by reset.S (ARMv6 low vectors,
 * SCTLR.V=0), so no separate relocate_vector_table() is needed here.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}
