/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/sys/atomic.h>
#include <ksched.h>
#include <zephyr/wait_q.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/slist.h>

#define PTHREAD_INIT_FLAGS	PTHREAD_CANCEL_ENABLE
#define PTHREAD_CANCELED	((void *) -1)

#define LOWEST_POSIX_THREAD_PRIORITY 1

PTHREAD_MUTEX_DEFINE(pthread_key_lock);

static const pthread_attr_t init_pthread_attrs = {
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
PTHREAD_MUTEX_DEFINE(pthread_pool_lock);

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
int pthread_attr_setschedparam(pthread_attr_t *attr,
			       const struct sched_param *schedparam)
{
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
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr,
			  size_t stacksize)
{
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
int pthread_create(pthread_t *newthread, const pthread_attr_t *attr,
		   void *(*threadroutine)(void *), void *arg)
{
	int32_t prio;
	uint32_t pthread_num;
	pthread_condattr_t cond_attr;
	struct posix_thread *thread;

	/*
	 * FIXME: Pthread attribute must be non-null and it provides stack
	 * pointer and stack size. So even though POSIX 1003.1 spec accepts
	 * attrib as NULL but zephyr needs it initialized with valid stack.
	 */
	if ((attr == NULL) || (attr->initialized == 0U)
	    || (attr->stack == NULL) || (attr->stacksize == 0)) {
		return EINVAL;
	}

	pthread_mutex_lock(&pthread_pool_lock);
	for (pthread_num = 0;
	    pthread_num < CONFIG_MAX_PTHREAD_COUNT; pthread_num++) {
		thread = &posix_thread_pool[pthread_num];
		if (thread->state == PTHREAD_TERMINATED) {
			thread->state = PTHREAD_JOINABLE;
			break;
		}
	}
	pthread_mutex_unlock(&pthread_pool_lock);

	if (pthread_num >= CONFIG_MAX_PTHREAD_COUNT) {
		return EAGAIN;
	}

	prio = posix_to_zephyr_priority(attr->priority, attr->schedpolicy);

	thread = &posix_thread_pool[pthread_num];
	/*
	 * Ignore return value, as we know that Zephyr implementation
	 * cannot fail.
	 */
	(void)pthread_mutex_init(&thread->state_lock, NULL);
	(void)pthread_mutex_init(&thread->cancel_lock, NULL);

	pthread_mutex_lock(&thread->cancel_lock);
	thread->cancel_state = (1 << _PTHREAD_CANCEL_POS) & attr->flags;
	thread->cancel_pending = 0;
	pthread_mutex_unlock(&thread->cancel_lock);

	pthread_mutex_lock(&thread->state_lock);
	thread->state = attr->detachstate;
	pthread_mutex_unlock(&thread->state_lock);

	pthread_cond_init(&thread->state_cond, &cond_attr);
	sys_slist_init(&thread->key_list);

	*newthread = (pthread_t) k_thread_create(&thread->thread, attr->stack,
						 attr->stacksize,
						 (k_thread_entry_t)
						 zephyr_thread_wrapper,
						 (void *)arg, NULL,
						 threadroutine, prio,
						 (~K_ESSENTIAL & attr->flags),
						 K_MSEC(attr->delayedstart));
	return 0;
}


/**
 * @brief Set cancelability State.
 *
 * See IEEE 1003.1
 */
int pthread_setcancelstate(int state, int *oldstate)
{
	struct posix_thread *pthread = (struct posix_thread *) pthread_self();

	if (state != PTHREAD_CANCEL_ENABLE &&
	    state != PTHREAD_CANCEL_DISABLE) {
		return EINVAL;
	}

	*oldstate = pthread->cancel_state;

	pthread_mutex_lock(&pthread->cancel_lock);
	pthread->cancel_state = state;
	pthread_mutex_unlock(&pthread->cancel_lock);

	if (state == PTHREAD_CANCEL_ENABLE && pthread->cancel_pending) {
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
	struct posix_thread *thread = (struct posix_thread *) pthread;
	int cancel_state;

	if ((thread == NULL) || (thread->state == PTHREAD_TERMINATED)) {
		return ESRCH;
	}

	pthread_mutex_lock(&thread->cancel_lock);
	thread->cancel_pending = 1;
	cancel_state = thread->cancel_state;
	pthread_mutex_unlock(&thread->cancel_lock);

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

		k_thread_abort((k_tid_t) thread);
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
	k_tid_t thread = (k_tid_t)pthread;
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

	k_thread_priority_set(thread, new_prio);
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
	struct posix_thread *thread = (struct posix_thread *) pthread;
	uint32_t priority;

	if ((thread == NULL) || (thread->state == PTHREAD_TERMINATED)) {
		return ESRCH;
	}

	priority = k_thread_priority_get((k_tid_t) thread);

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
	pthread_mutex_lock(&pthread_key_lock);

	if (*once != PTHREAD_ONCE_INIT) {
		pthread_mutex_unlock(&pthread_key_lock);
		return 0;
	}

	init_func();
	*once = 0;

	pthread_mutex_unlock(&pthread_key_lock);

	return 0;
}

/**
 * @brief Terminate calling thread.
 *
 * See IEEE 1003.1
 */
void pthread_exit(void *retval)
{
	struct posix_thread *self = (struct posix_thread *)pthread_self();
	pthread_key_obj *key_obj;
	pthread_thread_data *thread_spec_data;
	sys_snode_t *node_l;

	/* Make a thread as cancelable before exiting */
	pthread_mutex_lock(&self->cancel_lock);
	if (self->cancel_state == PTHREAD_CANCEL_DISABLE) {
		self->cancel_state = PTHREAD_CANCEL_ENABLE;
	}

	pthread_mutex_unlock(&self->cancel_lock);

	pthread_mutex_lock(&self->state_lock);
	if (self->state == PTHREAD_JOINABLE) {
		self->retval = retval;
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
	k_thread_abort((k_tid_t)self);
}

/**
 * @brief Wait for a thread termination.
 *
 * See IEEE 1003.1
 */
int pthread_join(pthread_t thread, void **status)
{
	struct posix_thread *pthread = (struct posix_thread *) thread;
	int ret = 0;

	if (pthread == NULL) {
		return ESRCH;
	}

	if (pthread == pthread_self()) {
		return EDEADLK;
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
	return ret;
}

/**
 * @brief Detach a thread.
 *
 * See IEEE 1003.1
 */
int pthread_detach(pthread_t thread)
{
	struct posix_thread *pthread = (struct posix_thread *) thread;
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
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
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
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
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
int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy)
{
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
int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)
{
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
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
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
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
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
int pthread_attr_getstack(const pthread_attr_t *attr,
				 void **stackaddr, size_t *stacksize)
{
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
int pthread_attr_getschedparam(const pthread_attr_t *attr,
			       struct sched_param *schedparam)
{
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
int pthread_attr_destroy(pthread_attr_t *attr)
{
	if ((attr != NULL) && (attr->initialized != 0U)) {
		attr->initialized = false;
		return 0;
	}

	return EINVAL;
}

int pthread_setname_np(pthread_t thread, const char *name)
{
#ifdef CONFIG_THREAD_NAME
	k_tid_t kthread = (k_tid_t)thread;

	if (kthread == NULL) {
		return ESRCH;
	}

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
	k_tid_t kthread = (k_tid_t)thread;

	if (kthread == NULL) {
		return ESRCH;
	}

	if (name == NULL) {
		return EINVAL;
	}

	memset(name, '\0', len);
	return k_thread_name_copy(kthread, name, len-1);
#else
	ARG_UNUSED(thread);
	ARG_UNUSED(name);
	ARG_UNUSED(len);
	return 0;
#endif
}
