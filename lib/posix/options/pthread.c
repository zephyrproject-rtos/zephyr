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
	/* invalid */
	POSIX_THREAD_INVALID_Q,
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
static sys_dlist_t posix_thread_q[] = {
	SYS_DLIST_STATIC_INIT(&posix_thread_q[POSIX_THREAD_READY_Q]),
	SYS_DLIST_STATIC_INIT(&posix_thread_q[POSIX_THREAD_RUN_Q]),
	SYS_DLIST_STATIC_INIT(&posix_thread_q[POSIX_THREAD_DONE_Q]),
};
static struct posix_thread posix_thread_pool[CONFIG_POSIX_THREAD_THREADS_MAX];
static struct k_spinlock pthread_pool_lock;
static int pthread_concurrency;

static inline void posix_thread_q_set(struct posix_thread *t, enum posix_thread_qid qid)
{
	switch (qid) {
	case POSIX_THREAD_READY_Q:
	case POSIX_THREAD_RUN_Q:
	case POSIX_THREAD_DONE_Q:
		sys_dlist_append(&posix_thread_q[qid], &t->q_node);
		t->qid = qid;
		break;
	default:
		__ASSERT(false, "cannot set invalid qid %d for posix thread %p", qid, t);
		break;
	}
}

static inline enum posix_thread_qid posix_thread_q_get(struct posix_thread *t)
{
	switch (t->qid) {
	case POSIX_THREAD_READY_Q:
	case POSIX_THREAD_RUN_Q:
	case POSIX_THREAD_DONE_Q:
		return t->qid;
	default:
		__ASSERT(false, "posix thread %p has invalid qid: %d", t, t->qid);
		return POSIX_THREAD_INVALID_Q;
	}
}

/*
 * We reserve the MSB to mark a pthread_t as initialized (from the
 * perspective of the application). With a linear space, this means that
 * the theoretical pthread_t range is [0,2147483647].
 */
BUILD_ASSERT(CONFIG_POSIX_THREAD_THREADS_MAX < PTHREAD_OBJ_MASK_INIT,
	     "CONFIG_POSIX_THREAD_THREADS_MAX is too high");

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
	struct posix_thread *t;
	bool actually_initialized;
	size_t bit = get_posix_thread_idx(pthread);

	/* if the provided thread does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(pthread)) {
		LOG_DBG("pthread is not initialized (%x)", pthread);
		return NULL;
	}

	if (bit >= ARRAY_SIZE(posix_thread_pool)) {
		LOG_DBG("Invalid pthread (%x)", pthread);
		return NULL;
	}

	t = &posix_thread_pool[bit];

	/*
	 * Denote a pthread as "initialized" (i.e. allocated) if it is not in ready_q.
	 * This differs from other posix object allocation strategies because they use
	 * a bitarray to indicate whether an object has been allocated.
	 */
	actually_initialized = !(posix_thread_q_get(t) == POSIX_THREAD_READY_Q ||
				 (posix_thread_q_get(t) == POSIX_THREAD_DONE_Q &&
				  t->attr.detachstate == PTHREAD_CREATE_DETACHED));

	if (!actually_initialized) {
		LOG_DBG("Pthread claims to be initialized (%x)", pthread);
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
	struct posix_thread *t = NULL;
	struct __pthread_cleanup *const c = (struct __pthread_cleanup *)cleanup;

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread_self());
		BUILD_ASSERT(3 * sizeof(void *) == sizeof(*c));
		__ASSERT_NO_MSG(t != NULL);
		__ASSERT_NO_MSG(c != NULL);
		__ASSERT_NO_MSG(routine != NULL);
		__z_pthread_cleanup_init(c, routine, arg);
		sys_slist_prepend(&t->cleanup_list, &c->node);
	}
}

void __z_pthread_cleanup_pop(int execute)
{
	sys_snode_t *node;
	struct __pthread_cleanup *c = NULL;
	struct posix_thread *t = NULL;

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread_self());
		__ASSERT_NO_MSG(t != NULL);
		node = sys_slist_get(&t->cleanup_list);
		__ASSERT_NO_MSG(node != NULL);
		c = CONTAINER_OF(node, struct __pthread_cleanup, node);
		__ASSERT_NO_MSG(c != NULL);
		__ASSERT_NO_MSG(c->routine != NULL);
	}
	if (execute) {
		c->routine(c->arg);
	}
}

static bool is_posix_policy_prio_valid(int priority, int policy)
{
	if (priority >= posix_sched_priority_min(policy) &&
	    priority <= posix_sched_priority_max(policy)) {
		return true;
	}

	LOG_DBG("Invalid priority %d and / or policy %d", priority, policy);

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

static bool __attr_is_runnable(const struct posix_thread_attr *attr)
{
	size_t stacksize;

	if (attr == NULL || attr->stack == NULL) {
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
		LOG_DBG("Invalid scheduler policy %d", attr->schedpolicy);
		return false;
	}

	return true;
}

static bool __attr_is_initialized(const struct posix_thread_attr *attr)
{
	if (IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		return __attr_is_runnable(attr);
	}

	if (attr == NULL || !attr->initialized) {
		LOG_DBG("attr %p is not initialized", attr);
		return false;
	}

	return true;
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
		LOG_DBG("Invalid pthread_attr_t or sched_param");
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
	int ret;
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (stackaddr == NULL) {
		LOG_DBG("NULL stack address");
		return EACCES;
	}

	if (!__attr_is_initialized(attr) || stacksize == 0 || stacksize < PTHREAD_STACK_MIN ||
	    stacksize > PTHREAD_STACK_MAX) {
		LOG_DBG("Invalid stacksize %zu", stacksize);
		return EINVAL;
	}

	if (attr->stack != NULL) {
		ret = k_thread_stack_free(attr->stack);
		if (ret == 0) {
			LOG_DBG("Freed attr %p thread stack %zu@%p", _attr,
				__get_attr_stacksize(attr), attr->stack);
		}
	}

	attr->stack = stackaddr;
	__set_attr_stacksize(attr, stacksize);

	LOG_DBG("Assigned thread stack %zu@%p to attr %p", __get_attr_stacksize(attr), attr->stack,
		_attr);

	return 0;
}

/**
 * @brief Get scope attributes in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_getscope(const pthread_attr_t *_attr, int *contentionscope)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || contentionscope == NULL) {
		return EINVAL;
	}
	*contentionscope = attr->contentionscope;
	return 0;
}

/**
 * @brief Set scope attributes in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_setscope(pthread_attr_t *_attr, int contentionscope)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr)) {
		LOG_DBG("attr %p is not initialized", attr);
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
	attr->contentionscope = contentionscope;
	return 0;
}

/**
 * @brief Get inherit scheduler attributes in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_getinheritsched(const pthread_attr_t *_attr, int *inheritsched)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || inheritsched == NULL) {
		return EINVAL;
	}
	*inheritsched = attr->inheritsched;
	return 0;
}

/**
 * @brief Set inherit scheduler attributes in thread attributes object.
 *
 * See IEEE 1003.1
 */
int pthread_attr_setinheritsched(pthread_attr_t *_attr, int inheritsched)
{
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr)) {
		LOG_DBG("attr %p is not initialized", attr);
		return EINVAL;
	}

	if (inheritsched != PTHREAD_INHERIT_SCHED && inheritsched != PTHREAD_EXPLICIT_SCHED) {
		LOG_DBG("Invalid inheritsched %d", inheritsched);
		return EINVAL;
	}

	attr->inheritsched = inheritsched;
	return 0;
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
	posix_thread_q_set(t, POSIX_THREAD_DONE_Q);
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
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&posix_thread_q[POSIX_THREAD_DONE_Q], t, safe_t, q_node) {
		if (t->attr.detachstate == PTHREAD_CREATE_JOINABLE) {
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

	SYS_DLIST_FOR_EACH_CONTAINER(&recyclables, t, q_node) {
		if (t->attr.caller_destroys) {
			t->attr = (struct posix_thread_attr){0};
		} else {
			(void)pthread_attr_destroy((pthread_attr_t *)&t->attr);
		}
	}

	key = k_spin_lock(&pthread_pool_lock);
	while (!sys_dlist_is_empty(&recyclables)) {
		t = CONTAINER_OF(sys_dlist_get(&recyclables), struct posix_thread, q_node);
		posix_thread_q_set(t, POSIX_THREAD_READY_Q);
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
	pthread_barrier_t barrier;
	struct posix_thread *t = NULL;

	if (!(_attr == NULL || __attr_is_runnable((struct posix_thread_attr *)_attr))) {
		return EINVAL;
	}

	/* reclaim resources greedily */
	posix_thread_recycle();

	K_SPINLOCK(&pthread_pool_lock) {
		if (!sys_dlist_is_empty(&posix_thread_q[POSIX_THREAD_READY_Q])) {
			t = CONTAINER_OF(sys_dlist_get(&posix_thread_q[POSIX_THREAD_READY_Q]),
					 struct posix_thread, q_node);

			/* initialize thread state */
			posix_thread_q_set(t, POSIX_THREAD_RUN_Q);
			sys_slist_init(&t->key_list);
			sys_slist_init(&t->cleanup_list);
		}
	}

	if (t != NULL && IS_ENABLED(CONFIG_PTHREAD_CREATE_BARRIER)) {
		err = pthread_barrier_init(&barrier, NULL, 2);
		if (err != 0) {
			/* cannot allocate barrier. move thread back to ready_q */
			K_SPINLOCK(&pthread_pool_lock) {
				sys_dlist_remove(&t->q_node);
				posix_thread_q_set(t, POSIX_THREAD_READY_Q);
			}
			t = NULL;
		}
	}

	if (t == NULL) {
		/* no threads are ready */
		LOG_DBG("No threads are ready");
		return EAGAIN;
	}

	if (_attr == NULL) {
		err = pthread_attr_init((pthread_attr_t *)&t->attr);
		if (err == 0 && !__attr_is_runnable(&t->attr)) {
			(void)pthread_attr_destroy((pthread_attr_t *)&t->attr);
			err = EINVAL;
		}
		if (err != 0) {
			/* cannot allocate pthread attributes (e.g. stack) */
			K_SPINLOCK(&pthread_pool_lock) {
				sys_dlist_remove(&t->q_node);
				posix_thread_q_set(t, POSIX_THREAD_READY_Q);
			}
			return err;
		}
		/* caller not responsible for destroying attr */
		t->attr.caller_destroys = false;
	} else {
		/* copy user-provided attr into thread, caller must destroy attr at a later time */
		t->attr = *(struct posix_thread_attr *)_attr;
	}

	if (t->attr.inheritsched == PTHREAD_INHERIT_SCHED) {
		int pol;

		t->attr.priority =
			zephyr_to_posix_priority(k_thread_priority_get(k_current_get()), &pol);
		t->attr.schedpolicy = pol;
	}

	/* spawn the thread */
	k_thread_create(
		&t->thread, t->attr.stack, __get_attr_stacksize(&t->attr) + t->attr.guardsize,
		zephyr_thread_wrapper, (void *)arg, threadroutine,
		IS_ENABLED(CONFIG_PTHREAD_CREATE_BARRIER) ? UINT_TO_POINTER(barrier) : NULL,
		posix_to_zephyr_priority(t->attr.priority, t->attr.schedpolicy), 0, K_NO_WAIT);

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
	int ret = 0;
	struct posix_thread *t;
	bool cancel_pending = false;
	bool cancel_type = PTHREAD_CANCEL_ENABLE;

	if (state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE) {
		LOG_DBG("Invalid pthread state %d", state);
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread_self());
		if (t == NULL) {
			ret = EINVAL;
			K_SPINLOCK_BREAK;
		}

		if (oldstate != NULL) {
			*oldstate = t->attr.cancelstate;
		}

		t->attr.cancelstate = state;
		cancel_pending = t->attr.cancelpending;
		cancel_type = t->attr.canceltype;
	}

	if (state == PTHREAD_CANCEL_ENABLE && cancel_type == PTHREAD_CANCEL_ASYNCHRONOUS &&
	    cancel_pending) {
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
	int ret = 0;
	struct posix_thread *t;

	if (type != PTHREAD_CANCEL_DEFERRED && type != PTHREAD_CANCEL_ASYNCHRONOUS) {
		LOG_DBG("Invalid pthread cancel type %d", type);
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread_self());
		if (t == NULL) {
			ret = EINVAL;
			K_SPINLOCK_BREAK;
		}

		if (oldtype != NULL) {
			*oldtype = t->attr.canceltype;
		}
		t->attr.canceltype = type;
	}

	return ret;
}

/**
 * @brief Create a cancellation point in the calling thread.
 *
 * See IEEE 1003.1
 */
void pthread_testcancel(void)
{
	struct posix_thread *t;
	bool cancel_pended = false;

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread_self());
		if (t == NULL) {
			K_SPINLOCK_BREAK;
		}
		if (t->attr.cancelstate != PTHREAD_CANCEL_ENABLE) {
			K_SPINLOCK_BREAK;
		}
		if (t->attr.cancelpending) {
			cancel_pended = true;
			t->attr.cancelstate = PTHREAD_CANCEL_DISABLE;
		}
	}

	if (cancel_pended) {
		posix_thread_finalize(t, PTHREAD_CANCELED);
	}
}

/**
 * @brief Cancel execution of a thread.
 *
 * See IEEE 1003.1
 */
int pthread_cancel(pthread_t pthread)
{
	int ret = 0;
	bool cancel_state = PTHREAD_CANCEL_ENABLE;
	bool cancel_type = PTHREAD_CANCEL_DEFERRED;
	struct posix_thread *t = NULL;

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		if (!__attr_is_initialized(&t->attr)) {
			/* thread has already terminated */
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		t->attr.cancelpending = true;
		cancel_state = t->attr.cancelstate;
		cancel_type = t->attr.canceltype;
	}

	if (ret == 0 && cancel_state == PTHREAD_CANCEL_ENABLE &&
	    cancel_type == PTHREAD_CANCEL_ASYNCHRONOUS) {
		posix_thread_finalize(t, PTHREAD_CANCELED);
	}

	return ret;
}

/**
 * @brief Set thread scheduling policy and parameters.
 *
 * See IEEE 1003.1
 */
int pthread_setschedparam(pthread_t pthread, int policy, const struct sched_param *param)
{
	int ret = 0;
	int new_prio = K_LOWEST_APPLICATION_THREAD_PRIO;
	struct posix_thread *t = NULL;

	if (param == NULL || !valid_posix_policy(policy) ||
	    !is_posix_policy_prio_valid(param->sched_priority, policy)) {
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		new_prio = posix_to_zephyr_priority(param->sched_priority, policy);
	}

	if (ret == 0) {
		k_thread_priority_set(&t->thread, new_prio);
	}

	return ret;
}

/**
 * @brief Set thread scheduling priority.
 *
 * See IEEE 1003.1
 */
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

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(thread);
		if (t == NULL) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		new_prio = posix_to_zephyr_priority(prio, policy);
	}

	if (ret == 0) {
		k_thread_priority_set(&t->thread, new_prio);
	}

	return ret;
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
		LOG_DBG("Invalid attr pointer");
		return ENOMEM;
	}

	BUILD_ASSERT(DYNAMIC_STACK_SIZE <= PTHREAD_STACK_MAX);

	*attr = (struct posix_thread_attr){0};
	attr->guardsize = CONFIG_POSIX_PTHREAD_ATTR_GUARDSIZE_DEFAULT;
	attr->contentionscope = PTHREAD_SCOPE_SYSTEM;
	attr->inheritsched = PTHREAD_INHERIT_SCHED;

	if (DYNAMIC_STACK_SIZE > 0) {
		attr->stack = k_thread_stack_alloc(DYNAMIC_STACK_SIZE + attr->guardsize,
						   k_is_user_context() ? K_USER : 0);
		if (attr->stack == NULL) {
			LOG_DBG("Did not auto-allocate thread stack");
		} else {
			__set_attr_stacksize(attr, DYNAMIC_STACK_SIZE);
			__ASSERT_NO_MSG(__attr_is_initialized(attr));
			LOG_DBG("Allocated thread stack %zu@%p", __get_attr_stacksize(attr),
				attr->stack);
		}
	}

	/* caller responsible for destroying attr */
	attr->initialized = true;

	LOG_DBG("Initialized attr %p", _attr);

	return 0;
}

/**
 * @brief Get thread scheduling policy and parameters
 *
 * See IEEE 1003.1
 */
int pthread_getschedparam(pthread_t pthread, int *policy, struct sched_param *param)
{
	int ret = 0;
	struct posix_thread *t;

	if (policy == NULL || param == NULL) {
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		if (!__attr_is_initialized(&t->attr)) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		param->sched_priority =
			zephyr_to_posix_priority(k_thread_priority_get(&t->thread), policy);
	}

	return ret;
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
	struct posix_thread *self = NULL;

	K_SPINLOCK(&pthread_pool_lock) {
		self = to_posix_thread(pthread_self());
		if (self == NULL) {
			K_SPINLOCK_BREAK;
		}

		/* Mark a thread as cancellable before exiting */
		self->attr.cancelstate = PTHREAD_CANCEL_ENABLE;
	}

	if (self == NULL) {
		/* not a valid posix_thread */
		LOG_DBG("Aborting non-pthread %p", k_current_get());
		k_thread_abort(k_current_get());

		CODE_UNREACHABLE;
	}

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
	int ret = 0;
	struct posix_thread *t = NULL;

	if (pthread == pthread_self()) {
		LOG_DBG("Pthread attempted to join itself (%x)", pthread);
		return EDEADLK;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		LOG_DBG("Pthread %p joining..", &t->thread);

		if (t->attr.detachstate != PTHREAD_CREATE_JOINABLE) {
			/* undefined behaviour */
			ret = EINVAL;
			K_SPINLOCK_BREAK;
		}

		if (posix_thread_q_get(t) == POSIX_THREAD_READY_Q) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		/*
		 * thread is joinable and is in run_q or done_q.
		 * let's ensure that the thread cannot be joined again after this point.
		 */
		t->attr.detachstate = PTHREAD_CREATE_DETACHED;
	}

	switch (ret) {
	case ESRCH:
		LOG_DBG("Pthread %p has already been joined", &t->thread);
		return ret;
	case EINVAL:
		LOG_DBG("Pthread %p is not a joinable", &t->thread);
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
	int ret = 0;
	struct posix_thread *t;

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		if (posix_thread_q_get(t) == POSIX_THREAD_READY_Q ||
		    t->attr.detachstate != PTHREAD_CREATE_JOINABLE) {
			LOG_DBG("Pthread %p cannot be detached", &t->thread);
			ret = EINVAL;
			K_SPINLOCK_BREAK;
		}

		t->attr.detachstate = PTHREAD_CREATE_DETACHED;
	}

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
	void *new_stack;
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr) || stacksize == 0 || stacksize < PTHREAD_STACK_MIN ||
	    stacksize > PTHREAD_STACK_MAX) {
		return EINVAL;
	}

	if (__get_attr_stacksize(attr) == stacksize) {
		return 0;
	}

	new_stack =
		k_thread_stack_alloc(stacksize + attr->guardsize, k_is_user_context() ? K_USER : 0);
	if (new_stack == NULL) {
		if (stacksize < __get_attr_stacksize(attr)) {
			__set_attr_stacksize(attr, stacksize);
			return 0;
		}

		LOG_DBG("k_thread_stack_alloc(%zu) failed",
			__get_attr_stacksize(attr) + attr->guardsize);
		return ENOMEM;
	}
	LOG_DBG("Allocated thread stack %zu@%p", stacksize + attr->guardsize, new_stack);

	if (attr->stack != NULL) {
		ret = k_thread_stack_free(attr->stack);
		if (ret == 0) {
			LOG_DBG("Freed attr %p thread stack %zu@%p", _attr,
				__get_attr_stacksize(attr), attr->stack);
		}
	}

	__set_attr_stacksize(attr, stacksize);
	attr->stack = new_stack;

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
	int ret;
	struct posix_thread_attr *attr = (struct posix_thread_attr *)_attr;

	if (!__attr_is_initialized(attr)) {
		return EINVAL;
	}

	ret = k_thread_stack_free(attr->stack);
	if (ret == 0) {
		LOG_DBG("Freed attr %p thread stack %zu@%p", _attr, __get_attr_stacksize(attr),
			attr->stack);
	}

	*attr = (struct posix_thread_attr){0};
	LOG_DBG("Destroyed attr %p", _attr);

	return 0;
}

int pthread_setname_np(pthread_t thread, const char *name)
{
#ifdef CONFIG_THREAD_NAME
	k_tid_t kthread;

	thread = get_posix_thread_idx(thread);
	if (thread >= ARRAY_SIZE(posix_thread_pool)) {
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
	if (thread >= ARRAY_SIZE(posix_thread_pool)) {
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
	int ret = 0;
	struct posix_thread *t;

	if (!(how == SIG_BLOCK || how == SIG_SETMASK || how == SIG_UNBLOCK)) {
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread_self());
		if (t == NULL) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

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

	return ret;
}

static int posix_thread_pool_init(void)
{
	ARRAY_FOR_EACH_PTR(posix_thread_pool, th) {
		posix_thread_q_set(th, POSIX_THREAD_READY_Q);
	}

	return 0;
}
SYS_INIT(posix_thread_pool_init, PRE_KERNEL_1, 0);
