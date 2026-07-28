/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/kernel/signal.h>
#include <zephyr/sys/thread.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/elastipool.h>
#include <zephyr/sys/internal/kobject_internal.h>
#include <zephyr/sys/timer.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(sys_timer, CONFIG_KERNEL_LOG_LEVEL);

/* For declaring an array of statically-initialized k_timer objects */
#define K_TIMER_ARRAY_DEFINE_ELEM(idx, name) \
	Z_TIMER_INITIALIZER_EXT(name[idx], NULL, NULL, SYS_CLOCK_MONOTONIC)
#define K_TIMER_ARRAY_DEFINE(name, num)                                                            \
	__maybe_unused struct k_timer name[] = {                                                   \
		LISTIFY(num, K_TIMER_ARRAY_DEFINE_ELEM, (,), name) }

static void *timer_pool_heap_alloc(const void *ptr, size_t size, size_t align, uint32_t flags)
{
	ARG_UNUSED(align);
	ARG_UNUSED(flags);

	if (size == 0) {
		/* deallocate */
#ifdef CONFIG_USERSPACE
		if (k_object_find((void *)ptr) != NULL) {
			k_object_free((void *)ptr);
			return NULL;
		}
#endif /* CONFIG_USERSPACE */
#if defined(K_HEAP_MEM_POOL_SIZE) && (K_HEAP_MEM_POOL_SIZE > 0)
		k_free((void *)ptr);
#endif
		return NULL;
	}

	if (IS_ENABLED(CONFIG_DYNAMIC_OBJECTS)) {
		void *timer = k_object_alloc(K_OBJ_TIMER);

#ifdef CONFIG_DYNAMIC_OBJECTS
		if (timer != NULL) {
			/* the pool owns the lifetime: opt out of perms-refcount disposal */
			k_object_find(timer)->flags &= ~K_OBJ_FLAG_ALLOC;
		}
#endif /* CONFIG_DYNAMIC_OBJECTS */

		return timer;
	}

	if (IS_ENABLED(CONFIG_USERSPACE) && k_is_user_context()) {
		LOG_WRN_ONCE("CONFIG_DYNAMIC_OBJECTS is not enabled");
		return NULL;
	}

#if defined(K_HEAP_MEM_POOL_SIZE) && (K_HEAP_MEM_POOL_SIZE > 0)
	return k_aligned_alloc(__alignof(struct k_timer), sizeof(struct k_timer));
#else
	return NULL;
#endif
}

BUILD_ASSERT(SYS_TIMER_MIN >= 0);
BUILD_ASSERT(CONFIG_SYS_TIMER_MAX >= SYS_TIMER_MIN);

static struct k_spinlock timer_pool_lock;
K_TIMER_ARRAY_DEFINE(sys_timer_pool, SYS_TIMER_MIN);
SYS_ELASTIPOOL_DEFINE_ADVANCED(timer_pool, sizeof(struct k_timer), __alignof(struct k_timer),
			       SYS_TIMER_MIN, CONFIG_SYS_TIMER_MAX, timer_pool_heap_alloc,
			       sys_timer_pool, static);

#if defined(CONFIG_TIMER_SIGNAL) && defined(CONFIG_SYS_THREAD) && defined(CONFIG_THREAD_DETACH)
/*
 * Function-notification dispatcher: a single kernel service thread accepts every
 * function-notify timer's expiry signal (K_SIG_SYS_TIMER) and spawns one detached
 * thread per notification. Notification threads for timers created by user threads
 * run in user mode, in the creator's memory domain, inheriting the creator's object
 * permissions at dispatch time.
 */
static struct k_thread sys_timer_dispatch_thread;
static K_KERNEL_STACK_DEFINE(sys_timer_dispatch_stack, CONFIG_SYS_TIMER_DISPATCH_STACK_SIZE);
static bool sys_timer_dispatch_running;

static void sys_timer_notify_entry(void *p1, void *p2, void *p3)
{
	void (*const fn)(union k_sig_val) = p1;
	const union k_sig_val value = {
		.sival_ptr = p2,
	};

	ARG_UNUSED(p3);

	fn(value);
}

static void sys_timer_dispatch(void *p1, void *p2, void *p3)
{
	struct k_sig_set set;
	struct k_sig_info info;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)k_sig_emptyset(&set);
	(void)k_sig_addset(&set, K_SIG_SYS_TIMER);
	(void)k_sig_mask(K_SIG_BLOCK, &set, NULL);

	for (;;) {
		int ret;
		int prio = 0;
		size_t stack_size = 0;
		struct k_thread *th = NULL;
		uint32_t options = K_INHERIT_PERMS;
		void (*fn)(union k_sig_val) = NULL;
#ifdef CONFIG_USERSPACE
		k_tid_t perms_src = NULL;
		struct k_mem_domain *domain = NULL;
#endif /* CONFIG_USERSPACE */

		if (k_sig_timedwait(&set, &info, K_FOREVER) < 0) {
			/* defensive: never let a persistent error become a busy spin */
			k_sleep(K_TICKS(1));
			continue;
		}

		K_SPINLOCK(&timer_pool_lock) {
			/*
			 * The generating timer may have been deleted (sys_timer_delete()
			 * purges undelivered instances, but this one was already accepted)
			 * or even recycled; validate before touching it. A drop here is the
			 * POSIX "disposition of pending notifications is unspecified" case.
			 */
			if ((sys_elastipool_check(&timer_pool, info.timer) != 0) ||
			    (info.timer->sig_fn == NULL)) {
				K_SPINLOCK_BREAK;
			}
			fn = info.timer->sig_fn;
			prio = info.timer->sig_fn_prio;
			stack_size = info.timer->sig_fn_stack_size;
#ifdef CONFIG_USERSPACE
			domain = info.timer->sig_fn_domain;
			perms_src = info.timer->sig_fn_perms_src;
#endif /* CONFIG_USERSPACE */
		}
		if (fn == NULL) {
			continue;
		}

#ifdef CONFIG_USERSPACE
		if (domain != NULL) {
			options |= K_USER;
			ret = sys_thread_create_paused(&th, NULL, stack_size, 0,
						       sys_timer_notify_entry, fn,
						       info.value.sival_ptr, NULL, prio, options);
			if (ret == 0) {
				ret = k_mem_domain_add_thread(domain, th);
				if (ret == 0) {
					k_thread_perms_inherit(perms_src, th);
					k_thread_start(th);
				} else {
					k_thread_abort(th);
				}
			}
		} else
#endif /* CONFIG_USERSPACE */
		{
			ret = sys_thread_create(&th, NULL, stack_size, 0, sys_timer_notify_entry,
						fn, info.value.sival_ptr, NULL, prio, options);
		}

		if (ret == 0) {
			(void)k_thread_detach(th);
		} else {
			LOG_DBG_RATELIMIT("function notification dropped for timer %p (%d)",
					  info.timer, ret);
		}
	}
}

static void sys_timer_dispatch_start(void)
{
	K_SPINLOCK(&timer_pool_lock) {
		if (!sys_timer_dispatch_running) {
			(void)k_thread_create(&sys_timer_dispatch_thread, sys_timer_dispatch_stack,
					      K_KERNEL_STACK_SIZEOF(sys_timer_dispatch_stack),
					      sys_timer_dispatch, NULL, NULL, NULL,
					      CONFIG_SYS_TIMER_DISPATCH_PRIO, 0, K_NO_WAIT);
			(void)k_thread_name_set(&sys_timer_dispatch_thread, "sys_timer");
			sys_timer_dispatch_running = true;
		}
	}
}
#endif /* CONFIG_TIMER_SIGNAL && CONFIG_SYS_THREAD && CONFIG_THREAD_DETACH */

int z_impl_sys_timer_create(int clock_id, const struct k_timer_notify *notify, uint32_t flags,
			    const struct timespec *value, const struct timespec *interval,
			    struct k_timer **timer)
{
	int ret;
	struct k_timer *t = NULL;

	if ((timer == NULL) ||
	    ((clock_id != SYS_CLOCK_MONOTONIC) && (clock_id != SYS_CLOCK_REALTIME))) {
		return -EINVAL;
	}

	if (notify != NULL) {
		if (!IS_ENABLED(CONFIG_TIMER_SIGNAL)) {
			/* expiry notification requires CONFIG_TIMER_SIGNAL */
			return -ENOTSUP;
		}
		if (notify->fn != NULL) {
			if (!IS_ENABLED(CONFIG_SYS_THREAD) || !IS_ENABLED(CONFIG_THREAD_DETACH)) {
				/* notification threads are detached system pool threads */
				return -ENOTSUP;
			}
			if ((notify->fn_priority > K_LOWEST_APPLICATION_THREAD_PRIO) ||
			    (notify->fn_priority < K_HIGHEST_APPLICATION_THREAD_PRIO)) {
				return -EINVAL;
			}
		} else if ((notify->target == NULL) || (notify->signo <= 0)) {
			return -EINVAL;
		}
	}

	K_SPINLOCK(&timer_pool_lock) {
		ret = sys_elastipool_alloc(&timer_pool, (void **)&t);
	}
	if (ret < 0) {
		/* POSIX timer_create() reports resource exhaustion as EAGAIN */
		return -EAGAIN;
	}

	k_timer_init(t, NULL, NULL);
	t->clock_id = (uint8_t)clock_id;

#ifdef CONFIG_TIMER_SIGNAL
	t->sig_fn = NULL;
	if (notify != NULL) {
		union k_sig_val notify_value = notify->value;

		if ((flags & SYS_TIMER_SIGVAL_SELF) != 0U) {
			/* the notification value is the timer itself (POSIX evp == NULL) */
			notify_value.sival_ptr = t;
		}
#if defined(CONFIG_SYS_THREAD) && defined(CONFIG_THREAD_DETACH)
		if (notify->fn != NULL) {
			sys_timer_dispatch_start();
			t->sig_fn = notify->fn;
			t->sig_fn_stack_size = notify->fn_stack_size;
			t->sig_fn_prio = notify->fn_priority;
#ifdef CONFIG_USERSPACE
			t->sig_fn_domain = NULL;
			t->sig_fn_perms_src = NULL;
			if (k_is_in_user_syscall()) {
				t->sig_fn_domain = _current->mem_domain_info.mem_domain;
				t->sig_fn_perms_src = _current;
			}
#endif /* CONFIG_USERSPACE */
			z_timer_sig_attach(t, &sys_timer_dispatch_thread, K_SIG_SYS_TIMER,
					   notify_value);
		} else
#endif /* CONFIG_SYS_THREAD && CONFIG_THREAD_DETACH */
		{
			z_timer_sig_attach(t, notify->target, notify->signo, notify_value);
		}
	}
#endif /* CONFIG_TIMER_SIGNAL */

#ifdef CONFIG_USERSPACE
	if (k_is_in_user_syscall()) {
		k_object_access_grant(t, _current);
	}
#endif /* CONFIG_USERSPACE */

	if (value != NULL) {
		/* arm atomically at creation; POSIX timer_create() passes value == NULL */
		ret = k_timer_settime(t, flags & ~(uint32_t)SYS_TIMER_SIGVAL_SELF, value,
				      interval, NULL, NULL);
		if (ret < 0) {
			goto err_release;
		}
	}

	*timer = t;

	return 0;

err_release:
#ifdef CONFIG_USERSPACE
	if (k_is_in_user_syscall()) {
		k_object_access_revoke(t, _current);
	}
#endif /* CONFIG_USERSPACE */
	K_SPINLOCK(&timer_pool_lock) {
		(void)sys_elastipool_free(&timer_pool, t);
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_timer_create(int clock_id, const struct k_timer_notify *notify,
					  uint32_t flags, const struct timespec *value,
					  const struct timespec *interval, struct k_timer **timer)
{
	struct k_timer_notify knotify;
	struct timespec kvalue;
	struct timespec kinterval;
	const struct k_timer_notify *np = NULL;
	const struct timespec *vp = NULL;
	const struct timespec *ip = NULL;

	K_OOPS(K_SYSCALL_MEMORY_WRITE(timer, sizeof(*timer)));
	if (notify != NULL) {
		/* copy to kernel stack: the target pointer must not change after validation */
		K_OOPS(k_usermode_from_copy(&knotify, notify, sizeof(knotify)));
		if (knotify.fn == NULL) {
			/* the caller must have been granted access to the target thread */
			K_OOPS(K_SYSCALL_OBJ(knotify.target, K_OBJ_THREAD));
		}
		np = &knotify;
	}
	if (value != NULL) {
		K_OOPS(k_usermode_from_copy(&kvalue, value, sizeof(kvalue)));
		vp = &kvalue;
	}
	if (interval != NULL) {
		K_OOPS(k_usermode_from_copy(&kinterval, interval, sizeof(kinterval)));
		ip = &kinterval;
	}
	return z_impl_sys_timer_create(clock_id, np, flags, vp, ip, timer);
}
#include <zephyr/syscalls/sys_timer_create_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_sys_timer_delete(struct k_timer *timer)
{
	int ret = 0;

	K_SPINLOCK(&timer_pool_lock) {
		if (sys_elastipool_check(&timer_pool, timer) != 0) {
			ret = -EINVAL;
			K_SPINLOCK_BREAK;
		}
	}
	if (ret < 0) {
		return ret;
	}

	/* stops the timer and deregisters it from the realtime re-target registry */
	k_timer_stop(timer);

#ifdef CONFIG_TIMER_SIGNAL
	/* detach notification and purge queued-but-undelivered signal instances */
	z_timer_sig_detach(timer);
	K_SPINLOCK(&timer_pool_lock) {
		timer->sig_fn = NULL;
	}
#endif /* CONFIG_TIMER_SIGNAL */

#ifdef CONFIG_USERSPACE
	if (k_is_in_user_syscall()) {
		k_object_access_revoke(timer, _current);
	}
#endif /* CONFIG_USERSPACE */

	K_SPINLOCK(&timer_pool_lock) {
		ret = sys_elastipool_free(&timer_pool, timer);
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_timer_delete(struct k_timer *timer)
{
	K_OOPS(K_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_sys_timer_delete(timer);
}
#include <zephyr/syscalls/sys_timer_delete_mrsh.c>
#endif /* CONFIG_USERSPACE */
