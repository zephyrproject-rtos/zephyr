/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/arch.h>
#include <zephyr/arch/hexagon/exception.h>
#include <zephyr/logging/log.h>
#include <zephyr/fatal.h>
#include <kswap.h>
#include <hexagon_vm.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* Get current exception stack frame */
static void z_hexagon_get_current_esf(struct arch_esf *esf)
{
	uint32_t g_val;

	memset(esf, 0, sizeof(*esf));

	__asm__ volatile("%0 = g0" : "=r"(g_val));
	esf->event_info[0] = g_val;
	__asm__ volatile("%0 = g1" : "=r"(g_val));
	esf->event_info[1] = g_val;
	__asm__ volatile("%0 = g2" : "=r"(g_val));
	esf->event_info[2] = g_val;
	__asm__ volatile("%0 = g3" : "=r"(g_val));
	esf->event_info[3] = g_val;
}

/*
 * Unlike the trap-based ARCH_EXCEPT() of other arches, ours is a plain
 * noreturn call, so the compiler drops whatever code follows k_panic()
 * in the caller. z_fatal_error() does return when the faulting thread
 * was already dead before the panic -- e.g. an essential thread
 * aborting itself, where z_thread_halt() marks _current dead and then
 * calls k_panic(), relying on the swap after it that we never reach.
 * Swap away from the dead thread here instead of falling off the end
 * of a noreturn function.
 */
static FUNC_NORETURN void z_hexagon_fatal_error_return(unsigned int reason)
{
	if (!arch_is_in_isr() && z_is_thread_dead(_current)) {
		z_swap_unlocked();
	}

	k_fatal_halt(reason);
}

FUNC_NORETURN void z_hexagon_fatal_error(unsigned int reason)
{
	struct arch_esf esf = {0};

	z_hexagon_get_current_esf(&esf);
	z_fatal_error(reason, &esf);

	z_hexagon_fatal_error_return(reason);
}

#ifndef CONFIG_USERSPACE
FUNC_NORETURN void arch_syscall_oops(void *ssf)
{
	ARG_UNUSED(ssf);
	z_hexagon_fatal_error(K_ERR_KERNEL_OOPS);
	CODE_UNREACHABLE;
}
#endif

FUNC_NORETURN void z_do_kernel_oops(const struct arch_esf *esf)
{
	/* Extract reason from r0 in the ESF, following Zephyr convention */
	unsigned int reason = esf->r0;

	z_fatal_error(reason, esf);

	CODE_UNREACHABLE;
}

FUNC_NORETURN void z_arch_except(unsigned int reason)
{
	struct arch_esf esf;

	z_hexagon_get_current_esf(&esf);
	z_fatal_error(reason, &esf);

	z_hexagon_fatal_error_return(reason);
}

FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);
	hexagon_vm_stop(VM_STOP_HALT);

	while (1) {
	}
}
