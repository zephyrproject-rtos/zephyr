/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
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
		/* signal-based expiry notification arrives with CONFIG_TIMER_SIGNAL */
		return -ENOTSUP;
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

#ifdef CONFIG_USERSPACE
	if (k_is_in_user_syscall()) {
		k_object_access_grant(t, _current);
	}
#endif /* CONFIG_USERSPACE */

	if (value != NULL) {
		/* arm atomically at creation; POSIX timer_create() passes value == NULL */
		ret = k_timer_settime(t, flags, value, interval, NULL, NULL);
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
		/* the caller must have been granted access to the target thread */
		K_OOPS(K_SYSCALL_OBJ(knotify.target, K_OBJ_THREAD));
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
