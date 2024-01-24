/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_internal.h"
#include "pthread_sched.h"

#include <stdio.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#define ZEPHYR_TO_POSIX_PRIORITY(_zprio)                                                           \
	(((_zprio) < 0) ? (-1 * ((_zprio) + 1)) : (CONFIG_NUM_PREEMPT_PRIORITIES - (_zprio)-1))

#define POSIX_TO_ZEPHYR_PRIORITY(_prio, _pol)                                                      \
	(((_pol) == SCHED_FIFO) ? (-1 * ((_prio) + 1))                                             \
				: (CONFIG_NUM_PREEMPT_PRIORITIES - (_prio)-1))

#define DEFAULT_PTHREAD_PRIORITY                                                                   \
	POSIX_TO_ZEPHYR_PRIORITY(K_LOWEST_APPLICATION_THREAD_PRIO, DEFAULT_PTHREAD_POLICY)
#define DEFAULT_PTHREAD_POLICY (IS_ENABLED(CONFIG_PREEMPT_ENABLED) ? SCHED_RR : SCHED_FIFO)

#define PTHREAD_STACK_MAX BIT(CONFIG_POSIX_PTHREAD_ATTR_STACKSIZE_BITS)
#define PTHREAD_GUARD_MAX BIT_MASK(CONFIG_POSIX_PTHREAD_ATTR_GUARDSIZE_BITS)

LOG_MODULE_REGISTER(pthread, CONFIG_PTHREAD_LOG_LEVEL);

#ifdef CONFIG_DYNAMIC_THREAD_STACK_SIZE
#define DYNAMIC_STACK_SIZE CONFIG_DYNAMIC_THREAD_STACK_SIZE
#else
#define DYNAMIC_STACK_SIZE 0
#endif

static inline size_t __get_attr_stacksize(const struct posix_thread_attr *attr)
{
	return attr->stacksize + 1;
}

static inline void __set_attr_stacksize(struct posix_thread_attr *attr, size_t stacksize)
{
	attr->stacksize = stacksize - 1;
}

struct __pthread_cleanup {
	void (*routine)(void *arg);
	void *arg;
	sys_snode_t node;
};

enum posix_thread_qid {
	/* ready to be started via pthread_create() */
	POSIX_THREAD_READY_Q,
	/* running */
	POSIX_THREAD_RUN_Q,
	/* exited (either joinable or detached) */
	POSIX_THREAD_DONE_Q,
};

/* only 2 bits in struct posix_thread_attr for schedpolicy */
BUILD_ASSERT(SCHED_OTHER < BIT(2) && SCHED_FIFO < BIT(2) && SCHED_RR < BIT(2));

BUILD_ASSERT((PTHREAD_CREATE_DETACHED == 0 || PTHREAD_CREATE_JOINABLE == 0) &&
	     (PTHREAD_CREATE_DETACHED == 1 || PTHREAD_CREATE_JOINABLE == 1));

BUILD_ASSERT((PTHREAD_CANCEL_ENABLE == 0 || PTHREAD_CANCEL_DISABLE == 0) &&
	     (PTHREAD_CANCEL_ENABLE == 1 || PTHREAD_CANCEL_DISABLE == 1));

BUILD_ASSERT(CONFIG_POSIX_PTHREAD_ATTR_STACKSIZE_BITS + CONFIG_POSIX_PTHREAD_ATTR_GUARDSIZE_BITS <=
	     32);

static void posix_thread_recycle(void);
static sys_dlist_t ready_q = SYS_DLIST_STATIC_INIT(&ready_q);
static sys_dlist_t run_q = SYS_DLIST_STATIC_INIT(&run_q);
static sys_dlist_t done_q = SYS_DLIST_STATIC_INIT(&done_q);
static struct posix_thread posix_thread_pool[CONFIG_MAX_PTHREAD_COUNT];
static struct k_spinlock pthread_pool_lock;
static int pthread_concurrency;

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

struct posix_thread *to_posix_thread(pthread_t pthread)
{
	k_spinlock_key_t key;
	struct posix_thread *t;
	bool actually_initialized;
	size_t bit = get_posix_thread_idx(pthread);

	/* if the provided thread does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(pthread)) {
		LOG_ERR("pthread is not initialized (%x)", pthread);
		return NULL;
	}

	if (bit >= CONFIG_MAX_PTHREAD_COUNT) {
		LOG_ERR("Invalid pthread (%x)", pthread);
		return NULL;
	}

	t = &posix_thread_pool[bit];

	key = k_spin_lock(&pthread_pool_lock);
	/*
	 * Denote a pthread as "initialized" (i.e. allocated) if it is not in ready_q.
	 * This differs from other posix object allocation strategies because they use
	 * a bitarray to indicate whether an object has been allocated.
	 */
	actually_initialized =
		!(t->qid == POSIX_THREAD_READY_Q ||
		  (t->qid == POSIX_THREAD_DONE_Q && t->detachstate == PTHREAD_CREATE_DETACHED));
	k_spin_unlock(&pthread_pool_lock, key);

	if (!actually_initialized) {
		LOG_ERR("Pthread claims to be initialized (%x)", pthread);
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

int pthread_equal(pthread_t pt1, pthread_t pt2)
{
	return (pt1 == pt2);
}

pid_t getpid(void)
{
	/*
	 * To maintain compatibility with some other POSIX operating systems,
	 * a PID of zero is used to indicate that the process exists in another namespace.
	 * PID zero is also used by the scheduler in some cases.
	 * PID one is usually reserved for the init process.
	 * Also note, that negative PIDs may be used by kill()
	 * to send signals to process groups in some implementations.
	 *
	 * At the moment, getpid just returns an arbitrary number >= 2
	 */

	return 42;
}

static inline void __z_pthread_cleanup_init(struct __pthread_cleanup *c, void (*routine)(void *arg),
					    void *arg)
{
	*c = (struct __pthread_cleanup){
		.routine = routine,
		.arg = arg,
		.node = {0},
	};
}

void __z_pthread_cleanup_push(void *cleanup[3], void (*routine)(void *arg), void *arg)
{
	struct posix_thread *const t = to_posix_thread(pthread_self());
	struct __pthread_cleanup *const c = (struct __pthread_cleanup *)cleanup;

	BUILD_ASSERT(3 * sizeof(void *) == sizeof(*c));
	__ASSERT_NO_MSG(t != NULL);
	__ASSERT_NO_MSG(c != NULL);
	__ASSERT_NO_MSG(routine != NULL);
	__z_pthread_cleanup_init(c, routine, arg);
	sys_slist_prepend(&t->cleanup_list, &c->node);
}

void __z_pthread_cleanup_pop(int execute)
{
	sys_snode_t *node;
	struct __pthread_cleanup *c;
	struct posix_thread *const t = to_posix_thread(pthread_self());

	__ASSERT_NO_MSG(t != NULL);
	node = sys_slist_get(&t->cleanup_list);
	__ASSERT_NO_MSG(node != NULL);
	c = CONTAINER_OF(node, struct __pthread_cleanup, node);
	__ASSERT_NO_MSG(c != NULL);
	__ASSERT_NO_MSG(c->routine != NULL);
	if (execute) {
		c->routine(c->arg);
	}
}

static bool is_posix_policy_prio_valid(int priority, int policy)
{
	if (priority >= sched_get_priority_min(policy) &&
	    priority <= sched_get_priority_max(policy)) {
		return true;
	}

	LOG_ERR("Invalid priority %d and / or policy %d", priority, policy);

	return false;
}

/* Non-static so that they can be tested in ztest */
int zephyr_to_posix_priority(int z_prio, int *policy)
{
	int priority;

	if (z_prio < 0) {
		__ASSERT_NO_MSG(-z_prio <= CONFIG_NUM_COOP_PRIORITIES);
	} else {
		__ASSERT_NO_MSG(z_prio < CONFIG_NUM_PREEMPT_PRIORITIES);
	}

	*policy = (z_prio < 0) ? SCHED_FIFO : SCHED_RR;
	priority = ZEPHYR_TO_POSIX_PRIORITY(z_prio);
	__ASSERT_NO_MSG(is_posix_policy_prio_valid(priority, *policy));

	return priority;
}

/* Non-static so that they can be tested in ztest */
int posix_to_zephyr_priority(int priority, int policy)
{
	__ASSERT_NO_MSG(is_posix_policy_prio_valid(priority, policy));

	return POSIX_TO_ZEPHYR_PRIORITY(priority, policy);
}

static bool __attr_is_initialized(const struct posix_thread_attr *attr)
{
	return attr != NULL && attr->initialized;
}

/**
 * @brief Set scheduling parameter attributes in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_setschedparam(pthread_attr_t *_attr, const struct sched_param *schedparam)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || schedparam == NULL ||
	    !is_posix_policy_prio_valid(schedparam->sched_priority, attr->schedpolicy)) {
		LOG_ERR("Invalid pthread_attr_t or sched_param");
		return EINVAL;
	}

	attr->priority = schedparam->sched_priority;
	return 0;
}

/**
 * @brief Set stack attributes in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_setstack(pthread_attr_t *_attr, void *stackaddr, size_t stacksize)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (stackaddr == NULL) {
		LOG_ERR("NULL stack address");
		return EACCES;
	}

	if (!__attr_is_initialized(attr) || stacksize < PTHREAD_STACK_MIN ||
	    stacksize > PTHREAD_STACK_MAX) {
		LOG_ERR("Invalid stacksize %zu", stacksize);
		return EINVAL;
	}

	attr->stack = stackaddr;
	__set_attr_stacksize(attr, stacksize);
	return 0;
}

static bool pthread_attr_is_valid(const struct posix_thread_attr *attr)
{
	size_t stacksize;

	/* auto-alloc thread stack */
	if (attr == NULL) {
		return true;
	}

	if (!__attr_is_initialized(attr) || attr->stack == NULL) {
		LOG_DBG("attr %p is not initialized", attr);
		return false;
	}

	stacksize = __get_attr_stacksize(attr);
	if (stacksize < PTHREAD_STACK_MIN) {
		LOG_DBG("attr %p has stacksize %zu is smaller than PTHREAD_STACK_MIN (%zu)", attr,
			stacksize, (size_t)PTHREAD_STACK_MIN);
		return false;
	}

	/* require a valid scheduler policy */
	if (!valid_posix_policy(attr->schedpolicy)) {
		LOG_ERR("Invalid scheduler policy %d", attr->schedpolicy);
		return false;
	}

	return true;
}

static void posix_thread_recycle_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	posix_thread_recycle();
}
static K_WORK_DELAYABLE_DEFINE(posix_thread_recycle_work, posix_thread_recycle_work_handler);

static void posix_thread_finalize(struct posix_thread *t, void *retval)
{
	sys_snode_t *node_l;
	k_spinlock_key_t key;
	pthread_key_obj *key_obj;
	pthread_thread_data *thread_spec_data;

	SYS_SLIST_FOR_EACH_NODE(&t->key_list, node_l) {
		thread_spec_data = (pthread_thread_data *)node_l;
		if (thread_spec_data != NULL) {
			key_obj = thread_spec_data->key;
			if (key_obj->destructor != NULL) {
				(key_obj->destructor)(thread_spec_data->spec_data);
			}
		}
	}

	/* move thread from run_q to done_q */
	key = k_spin_lock(&pthread_pool_lock);
	sys_dlist_remove(&t->q_node);
	sys_dlist_append(&done_q, &t->q_node);
	t->qid = POSIX_THREAD_DONE_Q;
	t->retval = retval;
	k_spin_unlock(&pthread_pool_lock, key);

	/* trigger recycle work */
	(void)k_work_schedule(&posix_thread_recycle_work, K_MSEC(CONFIG_PTHREAD_RECYCLER_DELAY_MS));

	/* abort the underlying k_thread */
	k_thread_abort(&t->thread);
}

FUNC_NORETURN
static void zephyr_thread_wrapper(void *arg1, void *arg2, void *arg3)
{
	int err;
	int barrier;
	void *(*fun_ptr)(void *arg) = arg2;
	struct posix_thread *t = CONTAINER_OF(k_current_get(), struct posix_thread, thread);

	if (IS_ENABLED(CONFIG_PTHREAD_CREATE_BARRIER)) {
		/* cross the barrier so that pthread_create() can continue */
		barrier = POINTER_TO_UINT(arg3);
		err = pthread_barrier_wait(&barrier);
		__ASSERT_NO_MSG(err == 0 || err == PTHREAD_BARRIER_SERIAL_THREAD);
	}

	posix_thread_finalize(t, fun_ptr(arg1));

	CODE_UNREACHABLE;
}

static void posix_thread_recycle(void)
{
	k_spinlock_key_t key;
	struct posix_thread *t;
	struct posix_thread *safe_t;
	sys_dlist_t recyclables = SYS_DLIST_STATIC_INIT(&recyclables);

	key = k_spin_lock(&pthread_pool_lock);
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&done_q, t, safe_t, q_node) {
		if (t->detachstate == PTHREAD_CREATE_JOINABLE) {
			/* thread has not been joined yet */
			continue;
		}

		sys_dlist_remove(&t->q_node);
		sys_dlist_append(&recyclables, &t->q_node);
	}
	k_spin_unlock(&pthread_pool_lock, key);

	if (sys_dlist_is_empty(&recyclables)) {
		return;
	}

	LOG_DBG("Recycling %zu threads", sys_dlist_len(&recyclables));

	if (IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		SYS_DLIST_FOR_EACH_CONTAINER(&recyclables, t, q_node) {
			if (t->dynamic_stack != NULL) {
				LOG_DBG("Freeing thread stack %p", t->dynamic_stack);
				(void)k_thread_stack_free(t->dynamic_stack);
				t->dynamic_stack = NULL;
			}
		}
	}

	key = k_spin_lock(&pthread_pool_lock);
	while (!sys_dlist_is_empty(&recyclables)) {
		sys_dlist_append(&ready_q, sys_dlist_get(&recyclables));
	}
	k_spin_unlock(&pthread_pool_lock, key);
}

/**
 * @brief Create a new thread.
 *
 * Pthread attribute should not be NULL. API will return Error on NULL
 * attribute value.
 *
 * See IEEE 1003.1
 */
int pthread_create(pthread_t *th, const pthread_attr_t *_attr, void *(*threadroutine)(void *),
		   void *arg)
{
	int err;
	k_spinlock_key_t key;
	pthread_barrier_t barrier;
	struct posix_thread *t = NULL;
	struct posix_thread_attr attr_storage;
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!pthread_attr_is_valid(attr)) {
		return EINVAL;
	}

	if (attr == NULL) {
		attr = &attr_storage;
		(void)pthread_attr_init((pthread_attr_t *)attr);
		BUILD_ASSERT(DYNAMIC_STACK_SIZE <= PTHREAD_STACK_MAX);
		__set_attr_stacksize(attr, DYNAMIC_STACK_SIZE);
		attr->stack = k_thread_stack_alloc(__get_attr_stacksize(attr) + attr->guardsize,
						   k_is_user_context() ? K_USER : 0);
		if (attr->stack == NULL) {
			LOG_ERR("Unable to allocate stack of size %u", DYNAMIC_STACK_SIZE);
			return EAGAIN;
		}
		LOG_DBG("Allocated thread stack %p", attr->stack);
	} else {
		__ASSERT_NO_MSG(attr != &attr_storage);
	}

	/* reclaim resources greedily */
	posix_thread_recycle();

	key = k_spin_lock(&pthread_pool_lock);
	if (!sys_dlist_is_empty(&ready_q)) {
		t = CONTAINER_OF(sys_dlist_get(&ready_q), struct posix_thread, q_node);

		/* initialize thread state */
		sys_dlist_append(&run_q, &t->q_node);
		t->qid = POSIX_THREAD_RUN_Q;
		t->detachstate = attr->detachstate;
		t->cancel_state = attr->cancelstate;
		t->cancel_pending = false;
		sys_slist_init(&t->key_list);
		sys_slist_init(&t->cleanup_list);
		t->dynamic_stack = _attr == NULL ? attr->stack : NULL;
	}
	k_spin_unlock(&pthread_pool_lock, key);

	if (t == NULL) {
		/* no threads are ready */
		LOG_ERR("No threads are ready");
		return EAGAIN;
	}

	if (IS_ENABLED(CONFIG_PTHREAD_CREATE_BARRIER)) {
		err = pthread_barrier_init(&barrier, NULL, 2);
		if (err != 0) {
			if (t->dynamic_stack != NULL) {
				LOG_DBG("freeing thread stack at %p", attr->stack);
				(void)k_thread_stack_free(attr->stack);
			}

			/* cannot allocate barrier. move thread back to ready_q */
			key = k_spin_lock(&pthread_pool_lock);
			sys_dlist_remove(&t->q_node);
			sys_dlist_append(&ready_q, &t->q_node);
			t->qid = POSIX_THREAD_READY_Q;
			k_spin_unlock(&pthread_pool_lock, key);
			t = NULL;
		}
	}

	/* spawn the thread */
	k_thread_create(&t->thread, attr->stack, __get_attr_stacksize(attr), zephyr_thread_wrapper,
			(void *)arg, threadroutine,
			IS_ENABLED(CONFIG_PTHREAD_CREATE_BARRIER) ? UINT_TO_POINTER(barrier) : NULL,
			posix_to_zephyr_priority(attr->priority, attr->schedpolicy), 0, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_PTHREAD_CREATE_BARRIER)) {
		/* wait for the spawned thread to cross our barrier */
		err = pthread_barrier_wait(&barrier);
		__ASSERT_NO_MSG(err == 0 || err == PTHREAD_BARRIER_SERIAL_THREAD);
		err = pthread_barrier_destroy(&barrier);
		__ASSERT_NO_MSG(err == 0);
	}

	/* finally provide the initialized thread to the caller */
	*th = mark_pthread_obj_initialized(posix_thread_to_offset(t));

	LOG_DBG("Created pthread %p", &t->thread);

	return 0;
}

int pthread_getconcurrency(void)
{
	int ret = 0;

	K_SPINLOCK(&pthread_pool_lock) {
		ret = pthread_concurrency;
	}

	return ret;
}

int pthread_setconcurrency(int new_level)
{
	if (new_level < 0) {
		return EINVAL;
	}

	if (new_level > CONFIG_MP_MAX_NUM_CPUS) {
		return EAGAIN;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		pthread_concurrency = new_level;
	}

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
	struct posix_thread *t;

	if (state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE) {
		LOG_ERR("Invalid pthread state %d", state);
		return EINVAL;
	}

	t = to_posix_thread(pthread_self());
	if (t == NULL) {
		return EINVAL;
	}

	key = k_spin_lock(&pthread_pool_lock);
	if (oldstate != NULL) {
		*oldstate = t->cancel_state;
	}
	t->cancel_state = state;
	cancel_pending = t->cancel_pending;
	k_spin_unlock(&pthread_pool_lock, key);

	if (state == PTHREAD_CANCEL_ENABLE && cancel_pending) {
		posix_thread_finalize(t, PTHREAD_CANCELED);
	}

	return 0;
}

/**
 * @brief Set cancelability Type.
 *
 * See IEEE 1003.1
 */
int pthread_setcanceltype(int type, int *oldtype)
{
	k_spinlock_key_t key;
	struct posix_thread *t;

	if (type != PTHREAD_CANCEL_DEFERRED && type != PTHREAD_CANCEL_ASYNCHRONOUS) {
		LOG_ERR("Invalid pthread cancel type %d", type);
		return EINVAL;
	}

	t = to_posix_thread(pthread_self());
	if (t == NULL) {
		return EINVAL;
	}

	key = k_spin_lock(&pthread_pool_lock);
	if (oldtype != NULL) {
		*oldtype = t->cancel_type;
	}
	t->cancel_type = type;
	k_spin_unlock(&pthread_pool_lock, key);

	return 0;
}

/**
 * @brief Cancel execution of a thread.
 *
 * See IEEE 1003.1
 */
int pthread_cancel(pthread_t pthread)
{
	int cancel_state;
	int cancel_type;
	k_spinlock_key_t key;
	struct posix_thread *t;

	t = to_posix_thread(pthread);
	if (t == NULL) {
		return ESRCH;
	}

	key = k_spin_lock(&pthread_pool_lock);
	t->cancel_pending = true;
	cancel_state = t->cancel_state;
	cancel_type = t->cancel_type;
	k_spin_unlock(&pthread_pool_lock, key);

	if (cancel_state == PTHREAD_CANCEL_ENABLE &&
	    cancel_type == PTHREAD_CANCEL_ASYNCHRONOUS) {
		posix_thread_finalize(t, PTHREAD_CANCELED);
	}

	return 0;
}

/**
 * @brief Set thread scheduling policy and parameters.
 *
 * See IEEE 1003.1
 */
int pthread_setschedparam(pthread_t pthread, int policy, const struct sched_param *param)
{
	struct posix_thread *t = to_posix_thread(pthread);
	int new_prio;

	if (t == NULL) {
		return ESRCH;
	}

	if (!valid_posix_policy(policy)) {
		LOG_ERR("Invalid scheduler policy %d", policy);
		return EINVAL;
	}

	if (is_posix_policy_prio_valid(param->sched_priority, policy) == false) {
		return EINVAL;
	}

	new_prio = posix_to_zephyr_priority(param->sched_priority, policy);

	k_thread_priority_set(&t->thread, new_prio);
	return 0;
}

/**
 * @brief Initialise threads attribute object
 *
 * See IEEE 1003.1
 */
int pthread_attr_init(pthread_attr_t *_attr)
{
	struct posix_thread_attr *const attr = (struct posix_thread_attr *)_attr;

	if (attr == NULL) {
		LOG_ERR("Invalid attr pointer");
		return ENOMEM;
	}

	*attr = (struct posix_thread_attr){0};
	attr->guardsize = CONFIG_POSIX_PTHREAD_ATTR_GUARDSIZE_DEFAULT;
	attr->initialized = true;

	return 0;
}

/**
 * @brief Get thread scheduling policy and parameters
 *
 * See IEEE 1003.1
 */
int pthread_getschedparam(pthread_t pthread, int *policy, struct sched_param *param)
{
	int priority;
	struct posix_thread *t;

	t = to_posix_thread(pthread);
	if (t == NULL) {
		return ESRCH;
	}

	priority = k_thread_priority_get(&t->thread);

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
	__unused int ret;
	bool run_init_func = false;
	struct pthread_once *const _once = (struct pthread_once *)once;

	if (init_func == NULL) {
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		if (!_once->flag) {
			run_init_func = true;
			_once->flag = true;
		}
	}

	if (run_init_func) {
		init_func();
	}

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
	k_spinlock_key_t key;
	struct posix_thread *self;

	self = to_posix_thread(pthread_self());
	if (self == NULL) {
		/* not a valid posix_thread */
		LOG_DBG("Aborting non-pthread %p", k_current_get());
		k_thread_abort(k_current_get());

		CODE_UNREACHABLE;
	}

	/* Mark a thread as cancellable before exiting */
	key = k_spin_lock(&pthread_pool_lock);
	self->cancel_state = PTHREAD_CANCEL_ENABLE;
	k_spin_unlock(&pthread_pool_lock, key);

	posix_thread_finalize(self, retval);
	CODE_UNREACHABLE;
}

/**
 * @brief Wait for a thread termination.
 *
 * See IEEE 1003.1
 */
int pthread_join(pthread_t pthread, void **status)
{
	struct posix_thread *t;
	int ret;

	if (pthread == pthread_self()) {
		LOG_ERR("Pthread attempted to join itself (%x)", pthread);
		return EDEADLK;
	}

	t = to_posix_thread(pthread);
	if (t == NULL) {
		return ESRCH;
	}

	LOG_DBG("Pthread %p joining..", &t->thread);

	ret = 0;
	K_SPINLOCK(&pthread_pool_lock)
	{
		if (t->detachstate != PTHREAD_CREATE_JOINABLE) {
			ret = EINVAL;
			K_SPINLOCK_BREAK;
		}

		if (t->qid == POSIX_THREAD_READY_Q) {
			/* in case thread has moved to ready_q between to_posix_thread() and here */
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		/*
		 * thread is joinable and is in run_q or done_q.
		 * let's ensure that the thread cannot be joined again after this point.
		 */
		t->detachstate = PTHREAD_CREATE_DETACHED;
	}

	switch (ret) {
	case ESRCH:
		LOG_ERR("Pthread %p has already been joined", &t->thread);
		return ret;
	case EINVAL:
		LOG_ERR("Pthread %p is not a joinable", &t->thread);
		return ret;
	case 0:
		break;
	}

	ret = k_thread_join(&t->thread, K_FOREVER);
	/* other possibilities? */
	__ASSERT_NO_MSG(ret == 0);

	LOG_DBG("Joined pthread %p", &t->thread);

	if (status != NULL) {
		LOG_DBG("Writing status to %p", status);
		*status = t->retval;
	}

	posix_thread_recycle();

	return 0;
}

/**
 * @brief Detach a thread.
 *
 * See IEEE 1003.1
 */
int pthread_detach(pthread_t pthread)
{
	int ret;
	k_spinlock_key_t key;
	struct posix_thread *t;
	enum posix_thread_qid qid;

	t = to_posix_thread(pthread);
	if (t == NULL) {
		return ESRCH;
	}

	key = k_spin_lock(&pthread_pool_lock);
	qid = t->qid;
	if (qid == POSIX_THREAD_READY_Q || t->detachstate != PTHREAD_CREATE_JOINABLE) {
		LOG_ERR("Pthread %p cannot be detached", &t->thread);
		ret = EINVAL;
	} else {
		ret = 0;
		t->detachstate = PTHREAD_CREATE_DETACHED;
	}
	k_spin_unlock(&pthread_pool_lock, key);

	if (ret == 0) {
		LOG_DBG("Pthread %p detached", &t->thread);
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
	const struct posix_thread_attr *attr = (const struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || (detachstate == NULL)) {
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
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || ((detachstate != PTHREAD_CREATE_DETACHED) &&
					     (detachstate != PTHREAD_CREATE_JOINABLE))) {
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
	const struct posix_thread_attr *attr = (const struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || (policy == NULL)) {
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
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || !valid_posix_policy(policy)) {
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
	const struct posix_thread_attr *attr = (const struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || (stacksize == NULL)) {
		return EINVAL;
	}

	*stacksize = __get_attr_stacksize(attr);
	return 0;
}

/**
 * @brief Set stack size attribute in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_setstacksize(pthread_attr_t *_attr, size_t stacksize)
{
	int ret;
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || stacksize < PTHREAD_STACK_MIN ||
	    stacksize > PTHREAD_STACK_MAX) {
		return EINVAL;
	}

	__set_attr_stacksize(attr, stacksize);
	return 0;
}

/**
 * @brief Get stack attributes in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_getstack(const pthread_attr_t *_attr, void **stackaddr, size_t *stacksize)
{
	const struct posix_thread_attr *attr = (const struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || (stackaddr == NULL) || (stacksize == NULL)) {
		return EINVAL;
	}

	*stackaddr = attr->stack;
	*stacksize = __get_attr_stacksize(attr);
	return 0;
}

int pthread_attr_getguardsize(const pthread_attr_t *ZRESTRICT _attr, size_t *ZRESTRICT guardsize)
{
	struct posix_thread_attr *const attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || guardsize == NULL) {
		return EINVAL;
	}

	*guardsize = attr->guardsize;

	return 0;
}

int pthread_attr_setguardsize(pthread_attr_t *_attr, size_t guardsize)
{
	struct posix_thread_attr *const attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || guardsize > PTHREAD_GUARD_MAX) {
		return EINVAL;
	}

	attr->guardsize = guardsize;

	return 0;
}

/**
 * @brief Get thread attributes object scheduling parameters.
 *
 * See IEEE 1003.1
 */
int pthread_attr_getschedparam(const pthread_attr_t *_attr, struct sched_param *schedparam)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || (schedparam == NULL)) {
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
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr)) {
		return EINVAL;
	}

	return 0;
}

int pthread_setname_np(pthread_t thread, const char *name)
{
#ifdef CONFIG_THREAD_NAME
	k_tid_t kthread;

	thread = get_posix_thread_idx(thread);
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

	thread = get_posix_thread_idx(thread);
	if (thread >= CONFIG_MAX_PTHREAD_COUNT) {
		return ESRCH;
	}

	if (name == NULL) {
		return EINVAL;
	}

	memset(name, '\0', len);
	kthread = &posix_thread_pool[thread].thread;
	return k_thread_name_copy(kthread, name, len - 1);
#else
	ARG_UNUSED(thread);
	ARG_UNUSED(name);
	ARG_UNUSED(len);
	return 0;
#endif
}

int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
	ARG_UNUSED(prepare);
	ARG_UNUSED(parent);
	ARG_UNUSED(child);

	return ENOSYS;
}

/* this should probably go into signal.c but we need access to the lock */
int pthread_sigmask(int how, const sigset_t *ZRESTRICT set, sigset_t *ZRESTRICT oset)
{
	struct posix_thread *t;

	if (!(how == SIG_BLOCK || how == SIG_SETMASK || how == SIG_UNBLOCK)) {
		return EINVAL;
	}

	t = to_posix_thread(pthread_self());
	if (t == NULL) {
		return ESRCH;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		if (oset != NULL) {
			*oset = t->sigset;
		}

		if (set == NULL) {
			K_SPINLOCK_BREAK;
		}

		switch (how) {
		case SIG_BLOCK:
			for (size_t i = 0; i < ARRAY_SIZE(set->sig); ++i) {
				t->sigset.sig[i] |= set->sig[i];
			}
			break;
		case SIG_SETMASK:
			t->sigset = *set;
			break;
		case SIG_UNBLOCK:
			for (size_t i = 0; i < ARRAY_SIZE(set->sig); ++i) {
				t->sigset.sig[i] &= ~set->sig[i];
			}
			break;
		}
	}

	return 0;
}

static int posix_thread_pool_init(void)
{
	size_t i;

	for (i = 0; i < CONFIG_MAX_PTHREAD_COUNT; ++i) {
		sys_dlist_append(&ready_q, &posix_thread_pool[i].q_node);
	}

	return 0;
}
SYS_INIT(posix_thread_pool_init, PRE_KERNEL_1, 0);
