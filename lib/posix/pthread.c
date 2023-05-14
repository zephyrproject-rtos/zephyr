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
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/slist.h>

#include "posix_internal.h"
#include "pthread_sched.h"

#define PTHREAD_INIT_FLAGS	PTHREAD_CANCEL_ENABLE
#define PTHREAD_CANCELED	((void *) -1)

K_MUTEX_DEFINE(pthread_once_lock);

static const struct pthread_attr init_pthread_attrs = {
	.priority = 0,
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
SYS_BITARRAY_DEFINE_STATIC(posix_thread_bitarray, CONFIG_MAX_PTHREAD_COUNT);

/*
 * We reserve the MSB to mark a pthread_t as initialized (from the
 * perspective of the application). With a linear space, this means that
 * the theoretical pthread_t range is [0,2147483647].
 */
BUILD_ASSERT(CONFIG_MAX_PTHREAD_COUNT < PTHREAD_OBJ_MASK_INIT,
	     "CONFIG_MAX_PTHREAD_COUNT is too high");

static inline size_t posix_thread_to_offset(struct posix_thread *t)
{
	return t - posix_thread_pool;
}

static inline size_t get_posix_thread_idx(pthread_t pth)
{
	return mark_pthread_obj_uninitialized(pth);
}

struct posix_thread *to_posix_thread(pthread_t pth)
{
	int actually_initialized;
	size_t bit = get_posix_thread_idx(pth);

	/* if the provided thread does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(pth)) {
		return NULL;
	}

	/* Mask off the MSB to get the actual bit index */
	if (sys_bitarray_test_bit(&posix_thread_bitarray, bit, &actually_initialized) < 0) {
		return NULL;
	}

	if (actually_initialized == 0) {
		/* The thread claims to be initialized but is actually not */
		return NULL;
	}

	return &posix_thread_pool[bit];
}

pthread_t pthread_self(void)
{
	size_t bit;
	struct posix_thread *t;

	t = (struct posix_thread *)CONTAINER_OF(k_current_get(), struct posix_thread, thread);
	bit = posix_thread_to_offset(t);

	return mark_pthread_obj_initialized(bit);
}

static bool is_posix_policy_prio_valid(uint32_t priority, int policy)
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
		__ASSERT_NO_MSG(prio < CONFIG_NUM_COOP_PRIORITIES);
	} else {
		*policy = SCHED_RR;
		prio = (CONFIG_NUM_PREEMPT_PRIORITIES - z_prio - 1);
		__ASSERT_NO_MSG(prio < CONFIG_NUM_PREEMPT_PRIORITIES);
	}

	return prio;
}

static int32_t posix_to_zephyr_priority(uint32_t priority, int policy)
{
	int32_t prio;

	if (policy == SCHED_FIFO) {
		/* Zephyr COOP priority starts from -1 */
		__ASSERT_NO_MSG(priority < CONFIG_NUM_COOP_PRIORITIES);
		prio =  -1 * (priority + 1);
	} else {
		__ASSERT_NO_MSG(priority < CONFIG_NUM_PREEMPT_PRIORITIES);
		prio = (CONFIG_NUM_PREEMPT_PRIORITIES - priority - 1);
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
	    (is_posix_policy_prio_valid(priority, attr->schedpolicy) == false)) {
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

FUNC_NORETURN
static void zephyr_thread_wrapper(void *arg1, void *arg2, void *arg3)
{
	void *(*fun_ptr)(void *) = arg3;

	pthread_exit(fun_ptr(arg1));
	CODE_UNREACHABLE;
}

static bool pthread_attr_is_valid(const struct pthread_attr *attr)
{
	/*
	 * FIXME: Pthread attribute must be non-null and it provides stack
	 * pointer and stack size. So even though POSIX 1003.1 spec accepts
	 * attrib as NULL but zephyr needs it initialized with valid stack.
	 */
	if (attr == NULL || attr->initialized == 0U || attr->stack == NULL ||
	    attr->stacksize == 0) {
		return false;
	}

	/* require a valid scheduler policy */
	if (!valid_posix_policy(attr->schedpolicy)) {
		return false;
	}

	/* require a valid detachstate */
	if (!(attr->detachstate == PTHREAD_JOINABLE || attr->detachstate == PTHREAD_DETACHED)) {
		return false;
	}

	return true;
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
	size_t bit;
	int32_t prio;
	k_spinlock_key_t key;
	struct posix_thread *t;
	const struct pthread_attr *attr = (const struct pthread_attr *)_attr;

	if (!pthread_attr_is_valid(attr)) {
		return EINVAL;
	}

	if (sys_bitarray_alloc(&posix_thread_bitarray, 1, &bit) < 0) {
		/* No threads left to allocate */
		return EAGAIN;
	}

	t = &posix_thread_pool[bit];
	key = k_spin_lock(&t->lock);
	__ASSERT_NO_MSG(t->state == PTHREAD_TERMINATED);
	t->state = attr->detachstate;
	prio = posix_to_zephyr_priority(attr->priority, attr->schedpolicy);
	t->cancel_state = BIT(_PTHREAD_CANCEL_POS) & attr->flags;
	t->cancel_pending = 0;
	sys_slist_init(&t->key_list);
	k_spin_unlock(&t->lock, key);

	k_thread_create(&t->thread, attr->stack, attr->stacksize,
			(k_thread_entry_t)zephyr_thread_wrapper, (void *)arg, NULL, threadroutine,
			prio, (~K_ESSENTIAL & attr->flags), K_MSEC(attr->delayedstart));
	*newthread = mark_pthread_obj_initialized(bit);

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
	k_spinlock_key_t key;
	struct posix_thread *t = to_posix_thread(pthread_self());

	if (state != PTHREAD_CANCEL_ENABLE &&
	    state != PTHREAD_CANCEL_DISABLE) {
		return EINVAL;
	}

	key = k_spin_lock(&t->lock);
	*oldstate = t->cancel_state;
	t->cancel_state = state;
	cancel_pending = t->cancel_pending;
	k_spin_unlock(&t->lock, key);

	if (state == PTHREAD_CANCEL_ENABLE && cancel_pending) {
		pthread_exit(INT_TO_POINTER(PTHREAD_CANCELED));
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
	int rv = 0;
	k_spinlock_key_t key;
	bool should_abort = false;
	struct posix_thread *t = to_posix_thread(pthread);
	size_t bit = get_posix_thread_idx(pthread);

	if (t == NULL) {
		return ESRCH;
	}

	key = k_spin_lock(&t->lock);
	switch (t->state) {
	case PTHREAD_DETACHED:
		t->cancel_pending = 1;
		t->state = PTHREAD_TERMINATED;
		should_abort = true;
		break;
	case PTHREAD_JOINABLE:
		t->retval = PTHREAD_CANCELED;
		t->state = PTHREAD_EXITED;
		should_abort = true;
		break;
	case PTHREAD_EXITED:
		break;
	case PTHREAD_TERMINATED:
		rv = ESRCH;
		break;
	default:
		__ASSERT(false, "bad thread state %d", t->state);
		rv = ESRCH;
		break;
	}
	k_spin_unlock(&t->lock, key);

	if (should_abort) {
		sys_bitarray_free(&posix_thread_bitarray, 1, bit);
		k_thread_abort(&t->thread);
	}

	return rv;
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

	if (!valid_posix_policy(policy)) {
		return EINVAL;
	}

	if (is_posix_policy_prio_valid(param->sched_priority, policy) == false) {
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
FUNC_NORETURN
void pthread_exit(void *retval)
{
	sys_snode_t *node_l;
	k_spinlock_key_t key;
	pthread_key_obj *key_obj;
	bool should_free = false;
	bool should_destruct = false;
	pthread_thread_data *thread_spec_data;
	struct posix_thread *t = to_posix_thread(pthread_self());
	size_t bit = posix_thread_to_offset(t);

	__ASSERT(t != NULL, "not a valid pthread (%p)", k_current_get());
	__ASSERT(&t->thread == k_current_get(), "mismatch! &self->thread: %p k_current_get(): %p",
		 &t->thread, k_current_get());

	key = k_spin_lock(&t->lock);
	switch (t->state) {
	case PTHREAD_DETACHED:
		t->state = PTHREAD_TERMINATED;
		should_free = true;
		should_destruct = true;
		break;
	case PTHREAD_JOINABLE:
		z_thread_wake_joiners(&t->thread);
		t->state = PTHREAD_EXITED;
		should_destruct = true;
		if (retval) {
			t->retval = retval;
		}
		break;
	case PTHREAD_EXITED:
	/* fall through */
	case PTHREAD_TERMINATED:
	/* fall through */
	default:
		__ASSERT(false, "bad thread state %d", t->state);
		break;
	}
	k_spin_unlock(&t->lock, key);

	if (should_destruct) {
		SYS_SLIST_FOR_EACH_NODE(&t->key_list, node_l) {
			thread_spec_data = (pthread_thread_data *)node_l;
			if (thread_spec_data != NULL) {
				key_obj = thread_spec_data->key;
				if (key_obj->destructor != NULL) {
					(key_obj->destructor)(thread_spec_data->spec_data);
				}
			}
		}
	}

	if (should_free) {
		/* only free detached threads on exit */
		sys_bitarray_free(&posix_thread_bitarray, 1, bit);
	}

	/* caller will *ALWAYS* be aborted  */
	k_thread_abort(k_current_get());

	CODE_UNREACHABLE;
}

/**
 * @brief Wait for a thread termination.
 *
 * See IEEE 1003.1
 */
int pthread_join(pthread_t pthread, void **status)
{
	int ret;
	k_spinlock_key_t key;
	bool should_free = false;
	struct posix_thread *t = to_posix_thread(pthread);
	size_t bit = get_posix_thread_idx(pthread);

	if (pthread == pthread_self()) {
		return EDEADLK;
	}

	if (t == NULL) {
		return ESRCH;
	}

	key = k_spin_lock(&t->lock);
	switch (t->state) {
	case PTHREAD_DETACHED:
		ret = EINVAL;
		break;
	case PTHREAD_JOINABLE:
		k_spin_unlock(&t->lock, key);
		ret = k_thread_join(&t->thread, K_FOREVER);
		__ASSERT(ret == 0, "k_thread_join() failed: %d", ret);
		key = k_spin_lock(&t->lock);
		/* fall through */
	case PTHREAD_EXITED:
		ret = 0;
		should_free = true;
		t->state = PTHREAD_TERMINATED;
		if (status != NULL) {
			*status = t->retval;
		}
		z_thread_wake_joiners(&t->thread);
		break;
	case PTHREAD_TERMINATED:
		ret = ESRCH;
		break;
	default:
		__ASSERT(false, "bad pthread state %d", t->state);
		ret = ESRCH;
		break;
	}
	k_spin_unlock(&t->lock, key);

	if (should_free) {
		/* only free joinable threads on join */
		sys_bitarray_free(&posix_thread_bitarray, 1, bit);
	}

	return ret;
}

/**
 * @brief Detach a thread.
 *
 * See IEEE 1003.1
 */
int pthread_detach(pthread_t pthread)
{
	int ret = 0;
	bool join = false;
	k_spinlock_key_t key;
	struct posix_thread *t = to_posix_thread(pthread);

	if (t == NULL) {
		return ESRCH;
	}

	key = k_spin_lock(&t->lock);
	switch (t->state) {
	case PTHREAD_JOINABLE:
		t->state = PTHREAD_DETACHED;
		z_thread_wake_joiners(&t->thread);
		break;
	case PTHREAD_EXITED:
		join = true;
		break;
	case PTHREAD_TERMINATED:
		ret = ESRCH;
		break;
	default:
		ret = EINVAL;
		break;
	}
	k_spin_unlock(&t->lock, key);

	if (join) {
		pthread_join(pthread, NULL);
	}

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

	if ((attr == NULL) || (attr->initialized == 0U) || !valid_posix_policy(policy)) {
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

int pthread_setname_np(pthread_t pthread, const char *name)
{
#ifdef CONFIG_THREAD_NAME
	k_tid_t kthread;
	struct posix_thread *t = to_posix_thread(pthread);

	if (t == NULL) {
		return ESRCH;
	}

	kthread = &posix_thread_pool[pthread].thread;

	if (name == NULL) {
		return EINVAL;
	}

	return k_thread_name_set(kthread, name);
#else
	ARG_UNUSED(pthread);
	ARG_UNUSED(name);
	return 0;
#endif
}

int pthread_getname_np(pthread_t pthread, char *name, size_t len)
{
#ifdef CONFIG_THREAD_NAME
	k_tid_t kthread;
	struct posix_thread *t = to_posix_thread(pthread);

	if (t == NULL) {
		return ESRCH;
	}

	if (name == NULL) {
		return EINVAL;
	}

	memset(name, '\0', len);
	kthread = &posix_thread_pool[pthread].thread;
	return k_thread_name_copy(kthread, name, len-1);
#else
	ARG_UNUSED(pthread);
	ARG_UNUSED(name);
	ARG_UNUSED(len);
	return 0;
#endif
}

static int posix_thread_pool_init(void)
{
	size_t i;


	for (i = 0; i < CONFIG_MAX_PTHREAD_COUNT; ++i) {
		posix_thread_pool[i].state = PTHREAD_TERMINATED;
	}

	return 0;
}

SYS_INIT(posix_thread_pool_init, PRE_KERNEL_1, 0);
