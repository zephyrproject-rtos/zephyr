/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Deferred work on the return from a system call: a thread cancellation, or a signal.
 *
 * Not compiled with __ZEPHYR_SUPERVISOR__: the trampoline runs in the thread's own mode, so a
 * user thread's handlers, cleanup routines and destructors execute in user mode.
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <kernel_internal.h>

/* work pending somewhere: the generated wrappers test this inline, see syscall.h */
atomic_t z_syscall_work_pending;

#ifdef CONFIG_THREAD_CANCEL
/* consume the pending request; true when the thread is to be cancelled now */
static bool cancel_claim(k_tid_t tid)
{
	if (!atomic_cas(&tid->base.cancel_pending, 1, 0)) {
		return false;
	}
	atomic_dec(&z_syscall_work_pending);
	atomic_set(&tid->base.result, (atomic_val_t)K_THREAD_CANCELLED);

	return true;
}

void z_thread_cancel_handler(int sig, struct k_sig_info *info, void *ucontext)
{
	ARG_UNUSED(sig);
	ARG_UNUSED(info);
	ARG_UNUSED(ucontext);

	/* runs in the cancelled thread's own mode: the request was claimed kernel-side */
	z_thread_cancel_actuate();
	CODE_UNREACHABLE;
}

FUNC_NORETURN void z_thread_cancel_actuate(void)
{
	z_thread_teardown();
}

bool z_thread_cancel_try_actuate(void)
{
	k_tid_t const tid = _current;

	if (!z_thread_cancel_is_enabled(tid)) {
		return false;
	}

#ifdef CONFIG_USERSPACE
	if (k_is_in_user_syscall()) {
		/*
		 * Cleanup handlers and destructors of a user thread run in user mode: arm the
		 * return hook, which redirects the return from this system call into the
		 * trampoline. The request stays pending until then.
		 */
		if (atomic_get(&tid->base.cancel_pending) == 0) {
			return false;
		}
		z_thread_cancel_arm(tid);
		return true;
	}
#endif /* CONFIG_USERSPACE */

	if (!cancel_claim(tid)) {
		return false;
	}

	z_thread_cancel_actuate();
	CODE_UNREACHABLE;
}

void z_syscall_cancel_check_slow(void)
{
	k_tid_t const tid = _current;

	if (k_is_pre_kernel() || k_is_in_isr()) {
		return;
	}

	if (z_thread_cancel_is_async(tid)) {
		return;
	}

	(void)z_thread_cancel_try_actuate();
}

/* an armed request becomes the pending action; a cancellation never returns, so it goes first */
static bool cancel_redirect(k_tid_t tid, struct k_sig_ctx *ctx)
{
	if (atomic_cas(&tid->base.cancel_armed, 1, 0) == 0) {
		return false;
	}

	if (!z_thread_cancel_is_enabled(tid) || !cancel_claim(tid)) {
		return false;
	}

	ctx->info = (struct k_sig_info){.signo = K_SIG_CANCEL};
	ctx->action = (struct k_sig_action){.handler = z_thread_cancel_handler};

	return true;
}
#endif /* CONFIG_THREAD_CANCEL */

void z_sig_trampoline(void)
{
	void *context;
	struct k_sig_info info;
	struct k_sig_action action;

	k_sig_current(&info, &action, &context);

#ifdef CONFIG_SIGNAL
	if ((action.flags & K_SIG_SA_RESETHAND) != 0) {
		struct k_sig_action reset_action = action;

		reset_action.handler = K_SIG_DFL;
		reset_action.flags &= ~K_SIG_SA_SIGINFO;

		(void)k_sig_action(info.signo, &reset_action, NULL);
	}
#endif /* CONFIG_SIGNAL */

	action.handler(info.signo, &info, &context);
	k_sig_return();
}

#ifndef CONFIG_ARCH_HAS_SIGNAL_TRAMPOLINE
void arch_sig_trampoline(void)
{
	z_sig_trampoline();
}
#endif

int z_sig_handle(void *user)
{
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		__ASSERT(!k_is_user_context(),
			 "z_sig_handle() should not be called from userspace");
	}

	if (k_is_pre_kernel() || k_is_in_isr()) {
		return 0;
	}

	k_tid_t const tid = _current;
	bool redirect = false;

#ifdef CONFIG_THREAD_CANCEL
	redirect = cancel_redirect(tid, &tid->base.sig);
#endif
#ifdef CONFIG_SIGNAL
	if (!redirect) {
		redirect = z_sig_claim(tid) != 0;
	}
#endif
	if (!redirect) {
		return 0;
	}

#ifdef CONFIG_USERSPACE
	if (user != NULL) {
		arch_sig_pre(tid, user);
	}
#endif

	return 1;
}

void z_impl_k_sig_current(struct k_sig_info *info, struct k_sig_action *action, void **context)
{
	__ASSERT_NO_MSG(!k_is_pre_kernel());
	__ASSERT_NO_MSG(!k_is_in_isr());

	struct k_sig_ctx *const ctx = &_current->base.sig;

	*info = ctx->info;
	*action = ctx->action;
	*context = NULL;
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_sig_current(struct k_sig_info *info, struct k_sig_action *action,
					void **context)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(info, sizeof(*info)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(action, sizeof(*action)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(context, sizeof(*context)));

	z_impl_k_sig_current(info, action, context);
}
#include <zephyr/syscalls/k_sig_current_mrsh.c>
#endif

void z_impl_k_sig_return(void)
{
#ifdef CONFIG_USERSPACE
	if (k_is_in_user_syscall()) {
		arch_sig_post(_current, _current->syscall_frame);
	}
#endif

#ifdef CONFIG_SIGNAL
	_current->base.sig.mask = _current->base.sig.restore;
#endif
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_sig_return(void)
{
	z_impl_k_sig_return();
}
#include <zephyr/syscalls/k_sig_return_mrsh.c>
#endif

void z_syscall_sig_check_slow(void)
{
#ifdef CONFIG_USERSPACE
	if (k_is_in_user_syscall()) {
		/*
		 * Reached from a syscall wrapper invoked in kernel context while a user
		 * syscall is in progress (e.g. k_kill() calling k_sig_queue()). Do not act
		 * here: that would run k_sig_return() -> arch_sig_post() against the outer
		 * syscall's frame with no matching arch_sig_pre(). The arch userspace-return
		 * path acts instead.
		 */
		return;
	}
#endif /* CONFIG_USERSPACE */

	k_tid_t const tid = _current;
	bool pending = false;

#ifdef CONFIG_THREAD_CANCEL
	pending = atomic_get(&tid->base.cancel_armed) != 0;
#endif
#ifdef CONFIG_SIGNAL
	pending = pending || z_sig_pending_any(tid);
#endif
	if (pending) {
		while (z_sig_handle(NULL)) {
			z_sig_trampoline();
		}
	}
}
