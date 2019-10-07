/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

#include <kernel_internal.h>
#include <kernel_structs.h>
#include <sys/__assert.h>
#include <arch/cpu.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
#include <fatal.h>

LOG_MODULE_DECLARE(os);

/* LCOV_EXCL_START */
FUNC_NORETURN __weak void z_arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);

	/* TODO: What's the best way to totally halt the system if SMP
	 * is enabled?
	 */

	(void)z_arch_irq_lock();
	for (;;) {
		/* Spin endlessly */
	}
}
/* LCOV_EXCL_STOP */

/* LCOV_EXCL_START */
__weak void k_sys_fatal_error_handler(unsigned int reason,
				      const z_arch_esf_t *esf)
{
	ARG_UNUSED(esf);

	LOG_PANIC();
	LOG_ERR("Halting system");
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

/* LCOV_EXCL_START */
FUNC_NORETURN void k_fatal_halt(unsigned int reason)
{
	z_arch_system_halt(reason);
}
/* LCOV_EXCL_STOP */

void z_fatal_error(unsigned int reason, const z_arch_esf_t *esf)
{
	struct k_thread *thread = k_current_get();

	/* sanitycheck looks for the "ZEPHYR FATAL ERROR" string, don't
	 * change it without also updating sanitycheck
	 */
	LOG_ERR(">>> ZEPHYR FATAL ERROR %d: %s", reason, reason_to_str(reason));

	/* FIXME: This doesn't seem to work as expected on all arches.
	 * Need a reliable way to determine whether the fault happened when
	 * an IRQ or exception was being handled, or thread context.
	 *
	 * See #17656
	 */
#if defined(CONFIG_ARCH_HAS_NESTED_EXCEPTION_DETECTION)
	if (z_arch_is_in_nested_exception(esf)) {
		LOG_ERR("Fault during interrupt handling\n");
	}
#endif

	LOG_ERR("Current thread: %p (%s)", thread,
		log_strdup(thread_name_get(thread)));

	k_sys_fatal_error_handler(reason, esf);

	/* If the system fatal error handler returns, then kill the faulting
	 * thread; a policy decision was made not to hang the system.
	 *
	 * Policy for fatal errors in ISRs: unconditionally panic.
	 *
	 * There is one exception to this policy: a stack sentinel
	 * check may be performed (on behalf of the current thread)
	 * during ISR exit, but in this case the thread should be
	 * aborted.
	 *
	 * Note that k_thread_abort() returns on some architectures but
	 * not others; e.g. on ARC, x86_64, Xtensa with ASM2, ARM
	 */
	if (!IS_ENABLED(CONFIG_TEST)) {
		__ASSERT(reason != K_ERR_KERNEL_PANIC,
			 "Attempted to recover from a kernel panic condition");
		/* FIXME: #17656 */
#if defined(CONFIG_ARCH_HAS_NESTED_EXCEPTION_DETECTION)
		if (z_arch_is_in_nested_exception(esf)) {
#if defined(CONFIG_STACK_SENTINEL)
			if (reason != K_ERR_STACK_CHK_FAIL) {
				__ASSERT(0,
				 "Attempted to recover from a fatal error in ISR");
			 }
#endif /* CONFIG_STACK_SENTINEL */
		}
#endif /* CONFIG_ARCH_HAS_NESTED_EXCEPTION_DETECTION */
	} else {
		/* Test mode */
#if defined(CONFIG_ARCH_HAS_NESTED_EXCEPTION_DETECTION)
			if (z_arch_is_in_nested_exception(esf)) {
				/* Abort the thread only on STACK Sentinel check fail. */
#if defined(CONFIG_STACK_SENTINEL)
				if (reason != K_ERR_STACK_CHK_FAIL) {
					return;
				}
#else
				return;
#endif /* CONFIG_STACK_SENTINEL */
			}
#endif /*CONFIG_ARCH_HAS_NESTED_EXCEPTION_DETECTION */
	}

	k_thread_abort(thread);
}
