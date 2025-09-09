/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2024 Gaetan Perrot
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_clock.h"
#include "posix_internal.h"
#include "pthread_sched.h"

#include <zephyr/logging/log.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/sem.h>
#include <zephyr/toolchain.h>

extern struct posix_thread posix_thread_pool[CONFIG_POSIX_THREAD_THREADS_MAX];
extern struct sys_sem pthread_pool_lock;

LOG_MODULE_REGISTER(xsi_realtime_threads, CONFIG_XSI_REALTIME_THREADS_LOG_LEVEL);

int pthread_attr_getscope(const pthread_attr_t *_attr, int *contentionscope)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || contentionscope == NULL) {
		return EINVAL;
	}
	*contentionscope = attr->contentionscope;
	return 0;
}

int pthread_attr_setscope(pthread_attr_t *attr, int contentionscope)
{
	struct posix_thread_attr *_attr = (struct posix_thread_attr *)attr;

	if (!__attr_is_initialized(_attr)) {
		LOG_DBG("attr %p is not initialized", _attr);
		return EINVAL;
	}
	if (!(contentionscope == PTHREAD_SCOPE_PROCESS ||
	      contentionscope == PTHREAD_SCOPE_SYSTEM)) {
		LOG_DBG("%s contentionscope %d", "Invalid", contentionscope);
		return EINVAL;
	}
	if (contentionscope == PTHREAD_SCOPE_PROCESS) {
		/* Zephyr does not yet support processes or process scheduling */
		LOG_DBG("%s contentionscope %d", "Unsupported", contentionscope);
		return ENOTSUP;
	}
	_attr->contentionscope = contentionscope;
	return 0;
}

int pthread_attr_getinheritsched(const pthread_attr_t *_attr, int *inheritsched)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || inheritsched == NULL) {
		LOG_DBG("attr %p is not initialized", _attr);
		return EINVAL;
	}
	*inheritsched = attr->inheritsched;
	return 0;
}

int pthread_attr_setinheritsched(pthread_attr_t *_attr, int inheritsched)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr)) {
		LOG_DBG("attr %p is not initialized", attr);
		return EINVAL;
	}

	if ((inheritsched != PTHREAD_INHERIT_SCHED) && (inheritsched != PTHREAD_EXPLICIT_SCHED)) {
		LOG_DBG("Invalid inheritsched %d", inheritsched);
		return EINVAL;
	}

	attr->inheritsched = inheritsched;
	return 0;
}

int pthread_attr_getschedpolicy(const pthread_attr_t *_attr, int *policy)
{
	const struct posix_thread_attr *attr = (const struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || (policy == NULL)) {
		LOG_DBG("attr %p is not initialized", attr);
		return EINVAL;
	}

	*policy = attr->schedpolicy;
	return 0;
}

int pthread_attr_setschedpolicy(pthread_attr_t *_attr, int policy)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || !valid_posix_policy(policy)) {
		LOG_DBG("attr %p is not initialized", attr);
		return EINVAL;
	}

	attr->schedpolicy = policy;
	return 0;
}

int pthread_getschedparam(pthread_t pthread, int *policy, struct sched_param *param)
{
	int ret = ESRCH;
	struct posix_thread *t;

	if (policy == NULL || param == NULL) {
		return EINVAL;
	}

	SYS_SEM_LOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL) {
			ret = ESRCH;
			SYS_SEM_LOCK_BREAK;
		}

		if (!__attr_is_initialized(&t->attr)) {
			ret = ESRCH;
			SYS_SEM_LOCK_BREAK;
		}

		ret = 0;
		param->sched_priority =
			zephyr_to_posix_priority(k_thread_priority_get(&t->thread), policy);
	}

	return ret;
}

int pthread_mutex_getprioceiling(const pthread_mutex_t *mutex, int *prioceiling)
{
	ARG_UNUSED(mutex);
	ARG_UNUSED(prioceiling);

	return ENOSYS;
}

int pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioceiling, int *old_ceiling)
{
	ARG_UNUSED(mutex);
	ARG_UNUSED(prioceiling);
	ARG_UNUSED(old_ceiling);

	return ENOSYS;
}

int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr, int *prioceiling)
{
	ARG_UNUSED(attr);
	ARG_UNUSED(prioceiling);

	return ENOSYS;
}

int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling)
{
	ARG_UNUSED(attr);
	ARG_UNUSED(prioceiling);

	return ENOSYS;
}

int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, int *protocol)
{
	if ((attr == NULL) || (protocol == NULL)) {
		return EINVAL;
	}

	*protocol = PTHREAD_PRIO_NONE;
	return 0;
}

int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)
{
	if (attr == NULL) {
		return EINVAL;
	}

	switch (protocol) {
	case PTHREAD_PRIO_NONE:
		return 0;
	case PTHREAD_PRIO_INHERIT:
		return ENOTSUP;
	case PTHREAD_PRIO_PROTECT:
		return ENOTSUP;
	default:
		return EINVAL;
	}
}

int pthread_setschedparam(pthread_t pthread, int policy, const struct sched_param *param)
{
	int ret = ESRCH;
	int new_prio = K_LOWEST_APPLICATION_THREAD_PRIO;
	struct posix_thread *t = NULL;

	if (param == NULL || !valid_posix_policy(policy) ||
	    !is_posix_policy_prio_valid(param->sched_priority, policy)) {
		return EINVAL;
	}

	SYS_SEM_LOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL) {
			ret = ESRCH;
			SYS_SEM_LOCK_BREAK;
		}

		ret = 0;
		new_prio = posix_to_zephyr_priority(param->sched_priority, policy);
	}

	if (ret == 0) {
		k_thread_priority_set(&t->thread, new_prio);
	}

	return ret;
}

int pthread_setschedprio(pthread_t thread, int prio)
{
	int ret;
	int new_prio = K_LOWEST_APPLICATION_THREAD_PRIO;
	struct posix_thread *t = NULL;
	int policy = -1;
	struct sched_param param;

	ret = pthread_getschedparam(thread, &policy, &param);
	if (ret != 0) {
		return ret;
	}

	if (!is_posix_policy_prio_valid(prio, policy)) {
		return EINVAL;
	}

	ret = ESRCH;
	SYS_SEM_LOCK(&pthread_pool_lock) {
		t = to_posix_thread(thread);
		if (t == NULL) {
			ret = ESRCH;
			SYS_SEM_LOCK_BREAK;
		}

		ret = 0;
		new_prio = posix_to_zephyr_priority(prio, policy);
	}

	if (ret == 0) {
		k_thread_priority_set(&t->thread, new_prio);
	}

	return ret;
}
