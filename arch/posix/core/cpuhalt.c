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
 * arch_cpu_idle() primitive required by the kernel idle loop component.
 * It can be called within an implementation of _pm_save_idle(),
 * which is provided for the kernel by the platform.
 *
 * An implementation of arch_cpu_atomic_idle(), which
 * atomically re-enables interrupts and enters low power mode.
 *
 * A weak stub for sys_arch_reboot(), which does nothing
 */

#include "posix_core.h"
#include "posix_board_if.h"
#include <zephyr/arch/posix/posix_soc_if.h>
#include <zephyr/tracing/tracing.h>

#if !defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
#error "The POSIX architecture needs a custom busy_wait implementation. \
CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT must be selected"
/* Each POSIX arch board (or SOC) must provide an implementation of
 * arch_busy_wait()
 */
#endif

void arch_cpu_idle(void)
{
	sys_trace_idle();
	posix_irq_full_unlock();
	posix_halt_cpu();
}

void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	posix_atomic_halt_cpu(key);
}

#if defined(CONFIG_REBOOT)
/**
 * @brief Stub for sys_arch_reboot
 *
 * Does nothing
 */
void __weak sys_arch_reboot(int type)
{
	posix_print_warning("%s called with type %d. Exiting\n",
						__func__, type);
	posix_exit(1);
}
#endif /* CONFIG_REBOOT */
