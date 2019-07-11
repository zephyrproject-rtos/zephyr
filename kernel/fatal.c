/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <arch/cpu.h>
#include <logging/log_ctrl.h>
#include <fatal.h>

/* LCOV_EXCL_START */
FUNC_NORETURN __weak void z_arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);

	/* TODO: What's the best way to totally halt the system if SMP
	 * is enabled?
	 */

	(void)z_arch_irq_lock();
	for (;;) {
		k_cpu_idle();
	}
}
/* LCOV_EXCL_STOP */

/* LCOV_EXCL_START */
__weak void k_sys_fatal_error_handler(unsigned int reason,
				      const NANO_ESF *esf)
{
	ARG_UNUSED(esf);

	LOG_PANIC();

	printk("Halting system.\n");
	z_arch_system_halt(reason);
	CODE_UNREACHABLE;
}
/* LCOV_EXCL_STOP */

static const char *thread_name_get(struct k_thread *thread)
{
	const char *thread_name = k_thread_name_get(thread);

	if (thread_name == NULL || thread_name[0] == '\0') {
		thread_name = "unknown";
	}

	return thread_name;
}

static const char *reason_to_str(unsigned int reason)
{
	switch (reason) {
	case K_ERR_CPU_EXCEPTION:
		return "CPU exception";
	case K_ERR_SPURIOUS_IRQ:
		return "Unhandled interrupt";
	case K_ERR_STACK_CHK_FAIL:
		return "Stack overflow";
	case K_ERR_KERNEL_OOPS:
		return "Kernel oops";
	case K_ERR_KERNEL_PANIC:
		return "Kernel panic";
	default:
		return "Unknown error";
	}
}

void z_fatal_error(unsigned int reason, const NANO_ESF *esf)
{
	struct k_thread *thread = k_current_get();

	/* TODO: Replace all printk()s here and in arch error handling code
	 * to some special printk_fatal() function, which enables panic
	 * mode and routes messages to printk or LOG subsystem appropriately
	 * based on configuration.
	 */

#ifdef CONFIG_PRINTK
	printk(">>> ZEPHYR FATAL ERROR %d: %s\n", reason,
	       reason_to_str(reason));

	/* FIXME: This doesn't seem to work as expected on all arches.
	 * Need a reliable way to determine whether the fault happened when
	 * an IRQ or exception was being handled, or thread context.
	 *
	 * See #17656
	 *
	 * if (k_is_in_isr()) {
	 *     printk("Fault during interrupt handling\n");
	 * }
	 */

	printk("Current thread: %p (%s)\n", thread, thread_name_get(thread));
#endif /* CONFIG_PRINTK */

	k_sys_fatal_error_handler(reason, esf);

	/* If the system fatal error handler returns, then kill the faulting
	 * thread; a policy decision was made not to hang the system.
	 *
	 * Note that k_thread_abort() returns on some architectures but
	 * not others; e.g. on ARC, x86_64, Xtensa with ASM2, ARM
	 */
	if (!IS_ENABLED(CONFIG_TEST)) {
		__ASSERT(reason != K_ERR_KERNEL_PANIC,
			 "Attempted to recover from a kernel panic condition");
		/* FIXME: #17656 */
		__ASSERT(!k_is_in_isr(),
			 "Attempted to recover from a fatal error in ISR");
	}
	k_thread_abort(thread);
}
