/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/sys/atomic.h>
#include <ksched.h>
#include <zephyr/wait_q.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/slist.h>

#include "posix_internal.h"

#define PTHREAD_INIT_FLAGS	PTHREAD_CANCEL_ENABLE
#define PTHREAD_CANCELED	((void *) -1)

#define LOWEST_POSIX_THREAD_PRIORITY 1

K_MUTEX_DEFINE(pthread_once_lock);

static const struct pthread_attr init_pthread_attrs = {
	.priority = LOWEST_POSIX_THREAD_PRIORITY,
	.stack = NULL,
	.stacksize = 0,
	.flags = PTHREAD_INIT_FLAGS,
	.delayedstart = 0,
#if defined(CONFIG_PREEMPT_ENABLED)
	.schedpolicy = SCHED_RR,
#else
	.schedpolicy = SCHED_FIFO,
#endif
	.detachstate = PTHREAD_CREATE_JOINABLE,
	.initialized = true,
};

static struct posix_thread posix_thread_pool[CONFIG_MAX_PTHREAD_COUNT];
static struct k_spinlock pthread_pool_lock;

pthread_t pthread_self(void)
{
	return (struct posix_thread *)
		CONTAINER_OF(k_current_get(), struct posix_thread, thread)
		- posix_thread_pool;
}

struct posix_thread *to_posix_thread(pthread_t pthread)
{
	if (pthread >= CONFIG_MAX_PTHREAD_COUNT) {
		return NULL;
	}

	return &posix_thread_pool[pthread];
}

static bool is_posix_prio_valid(uint32_t priority, int policy)
{
	if (priority >= sched_get_priority_min(policy) &&
	    priority <= sched_get_priority_max(policy)) {
		return true;
	}

	return false;
}

static uint32_t zephyr_to_posix_priority(int32_t z_prio, int *policy)
{
	uint32_t prio;

	if (z_prio < 0) {
		*policy = SCHED_FIFO;
		prio = -1 * (z_prio + 1);
	} else {
		*policy = SCHED_RR;
		prio = (CONFIG_NUM_PREEMPT_PRIORITIES - z_prio);
	}

	return prio;
}

static int32_t posix_to_zephyr_priority(uint32_t priority, int policy)
{
	int32_t prio;

	if (policy == SCHED_FIFO) {
		/* Zephyr COOP priority starts from -1 */
		prio =  -1 * (priority + 1);
	} else {
		prio = (CONFIG_NUM_PREEMPT_PRIORITIES - priority);
	}

	return prio;
}

/**
 * @brief Set scheduling parameter attributes in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_setschedparam(pthread_attr_t *_attr, const struct sched_param *schedparam)
{
	struct pthread_attr *attr = (struct pthread_attr *)_attr;
	int priority = schedparam->sched_priority;

	if ((attr == NULL) || (attr->initialized == 0U) ||
	    (is_posix_prio_valid(priority, attr->schedpolicy) == false)) {
		return EINVAL;
	}

	attr->priority = priority;
	return 0;
}

/**
 * @brief Set stack attributes in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_setstack(pthread_attr_t *_attr, void *stackaddr, size_t stacksize)
{
	struct pthread_attr *attr = (struct pthread_attr *)_attr;

	if (stackaddr == NULL) {
		return EACCES;
	}

	attr->stack = stackaddr;
	attr->stacksize = stacksize;
	return 0;
}

static void zephyr_thread_wrapper(void *arg1, void *arg2, void *arg3)
{
	void * (*fun_ptr)(void *) = arg3;

	fun_ptr(arg1);
	pthread_exit(NULL);
}

/**
 * @brief Create a new thread.
 *
 * Pthread attribute should not be NULL. API will return Error on NULL
 * attribute value.
 *
 * See IEEE 1003.1
 */
int pthread_create(pthread_t *newthread, const pthread_attr_t *_attr,
		   void *(*threadroutine)(void *), void *arg)
{
	int rv;
	int32_t prio;
	k_spinlock_key_t key;
	uint32_t pthread_num;
	k_spinlock_key_t cancel_key;
	pthread_condattr_t cond_attr;
	struct posix_thread *thread;
	const struct pthread_attr *attr = (const struct pthread_attr *)_attr;

	/*
	 * FIXME: Pthread attribute must be non-null and it provides stack
	 * pointer and stack size. So even though POSIX 1003.1 spec accepts
	 * attrib as NULL but zephyr needs it initialized with valid stack.
	 */
	if ((attr == NULL) || (attr->initialized == 0U)
	    || (attr->stack == NULL) || (attr->stacksize == 0)) {
		return EINVAL;
	}

	key = k_spin_lock(&pthread_pool_lock);
	for (pthread_num = 0;
	    pthread_num < CONFIG_MAX_PTHREAD_COUNT; pthread_num++) {
		thread = &posix_thread_pool[pthread_num];
		if (thread->state == PTHREAD_EXITED || thread->state == PTHREAD_TERMINATED) {
			thread->state = PTHREAD_JOINABLE;
			break;
		}
	}
	k_spin_unlock(&pthread_pool_lock, key);

	if (pthread_num >= CONFIG_MAX_PTHREAD_COUNT) {
		return EAGAIN;
	}

	rv = pthread_mutex_init(&thread->state_lock, NULL);
	if (rv != 0) {
		key = k_spin_lock(&pthread_pool_lock);
		thread->state = PTHREAD_EXITED;
		k_spin_unlock(&pthread_pool_lock, key);
		return rv;
	}

	prio = posix_to_zephyr_priority(attr->priority, attr->schedpolicy);

	cancel_key = k_spin_lock(&thread->cancel_lock);
	thread->cancel_state = (1 << _PTHREAD_CANCEL_POS) & attr->flags;
	thread->cancel_pending = 0;
	k_spin_unlock(&thread->cancel_lock, cancel_key);

	pthread_mutex_lock(&thread->state_lock);
	thread->state = attr->detachstate;
	pthread_mutex_unlock(&thread->state_lock);

	pthread_cond_init(&thread->state_cond, &cond_attr);
	sys_slist_init(&thread->key_list);

	*newthread = pthread_num;
	k_thread_create(&thread->thread, attr->stack, attr->stacksize,
			(k_thread_entry_t)zephyr_thread_wrapper, (void *)arg, NULL, threadroutine,
			prio, (~K_ESSENTIAL & attr->flags), K_MSEC(attr->delayedstart));
	return 0;
}


/**
 * @brief Set cancelability State.
 *
 * See IEEE 1003.1
 */
int pthread_setcancelstate(int state, int *oldstate)
{
	bool cancel_pending;
	k_spinlock_key_t cancel_key;
	struct posix_thread *pthread = to_posix_thread(pthread_self());

	if (state != PTHREAD_CANCEL_ENABLE &&
	    state != PTHREAD_CANCEL_DISABLE) {
		return EINVAL;
	}

	cancel_key = k_spin_lock(&pthread->cancel_lock);
	*oldstate = pthread->cancel_state;
	pthread->cancel_state = state;
	cancel_pending = pthread->cancel_pending;
	k_spin_unlock(&pthread->cancel_lock, cancel_key);

	if (state == PTHREAD_CANCEL_ENABLE && cancel_pending) {
		pthread_exit((void *)PTHREAD_CANCELED);
	}

	return 0;
}

/**
 * @brief Cancel execution of a thread.
 *
 * See IEEE 1003.1
 */
int pthread_cancel(pthread_t pthread)
{
	struct posix_thread *thread = to_posix_thread(pthread);
	int cancel_state;
	k_spinlock_key_t cancel_key;

	if ((thread == NULL) || (thread->state == PTHREAD_TERMINATED)) {
		return ESRCH;
	}

	cancel_key = k_spin_lock(&thread->cancel_lock);
	thread->cancel_pending = 1;
	cancel_state = thread->cancel_state;
	k_spin_unlock(&thread->cancel_lock, cancel_key);

	if (cancel_state == PTHREAD_CANCEL_ENABLE) {
		pthread_mutex_lock(&thread->state_lock);
		if (thread->state == PTHREAD_DETACHED) {
			thread->state = PTHREAD_TERMINATED;
		} else {
			thread->retval = PTHREAD_CANCELED;
			thread->state = PTHREAD_EXITED;
			pthread_cond_broadcast(&thread->state_cond);
		}
		pthread_mutex_unlock(&thread->state_lock);

		k_thread_abort(&thread->thread);
	}

	return 0;
}

/**
 * @brief Set thread scheduling policy and parameters.
 *
 * See IEEE 1003.1
 */
int pthread_setschedparam(pthread_t pthread, int policy,
			  const struct sched_param *param)
{
	struct posix_thread *thread = to_posix_thread(pthread);
	int new_prio;

	if (thread == NULL) {
		return ESRCH;
	}

	if (policy != SCHED_RR && policy != SCHED_FIFO) {
		return EINVAL;
	}

	if (is_posix_prio_valid(param->sched_priority, policy) == false) {
		return EINVAL;
	}

	new_prio = posix_to_zephyr_priority(param->sched_priority, policy);

	k_thread_priority_set(&thread->thread, new_prio);
	return 0;
}

/**
 * @brief Initialise threads attribute object
 *
 * See IEEE 1003.1
 */
int pthread_attr_init(pthread_attr_t *attr)
{

	if (attr == NULL) {
		return ENOMEM;
	}

	(void)memcpy(attr, &init_pthread_attrs, sizeof(pthread_attr_t));

	return 0;
}

/**
 * @brief Get thread scheduling policy and parameters
 *
 * See IEEE 1003.1
 */
int pthread_getschedparam(pthread_t pthread, int *policy,
			  struct sched_param *param)
{
	struct posix_thread *thread = to_posix_thread(pthread);
	uint32_t priority;

	if ((thread == NULL) || (thread->state == PTHREAD_TERMINATED)) {
		return ESRCH;
	}

	priority = k_thread_priority_get(&thread->thread);

	param->sched_priority = zephyr_to_posix_priority(priority, policy);
	return 0;
}

/**
 * @brief Dynamic package initialization
 *
 * See IEEE 1003.1
 */
int pthread_once(pthread_once_t *once, void (*init_func)(void))
{
	k_mutex_lock(&pthread_once_lock, K_FOREVER);

	if (once->is_initialized != 0 && once->init_executed == 0) {
		init_func();
		once->init_executed = 1;
	}

	k_mutex_unlock(&pthread_once_lock);

	return 0;
}

/**
 * @brief Terminate calling thread.
 *
 * See IEEE 1003.1
 */
void pthread_exit(void *retval)
{
	k_spinlock_key_t cancel_key;
	struct posix_thread *self = to_posix_thread(pthread_self());
	pthread_key_obj *key_obj;
	pthread_thread_data *thread_spec_data;
	sys_snode_t *node_l;

	/* Make a thread as cancelable before exiting */
	cancel_key = k_spin_lock(&self->cancel_lock);
	if (self->cancel_state == PTHREAD_CANCEL_DISABLE) {
		self->cancel_state = PTHREAD_CANCEL_ENABLE;
	}

	k_spin_unlock(&self->cancel_lock, cancel_key);

	pthread_mutex_lock(&self->state_lock);
	if (self->state == PTHREAD_JOINABLE) {
		self->state = PTHREAD_EXITED;
		self->retval = retval;
		pthread_cond_broadcast(&self->state_cond);
	} else {
		self->state = PTHREAD_TERMINATED;
	}

	SYS_SLIST_FOR_EACH_NODE(&self->key_list, node_l) {
		thread_spec_data = (pthread_thread_data *)node_l;
		if (thread_spec_data != NULL) {
			key_obj = thread_spec_data->key;
			if (key_obj->destructor != NULL) {
				(key_obj->destructor)(thread_spec_data->spec_data);
			}
		}
	}

	pthread_mutex_unlock(&self->state_lock);
	pthread_mutex_destroy(&self->state_lock);

	pthread_cond_destroy(&self->state_cond);

	k_thread_abort((k_tid_t)self);
}

/**
 * @brief Wait for a thread termination.
 *
 * See IEEE 1003.1
 */
int pthread_join(pthread_t thread, void **status)
{
	struct posix_thread *pthread = to_posix_thread(thread);
	int ret = 0;

	if (thread == pthread_self()) {
		return EDEADLK;
	}

	if (pthread == NULL) {
		return ESRCH;
	}

	pthread_mutex_lock(&pthread->state_lock);

	if (pthread->state == PTHREAD_JOINABLE) {
		pthread_cond_wait(&pthread->state_cond, &pthread->state_lock);
	}

	if (pthread->state == PTHREAD_EXITED) {
		if (status != NULL) {
			*status = pthread->retval;
		}
	} else if (pthread->state == PTHREAD_DETACHED) {
		ret = EINVAL;
	} else {
		ret = ESRCH;
	}

	pthread_mutex_unlock(&pthread->state_lock);
	if (pthread->state == PTHREAD_EXITED) {
		pthread_mutex_destroy(&pthread->state_lock);
	}

	return ret;
}

/**
 * @brief Detach a thread.
 *
 * See IEEE 1003.1
 */
int pthread_detach(pthread_t thread)
{
	struct posix_thread *pthread = to_posix_thread(thread);
	int ret = 0;

	if (pthread == NULL) {
		return ESRCH;
	}

	pthread_mutex_lock(&pthread->state_lock);

	switch (pthread->state) {
	case PTHREAD_JOINABLE:
		pthread->state = PTHREAD_DETACHED;
		/* Broadcast the condition.
		 * This will make threads waiting to join this thread continue.
		 */
		pthread_cond_broadcast(&pthread->state_cond);
		break;
	case PTHREAD_EXITED:
		pthread->state = PTHREAD_TERMINATED;
		/* THREAD has already exited.
		 * Pthread remained to provide exit status.
		 */
		break;
	case PTHREAD_TERMINATED:
		ret = ESRCH;
		break;
	default:
		ret = EINVAL;
		break;
	}

	pthread_mutex_unlock(&pthread->state_lock);
	return ret;
}

/**
 * @brief Get detach state attribute in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_getdetachstate(const pthread_attr_t *_attr, int *detachstate)
{
	const struct pthread_attr *attr = (const struct pthread_attr *)_attr;

	if ((attr == NULL) || (attr->initialized == 0U)) {
		return EINVAL;
	}

	*detachstate = attr->detachstate;
	return 0;
}

/**
 * @brief Set detach state attribute in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_setdetachstate(pthread_attr_t *_attr, int detachstate)
{
	struct pthread_attr *attr = (struct pthread_attr *)_attr;

	if ((attr == NULL) || (attr->initialized == 0U) ||
	    (detachstate != PTHREAD_CREATE_DETACHED &&
	     detachstate != PTHREAD_CREATE_JOINABLE)) {
		return EINVAL;
	}

	attr->detachstate = detachstate;
	return 0;
}


/**
 * @brief Get scheduling policy attribute in Thread attributes.
 *
 * See IEEE 1003.1
 */
int pthread_attr_getschedpolicy(const pthread_attr_t *_attr, int *policy)
{
	const struct pthread_attr *attr = (const struct pthread_attr *)_attr;

	if ((attr == NULL) || (attr->initialized == 0U)) {
		return EINVAL;
	}

	*policy = attr->schedpolicy;
	return 0;
}


/**
 * @brief Set scheduling policy attribute in Thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_setschedpolicy(pthread_attr_t *_attr, int policy)
{
	struct pthread_attr *attr = (struct pthread_attr *)_attr;

	if ((attr == NULL) || (attr->initialized == 0U) ||
	    (policy != SCHED_RR && policy != SCHED_FIFO)) {
		return EINVAL;
	}

	attr->schedpolicy = policy;
	return 0;
}

/**
 * @brief Get stack size attribute in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_getstacksize(const pthread_attr_t *_attr, size_t *stacksize)
{
	const struct pthread_attr *attr = (const struct pthread_attr *)_attr;

	if ((attr == NULL) || (attr->initialized == 0U)) {
		return EINVAL;
	}

	*stacksize = attr->stacksize;
	return 0;

}

/**
 * @brief Set stack size attribute in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_setstacksize(pthread_attr_t *_attr, size_t stacksize)
{
	struct pthread_attr *attr = (struct pthread_attr *)_attr;

	if ((attr == NULL) || (attr->initialized == 0U)) {
		return EINVAL;
	}

	if (stacksize < PTHREAD_STACK_MIN) {
		return EINVAL;
	}

	attr->stacksize = stacksize;
	return 0;
}

/**
 * @brief Get stack attributes in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_getstack(const pthread_attr_t *_attr, void **stackaddr, size_t *stacksize)
{
	const struct pthread_attr *attr = (const struct pthread_attr *)_attr;

	if ((attr == NULL) || (attr->initialized == 0U)) {
		return EINVAL;
	}

	*stackaddr = attr->stack;
	*stacksize = attr->stacksize;
	return 0;
}

/**
 * @brief Get thread attributes object scheduling parameters.
 *
 * See IEEE 1003.1
 */
int pthread_attr_getschedparam(const pthread_attr_t *_attr, struct sched_param *schedparam)
{
	struct pthread_attr *attr = (struct pthread_attr *)_attr;

	if ((attr == NULL) || (attr->initialized == 0U)) {
		return EINVAL;
	}

	schedparam->sched_priority = attr->priority;
	return 0;
}

/**
 * @brief Destroy thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_destroy(pthread_attr_t *_attr)
{
	struct pthread_attr *attr = (struct pthread_attr *)_attr;

	if ((attr != NULL) && (attr->initialized != 0U)) {
		attr->initialized = false;
		return 0;
	}

	return EINVAL;
}

int pthread_setname_np(pthread_t thread, const char *name)
{
#ifdef CONFIG_THREAD_NAME
	k_tid_t kthread;

	if (thread >= CONFIG_MAX_PTHREAD_COUNT) {
		return ESRCH;
	}

	kthread = &posix_thread_pool[thread].thread;

	if (name == NULL) {
		return EINVAL;
	}

	return k_thread_name_set(kthread, name);
#else
	ARG_UNUSED(thread);
	ARG_UNUSED(name);
	return 0;
#endif
}

int pthread_getname_np(pthread_t thread, char *name, size_t len)
{
#ifdef CONFIG_THREAD_NAME
	k_tid_t kthread;

	if (thread >= CONFIG_MAX_PTHREAD_COUNT) {
		return ESRCH;
	}

	if (name == NULL) {
		return EINVAL;
	}

	memset(name, '\0', len);
	kthread = &posix_thread_pool[thread].thread;
	return k_thread_name_copy(kthread, name, len-1);
#else
	ARG_UNUSED(thread);
	ARG_UNUSED(name);
	ARG_UNUSED(len);
	return 0;
#endif
}

static int posix_thread_pool_init(void)
{
	size_t i;


	for (i = 0; i < CONFIG_MAX_PTHREAD_COUNT; ++i) {
		posix_thread_pool[i].state = PTHREAD_EXITED;
	}

	return 0;
}

SYS_INIT(posix_thread_pool_init, PRE_KERNEL_1, 0);
