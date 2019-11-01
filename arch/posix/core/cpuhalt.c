/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file  CPU power management code for POSIX
 *
 * This module provides:
 *
 * An implementation of the architecture-specific
 * z_arch_cpu_idle() primitive required by the kernel idle loop component.
 * It can be called within an implementation of _sys_power_save_idle(),
 * which is provided for the kernel by the platform.
 *
 * An implementation of z_arch_cpu_atomic_idle(), which
 * atomically re-enables interrupts and enters low power mode.
 *
 * A weak stub for sys_arch_reboot(), which does nothing
 */

#include "posix_core.h"
#include "posix_soc_if.h"
#include <debug/tracing.h>

void z_arch_cpu_idle(void)
{
	sys_trace_idle();
	posix_irq_full_unlock();
	posix_halt_cpu();
}

void z_arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	posix_atomic_halt_cpu(key);
}

#if defined(CONFIG_REBOOT)
/**
 *
 * @brief Stub for sys_arch_reboot
 *
 * Does nothing
 *
 * @return N/A
 */
void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}
#endif /* CONFIG_REBOOT */
