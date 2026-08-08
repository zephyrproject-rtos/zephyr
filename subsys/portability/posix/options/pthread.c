/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_clock.h"
#include "posix_internal.h"
#include "pthread_sched.h"

#include <stdio.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sem.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <kthread.h>

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
/* 0==DYNAMIC_STACK_SIZE renders attribute useless for `pthread_create`.
 * Keep this use-case for backward-compatibility (ZTests).
 */
#define DYNAMIC_STACK_SIZE 0
#endif

static inline size_t __get_attr_stacksize(const struct posix_thread_attr *attr)
{
	return attr->is_external ? 0 : attr->stacksize + 1;
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
	/* running pthread thread */
	POSIX_THREAD_RUN_Q,
	/* exited (either joinable or detached) */
	POSIX_THREAD_DONE_Q,
	/* running external thread */
	POSIX_THREAD_EXTERNAL_Q,
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

/* ensure `pthread_attr_t` covers memory requirements of private `struct posix_thread_attr` */
BUILD_ASSERT(sizeof(struct posix_thread_attr) <= sizeof(pthread_attr_t));

/* ensure unsigned `pthread_t` type is sufficient as the private `posix_thread_pool` index. */
BUILD_ASSERT(CONFIG_POSIX_THREAD_THREADS_MAX + 1 <= (pthread_t)~0);
BUILD_ASSERT(PTHREAD_OBJ_MASK_INIT <= (pthread_t)~0);

static size_t posix_thread_recycle(void);

static sys_dlist_t posix_thread_q[] = {
	SYS_DLIST_STATIC_INIT(&posix_thread_q[POSIX_THREAD_READY_Q]),
	SYS_DLIST_STATIC_INIT(&posix_thread_q[POSIX_THREAD_RUN_Q]),
	SYS_DLIST_STATIC_INIT(&posix_thread_q[POSIX_THREAD_DONE_Q]),
	SYS_DLIST_STATIC_INIT(&posix_thread_q[POSIX_THREAD_EXTERNAL_Q]),
};

/* entries = CONFIG_POSIX_THREAD_THREADS_MAX + 1 for main-thread */
static struct posix_thread posix_thread_pool[CONFIG_POSIX_THREAD_THREADS_MAX + 1];

static struct k_spinlock pthread_pool_lock;
static int pthread_concurrency;

static inline void posix_thread_q_set(struct posix_thread *t, enum posix_thread_qid qid)
{
	switch (qid) {
	case POSIX_THREAD_READY_Q:
	case POSIX_THREAD_RUN_Q:
	case POSIX_THREAD_DONE_Q:
	case POSIX_THREAD_EXTERNAL_Q:
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
	case POSIX_THREAD_EXTERNAL_Q:
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

/** Return true if given `pth` is inside `posix_thread_pool`, otherwise false. */
static inline bool is_valid_posix_thread_ptr(struct posix_thread *pth)
{
	return posix_thread_to_offset(pth) < ARRAY_SIZE(posix_thread_pool);
}

/** Converts given `struct posix_thread *` to its public `pthread_t` ID. */
pthread_t to_pthread_id(struct posix_thread *pth)
{
	return mark_pthread_obj_initialized(posix_thread_to_offset(pth));
}

/* Converts given public `pthread_t` to `struct posix_thread *` only if valid and initialized. */
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
	const enum posix_thread_qid qid = posix_thread_q_get(t);

	actually_initialized =
		qid == POSIX_THREAD_EXTERNAL_Q ||
		!(qid == POSIX_THREAD_READY_Q ||
		  (qid == POSIX_THREAD_DONE_Q && t->attr.detachstate == PTHREAD_CREATE_DETACHED));

	if (!actually_initialized) {
		LOG_DBG("Pthread claims to be initialized (%x)", pthread);
		return NULL;
	}

	return &posix_thread_pool[bit];
}

/* Converts given public `pthread_t` to Zephyr `k_tid_t`, only if valid and initialized. */
k_tid_t pthread_to_zephyr_thread(pthread_t pthread)
{
	struct posix_thread *th = to_posix_thread(pthread);

	return th ? th->z_thread : NULL;
}

/** Converts given public `pthread_t` to Zephyr `k_tid_t`, only if valid (maybe uninitialized). */
static k_tid_t pthread_to_zephyr_thread_any(pthread_t pthread)
{
	/* if the provided thread does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(pthread)) {
		return NULL;
	}
	const size_t idx = get_posix_thread_idx(pthread);

	if (idx >= ARRAY_SIZE(posix_thread_pool)) {
		return NULL;
	}
	return posix_thread_pool[idx].z_thread;
}

static void posix_thread_init_thread_state(struct posix_thread *t, bool is_external)
{
	if (is_external) {
		posix_thread_q_set(t, POSIX_THREAD_EXTERNAL_Q);
	} else {
		posix_thread_q_set(t, POSIX_THREAD_RUN_Q);
	}
	sys_slist_init(&t->key_list);
	sys_slist_init(&t->cleanup_list);
}

static bool __attr_is_runnable(const struct posix_thread_attr *attr)
{
	size_t stacksize;

	if (attr == NULL || (!attr->is_external && attr->stack == NULL)) {
		LOG_DBG("attr %p is not initialized", attr);
		return false;
	}

	if (!attr->is_external) {
		stacksize = __get_attr_stacksize(attr);
		if (stacksize < PTHREAD_STACK_MIN) {
			LOG_DBG("attr %p has stacksize %zu is smaller than " /* NOSONAR */
				"PTHREAD_STACK_MIN (%zu)",
				attr, stacksize, (size_t)PTHREAD_STACK_MIN);
			return false;
		}
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

static int posix_thread_attr_init(struct posix_thread_attr *const attr, bool is_external)
{
	if (attr == NULL) {
		LOG_DBG("Invalid attr pointer");
		return ENOMEM;
	}

	BUILD_ASSERT(DYNAMIC_STACK_SIZE <= PTHREAD_STACK_MAX);

	*attr = (struct posix_thread_attr){0};
	attr->guardsize = CONFIG_POSIX_PTHREAD_ATTR_GUARDSIZE_DEFAULT;
	attr->contentionscope = PTHREAD_SCOPE_SYSTEM;
	attr->inheritsched = PTHREAD_INHERIT_SCHED;
	if (is_external) {
		attr->is_external = true;
		attr->cancelstate = PTHREAD_CANCEL_DISABLE;
		attr->detachstate = PTHREAD_CREATE_DETACHED;
	} else if (DYNAMIC_STACK_SIZE > 0) { /* NOSONAR */
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

	LOG_DBG("Initialized attr %p", attr); /* NOSONAR */

	return 0;
}

static void posix_thread_move_to_readyq(struct posix_thread *t, bool acquire_lock)
{
	if (acquire_lock) {
		K_SPINLOCK(&pthread_pool_lock) {
			sys_dlist_remove(&t->q_node);
			t->z_thread = NULL;
			posix_thread_q_set(t, POSIX_THREAD_READY_Q);
		}
	} else {
		sys_dlist_remove(&t->q_node);
		t->z_thread = NULL;
		posix_thread_q_set(t, POSIX_THREAD_READY_Q);
	}
}

static int posix_thread_create_attr(struct posix_thread *t, bool is_external)
{
	int err = posix_thread_attr_init(&t->attr, is_external);

	if (err == 0 && !__attr_is_runnable(&t->attr)) {
		(void)pthread_attr_destroy((pthread_attr_t *)&t->attr);
		err = EINVAL;
	}
	if (err != 0) {
		/* cannot allocate pthread attributes (e.g. stack) */
		posix_thread_move_to_readyq(t, !is_external);
		LOG_ERR("pthread_create_attr: failed (ext %d)", is_external); /* NOSONAR */
		return err;
	}
	/* caller not responsible for destroying attr */
	t->attr.caller_destroys = false;
	return err;
}

static void posix_thread_inherit_sched(int z_prio, struct posix_thread *t)
{
	int pol;

	t->attr.priority = zephyr_to_posix_priority(z_prio, &pol);
	t->attr.schedpolicy = pol;
}

/**
 * Add one posix_thread_pool entry for the Zephyr external thread READY -> EXTERNAL
 * and initialize its attributes w/ states:PTHREAD_CANCEL_DISABLE, PTHREAD_CREATE_DETACHED.
 *
 * Caller is required to lock pthread_pool_lock post initialization.
 *
 * @return valid added reference or NULL if READY queue is empty
 */
static struct posix_thread *posix_thread_add_external(k_tid_t zexternal)
{
	struct posix_thread *pexternal = NULL;

	if (!sys_dlist_is_empty(&posix_thread_q[POSIX_THREAD_READY_Q])) {
		pexternal = CONTAINER_OF(/* NOSONAR */
					 sys_dlist_get(&posix_thread_q[POSIX_THREAD_READY_Q]),
					 struct posix_thread, q_node);
		posix_thread_init_thread_state(pexternal, true);
	} else {
		LOG_ERR("pthread_add_external: Empty POSIX_THREAD_READY_Q" /* NOSONAR */
			" (CONFIG_POSIX_THREAD_THREADS_MAX)");
		return NULL;
	}
	if (posix_thread_create_attr(pexternal, true) != 0) {
		return NULL;
	}
	pexternal->z_thread = zexternal;

	__ASSERT_NO_MSG(pexternal == to_posix_thread(to_pthread_id(pexternal)));
	__ASSERT_NO_MSG(zexternal == pthread_to_zephyr_thread(to_pthread_id(pexternal)));
	return pexternal;
}

static pthread_t posix_thread_self(bool lock)
{
#ifdef CONFIG_CURRENT_THREAD_USE_TLS
	static Z_THREAD_LOCAL struct posix_thread *posix_thread_tls_current =
		(struct posix_thread *)0;
#endif
	struct posix_thread *t;

#ifdef CONFIG_CURRENT_THREAD_USE_TLS
	/* fast path TLS */
	t = posix_thread_tls_current;
	if (t && is_valid_posix_thread_ptr(t)) {
		LOG_DBG("pthread_self: %p (tls, ext %d)", /* NOSONAR */
			t->z_thread, (int)t->attr.is_external);
		return to_pthread_id(t);
	}
#endif
	struct k_thread *kself = k_current_get();

	/* existing pthread */
	t = (struct posix_thread *)CONTAINER_OF(kself, struct posix_thread, thread); /* NOSONAR */
	if (is_valid_posix_thread_ptr(t)) {
#ifdef CONFIG_CURRENT_THREAD_USE_TLS
		posix_thread_tls_current = t;
#endif
		LOG_DBG("pthread_self: %p (existing-p, ext %d)", t->z_thread, /* NOSONAR */
			(int)t->attr.is_external);
		return to_pthread_id(t);
	}
	pthread_t res = 0;
	k_spinlock_key_t key;

	if (lock) {
		key = k_spin_lock(&pthread_pool_lock);
	}
	/* existing external thread */
	SYS_DLIST_FOR_EACH_CONTAINER(/* NOSONAR */
				     &posix_thread_q[POSIX_THREAD_EXTERNAL_Q], t, q_node) {
		if (t->z_thread == kself) {
			res = to_pthread_id(t);
#ifdef CONFIG_CURRENT_THREAD_USE_TLS
			posix_thread_tls_current = t;
#endif
			LOG_DBG("pthread_self: %p (existing-e, ext %d)", /* NOSONAR */
				t->z_thread, (int)t->attr.is_external);
			break;
		}
	}
	/* new external thread */
	if (!res) {
		t = posix_thread_add_external(kself);
		if (!t) {
			LOG_ERR(/* NOSONAR */
				"pthread_self: Failed to add external thread");
			k_fatal_halt(K_ERR_KERNEL_OOPS);
		}
#ifdef CONFIG_CURRENT_THREAD_USE_TLS
		posix_thread_tls_current = t;
#endif
		posix_thread_inherit_sched(k_thread_priority_get(kself), t);
		res = to_pthread_id(t);
		LOG_DBG("pthread_self: %p (new-e, ext %d)", t->z_thread, /* NOSONAR */
			(int)t->attr.is_external);
	}
	if (lock) {
		k_spin_unlock(&pthread_pool_lock, key);
	}
	return res;
}

pthread_t pthread_self(void)
{
	return posix_thread_self(true);
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
		t = to_posix_thread(posix_thread_self(false));
		BUILD_ASSERT(3 * sizeof(void *) == sizeof(*c));
		__ASSERT_NO_MSG(t != NULL);
		__ASSERT_NO_MSG(c != NULL);
		__ASSERT_NO_MSG(routine != NULL);
		__z_pthread_cleanup_init(c, routine, arg);
		sys_slist_prepend(&t->cleanup_list, &c->node);
	}
}

static struct __pthread_cleanup *posix_thread_cleanup_pop(sys_slist_t *cleanup_listp)
{
	sys_snode_t *node = sys_slist_get(cleanup_listp);

	__ASSERT_NO_MSG(node != NULL);
	struct __pthread_cleanup *c =
		CONTAINER_OF(node, struct __pthread_cleanup, node); /* NOSONAR */

	__ASSERT_NO_MSG(c != NULL);
	__ASSERT_NO_MSG(c->routine != NULL);
	return c;
}

void __z_pthread_cleanup_pop(int execute)
{
	struct __pthread_cleanup *c = NULL;
	struct posix_thread *t = NULL;

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(posix_thread_self(false));
		__ASSERT_NO_MSG(t != NULL);
		c = posix_thread_cleanup_pop(&t->cleanup_list);
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
		LOG_DBG("Invalid pthread_attr_t or sched_param"); /* NOSONAR */
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
		LOG_DBG("NULL stack address"); /* NOSONAR */
		return EACCES;
	}

	if (!__attr_is_initialized(attr)) {
		return EINVAL;
	} else if (attr->is_external) {
		LOG_DBG("Not supported on external thread"); /* NOSONAR */
		return EINVAL;
	} else if ((stacksize == 0) || (PTHREAD_STACK_MIN > 0 && stacksize < PTHREAD_STACK_MIN) ||
		   (stacksize > PTHREAD_STACK_MAX)) {
		LOG_DBG("Invalid stacksize %zu", stacksize); /* NOSONAR */
		return EINVAL;
	}

	if (attr->stack != NULL) {
		ret = k_thread_stack_free(attr->stack);
		if (ret == 0) {
			LOG_DBG("Freed attr %p thread stack %zu@%p", _attr, /* NOSONAR */
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

extern struct sys_sem pthread_key_lock;

static void posix_thread_finalize_cleanuplist(struct posix_thread *t, bool exec_cleanup)
{
	if (exec_cleanup) {
		sys_slist_t cleanup_list = SYS_SLIST_STATIC_INIT(&cleanup_list);

		K_SPINLOCK(&pthread_pool_lock) {
			while (!sys_slist_is_empty(&t->cleanup_list)) {
				sys_slist_append(&cleanup_list, sys_slist_get(&t->cleanup_list));
			}
		}
		struct __pthread_cleanup *c;

		while (!sys_slist_is_empty(&cleanup_list)) {
			c = posix_thread_cleanup_pop(&cleanup_list);
			c->routine(c->arg);
		}
	} else {
		/* Avoid access to potentially unwound `__pthread_cleanup` stack entries,
		 * possible via `return`, `goto`, `longjmp`, etc.
		 */
		K_SPINLOCK(&pthread_pool_lock) {
			sys_slist_init(&t->cleanup_list);
		}
	}
}

static void posix_thread_finalize_keyvaluepairs(struct posix_thread *t)
{
	sys_snode_t *node_l, *node_s;
	pthread_key_obj *key_obj;
	pthread_thread_data *thread_spec_data;
	sys_snode_t *node_key_data, *node_key_data_s, *node_key_data_prev = NULL;
	struct pthread_key_data *key_data;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&t->key_list, node_l, node_s) {
		thread_spec_data = (pthread_thread_data *)node_l;
		if (thread_spec_data != NULL) {
			key_obj = thread_spec_data->key;
			if (key_obj->destructor != NULL) {
				(key_obj->destructor)(thread_spec_data->spec_data);
			}

			SYS_SEM_LOCK(&pthread_key_lock) {
				SYS_SLIST_FOR_EACH_NODE_SAFE(
					&key_obj->key_data_l,
					node_key_data,
					node_key_data_s) {
					key_data = (struct pthread_key_data *)node_key_data;
					if (&key_data->thread_data == thread_spec_data) {
						sys_slist_remove(
							&key_obj->key_data_l,
							node_key_data_prev,
							node_key_data
						);
						k_free(key_data);
						break;
					}
					node_key_data_prev = node_key_data;
				}
			}
		}
	}
}

static void posix_thread_finalize(struct posix_thread *t, bool exec_cleanup, void *retval)
{
	k_tid_t z_thread = NULL;

	posix_thread_finalize_cleanuplist(t, exec_cleanup);
	posix_thread_finalize_keyvaluepairs(t);

	/* move thread from run_q to done_q */
	K_SPINLOCK(&pthread_pool_lock) {
		sys_dlist_remove(&t->q_node);
		t->retval = retval;
		z_thread = t->z_thread;
		t->z_thread = NULL;
		posix_thread_q_set(t, POSIX_THREAD_DONE_Q);
	}
	__ASSERT_NO_MSG(NULL != z_thread);

	/* trigger recycle work */
	(void)k_work_schedule(&posix_thread_recycle_work, K_MSEC(CONFIG_PTHREAD_RECYCLER_DELAY_MS));

	/* Note: `z_main_thread` ends up with `k_panic` if `essential`. */
	LOG_DBG("pthread_finalize: aborting %p", z_thread); /* NOSONAR */
	k_thread_abort(z_thread);
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

	/* No execution of cleanup-handler on normal return from user-function. */
	posix_thread_finalize(t, false, fun_ptr(arg1));

	CODE_UNREACHABLE;
}

/* Called by halt_thread (sched.c), in any case of thread-death.
 * - `_sched_spinlock` is acquired by caller
 * - if given kthread refers to a registered external thread, it gets recycled immediately
 */
void posix_thread_kthread_exit(struct k_thread *kthread)
{
	struct posix_thread *t, *res = NULL;

	t = (struct posix_thread *)CONTAINER_OF(/* NOSONAR */
						kthread, struct posix_thread, thread);
	if (is_valid_posix_thread_ptr(t)) {
		return;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		/* existing external thread */
		SYS_DLIST_FOR_EACH_CONTAINER(/* NOSONAR */
					     &posix_thread_q[POSIX_THREAD_EXTERNAL_Q], t, q_node) {
			if (t->z_thread == kthread) {
				res = t;
				sys_dlist_remove(&t->q_node);
				LOG_DBG("pthread_kthread_exit: %p (ext %d)", /* NOSONAR */
					t->z_thread, (int)t->attr.is_external);
				t->attr = (struct posix_thread_attr){0};
				t->z_thread = NULL;
				posix_thread_q_set(t, POSIX_THREAD_READY_Q);
				break;
			}
		}
	}
	if (!res) {
		LOG_DBG("pthread_kthread_exit: Not found %p", kthread); /* NOSONAR */
	}
}

static size_t posix_thread_recycle(void)
{
	struct posix_thread *t;
	struct posix_thread *safe_t;
	size_t count, count_ext = 0;
	sys_dlist_t recyclables = SYS_DLIST_STATIC_INIT(&recyclables);

	K_SPINLOCK(&pthread_pool_lock) {
		SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&posix_thread_q[POSIX_THREAD_DONE_Q], t, safe_t,
						  q_node) {
			if (t->attr.detachstate == PTHREAD_CREATE_JOINABLE) {
				/* thread has not been joined yet */
				continue;
			}
			if (t->attr.is_external) {
				++count_ext;
			}
			sys_dlist_remove(&t->q_node);
			sys_dlist_append(&recyclables, &t->q_node);
		}
	}

	if (sys_dlist_is_empty(&recyclables)) {
		return 0;
	}

	count = sys_dlist_len(&recyclables);
	LOG_DBG("Recycling %zu threads (%zu ext)", count, count_ext); /* NOSONAR */

	SYS_DLIST_FOR_EACH_CONTAINER(&recyclables, t, q_node) {
		if (t->attr.caller_destroys) {
			t->attr = (struct posix_thread_attr){0};
		} else {
			(void)pthread_attr_destroy((pthread_attr_t *)&t->attr);
		}
	}

	K_SPINLOCK(&pthread_pool_lock) {
		while (!sys_dlist_is_empty(&recyclables)) {
			t = CONTAINER_OF(sys_dlist_get(&recyclables), struct posix_thread, q_node);
			posix_thread_q_set(t, POSIX_THREAD_READY_Q);
		}
	}
	return count;
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
			posix_thread_init_thread_state(t, false);
			t->z_thread = &t->thread;
		}
	}

	if (t != NULL && IS_ENABLED(CONFIG_PTHREAD_CREATE_BARRIER)) {
		err = pthread_barrier_init(&barrier, NULL, 2);
		if (err != 0) {
			/* cannot allocate barrier. move thread back to ready_q */
			posix_thread_move_to_readyq(t, true);
			t = NULL;
		}
	}

	if (t == NULL) {
		/* no threads are ready */
		LOG_DBG("No threads are ready");
		return EAGAIN;
	}

	if (_attr == NULL) {
		err = posix_thread_create_attr(t, false);
		if (err != 0) {
			return err;
		}
	} else {
		/* copy user-provided attr into thread, caller must destroy attr at a later time */
		t->attr = *(struct posix_thread_attr *)_attr;
	}

	if (t->attr.inheritsched == PTHREAD_INHERIT_SCHED) {
		posix_thread_inherit_sched(k_thread_priority_get(k_current_get()), t);
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
	*th = to_pthread_id(t);

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
	int ret = EINVAL;
	bool cancel_pending = false;
	struct posix_thread *t = NULL;
	bool cancel_type = -1;

	if (state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE) {
		LOG_DBG("Invalid pthread state %d", state);
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(posix_thread_self(false));
		if (t == NULL || t->attr.is_external) {
			ret = EINVAL;
			K_SPINLOCK_BREAK;
		}

		if (oldstate != NULL) {
			*oldstate = t->attr.cancelstate;
		}

		t->attr.cancelstate = state;
		cancel_pending = t->attr.cancelpending;
		cancel_type = t->attr.canceltype;

		ret = 0;
	}

	if (ret == 0 && state == PTHREAD_CANCEL_ENABLE &&
	    cancel_type == PTHREAD_CANCEL_ASYNCHRONOUS && cancel_pending) {
		/* TODO: execution of cleanup-handler on target-thread for async-cancel */
		posix_thread_finalize(t, false, PTHREAD_CANCELED);
	}

	return ret;
}

/**
 * @brief Set cancelability Type.
 *
 * See IEEE 1003.1
 */
int pthread_setcanceltype(int type, int *oldtype)
{
	int ret = EINVAL;
	struct posix_thread *t;

	if (type != PTHREAD_CANCEL_DEFERRED && type != PTHREAD_CANCEL_ASYNCHRONOUS) {
		LOG_DBG("Invalid pthread cancel type %d", type);
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(posix_thread_self(false));
		if (t == NULL || t->attr.is_external) {
			ret = EINVAL;
			K_SPINLOCK_BREAK;
		}

		if (oldtype != NULL) {
			*oldtype = t->attr.canceltype;
		}
		t->attr.canceltype = type;

		ret = 0;
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
	bool cancel_pended = false;
	struct posix_thread *t = NULL;

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(posix_thread_self(false));
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
		posix_thread_finalize(t, true, PTHREAD_CANCELED);
	}
}

/**
 * @brief Cancel execution of a thread.
 *
 * See IEEE 1003.1
 */
int pthread_cancel(pthread_t pthread)
{
	int ret = ESRCH;
	bool cancel_state = PTHREAD_CANCEL_ENABLE;
	bool cancel_type = PTHREAD_CANCEL_DEFERRED;
	struct posix_thread *t = NULL;

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL || t->attr.is_external) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		if (!__attr_is_initialized(&t->attr)) {
			/* thread has already terminated */
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		ret = 0;
		t->attr.cancelpending = true;
		cancel_state = t->attr.cancelstate;
		cancel_type = t->attr.canceltype;
	}

	if (ret == 0 && cancel_state == PTHREAD_CANCEL_ENABLE &&
	    cancel_type == PTHREAD_CANCEL_ASYNCHRONOUS) {
		/* TODO: execution of cleanup-handler on target-thread for async-cancel */
		posix_thread_finalize(t, false, PTHREAD_CANCELED);
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
	int ret = ESRCH;
	int new_prio = K_LOWEST_APPLICATION_THREAD_PRIO;
	struct posix_thread *t = NULL;

	if (param == NULL || !valid_posix_policy(policy) ||
	    !is_posix_policy_prio_valid(param->sched_priority, policy)) {
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL || !t->z_thread) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		ret = 0;
		new_prio = posix_to_zephyr_priority(param->sched_priority, policy);
	}

	if (ret == 0) {
		k_thread_priority_set(t->z_thread, new_prio);
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

	ret = ESRCH;
	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(thread);
		if (t == NULL || !t->z_thread) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		ret = 0;
		new_prio = posix_to_zephyr_priority(prio, policy);
	}

	if (ret == 0) {
		k_thread_priority_set(t->z_thread, new_prio);
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
	return posix_thread_attr_init((struct posix_thread_attr *)_attr, false);
}

/**
 * @brief Get thread scheduling policy and parameters
 *
 * See IEEE 1003.1
 */
int pthread_getschedparam(pthread_t pthread, int *policy, struct sched_param *param)
{
	int ret = ESRCH;
	struct posix_thread *t;

	if (policy == NULL || param == NULL) {
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL || !t->z_thread) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		if (!__attr_is_initialized(&t->attr)) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		ret = 0;
		param->sched_priority =
			zephyr_to_posix_priority(k_thread_priority_get(t->z_thread), policy);
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
	int ret = EINVAL;
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
		ret = 0;
	}

	if (ret == 0 && run_init_func) {
		init_func();
	}

	return ret;
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
		self = to_posix_thread(posix_thread_self(false));
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

	posix_thread_finalize(self, true, retval);
	CODE_UNREACHABLE;
}

static int pthread_timedjoin_internal(pthread_t pthread, void **status, k_timeout_t timeout)
{
	int ret = ESRCH;
	struct posix_thread *t = NULL;
	k_tid_t z_thread = NULL;

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
		if (t->attr.is_external) {
			ret = EINVAL;
			K_SPINLOCK_BREAK;
		}
		z_thread = t->z_thread;
		if (z_thread) {
			LOG_DBG("Pthread %p joining..", z_thread); /* NOSONAR */
		} else {
			LOG_DBG("Pthread skip joining, already terminated.."); /* NOSONAR */
		}

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
		ret = 0;
		t->attr.detachstate = PTHREAD_CREATE_DETACHED;
	}

	switch (ret) {
	case ESRCH:
		LOG_DBG("Pthread %p has already been joined", z_thread); /* NOSONAR */
		return ret;
	case EINVAL:
		LOG_DBG("Pthread %p is not a joinable", z_thread); /* NOSONAR */
		return ret;
	case 0:
		break;
	}

	if (z_thread) {
		ret = k_thread_join(z_thread, timeout);
	} /* else: already terminated */
	if (ret != 0) {
		/* when joining failed, ensure that the thread can be joined later */
		K_SPINLOCK(&pthread_pool_lock) {
			t->attr.detachstate = PTHREAD_CREATE_JOINABLE;
		}
	}
	if (ret == -EBUSY) {
		return EBUSY;
	} else if (ret == -EAGAIN) {
		return ETIMEDOUT;
	}
	/* Can only be ok or -EDEADLK, which should never occur for pthreads */
	__ASSERT_NO_MSG(ret == 0);

	if (z_thread) {
		LOG_DBG("Joined pthread %p", z_thread); /* NOSONAR */
	}

	if (status != NULL) {
		LOG_DBG("Writing status to %p", status);
		*status = t->retval;
	}

	posix_thread_recycle();

	return 0;
}

/**
 * @brief Await a thread termination with timeout.
 *
 * Non-portable GNU extension of IEEE 1003.1
 */
int pthread_timedjoin_np(pthread_t pthread, void **status, const struct timespec *abstime)
{
	if ((abstime == NULL) || !timespec_is_valid(abstime)) {
		LOG_DBG("%s is invalid", "abstime");
		return EINVAL;
	}

	return pthread_timedjoin_internal(pthread, status,
					  K_MSEC(timespec_to_timeoutms(CLOCK_REALTIME, abstime)));
}

/**
 * @brief Check a thread for termination.
 *
 * Non-portable GNU extension of IEEE 1003.1
 */
int pthread_tryjoin_np(pthread_t pthread, void **status)
{
	return pthread_timedjoin_internal(pthread, status, K_NO_WAIT);
}

/**
 * @brief Await a thread termination.
 *
 * See IEEE 1003.1
 */
int pthread_join(pthread_t pthread, void **status)
{
	return pthread_timedjoin_internal(pthread, status, K_FOREVER);
}

/**
 * @brief Detach a thread.
 *
 * See IEEE 1003.1
 */
int pthread_detach(pthread_t pthread)
{
	int ret = ESRCH;
	struct posix_thread *t = NULL;

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(pthread);
		if (t == NULL || !t->z_thread) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}
		if (t->attr.is_external) {
			ret = EINVAL;
			K_SPINLOCK_BREAK;
		}
		if (posix_thread_q_get(t) == POSIX_THREAD_READY_Q ||
		    t->attr.detachstate != PTHREAD_CREATE_JOINABLE) {
			LOG_DBG("Pthread %p cannot be detached", t->z_thread); /* NOSONAR */
			ret = EINVAL;
			K_SPINLOCK_BREAK;
		}

		ret = 0;
		t->attr.detachstate = PTHREAD_CREATE_DETACHED;
	}

	if (ret == 0) {
		LOG_DBG("Pthread %p detached", t->z_thread); /* NOSONAR */
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

	if (!__attr_is_initialized(attr)) {
		return EINVAL;
	} else if (attr->is_external) {
		LOG_DBG("Not supported on external thread"); /* NOSONAR */
		return EINVAL;
	} else if ((stacksize == 0) || (PTHREAD_STACK_MIN > 0 && stacksize < PTHREAD_STACK_MIN) ||
		   (stacksize > PTHREAD_STACK_MAX)) {
		LOG_DBG("Invalid stacksize %zu", stacksize); /* NOSONAR */
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

	if (!attr->is_external) { /* external thread owns the stack */
		ret = k_thread_stack_free(attr->stack);
		if (ret == 0) {
			LOG_DBG("Freed attr %p thread stack %zu@%p", _attr, /* NOSONAR */
				__get_attr_stacksize(attr), attr->stack);
		}
	}

	*attr = (struct posix_thread_attr){0};
	LOG_DBG("Destroyed attr %p", _attr);

	return 0;
}

int pthread_setname_np(pthread_t thread, const char *name)
{
	k_tid_t kthread = pthread_to_zephyr_thread_any(thread);

	if (!kthread) {
		return ESRCH;
	}
	if (name == NULL) {
		return EINVAL;
	}
#ifdef CONFIG_THREAD_NAME
	return k_thread_name_set(kthread, name);
#else
	return 0;
#endif
}

int pthread_getname_np(pthread_t thread, char *name, size_t len)
{
	k_tid_t kthread = pthread_to_zephyr_thread_any(thread);

	if (!kthread) {
		return ESRCH;
	}
	if (name == NULL) {
		return EINVAL;
	}
#ifdef CONFIG_THREAD_NAME
	memset(name, '\0', len);
	return k_thread_name_copy(kthread, name, len - 1);
#else
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
	int ret = ESRCH;
	struct posix_thread *t = NULL;

	if (!(how == SIG_BLOCK || how == SIG_SETMASK || how == SIG_UNBLOCK)) {
		return EINVAL;
	}

	K_SPINLOCK(&pthread_pool_lock) {
		t = to_posix_thread(posix_thread_self(false));
		if (t == NULL) {
			ret = ESRCH;
			K_SPINLOCK_BREAK;
		}

		if (oset != NULL) {
			*oset = t->sigset;
		}

		ret = 0;
		if (set == NULL) {
			K_SPINLOCK_BREAK;
		}

		const unsigned long *const x = (const unsigned long *)set;
		unsigned long *const y = (unsigned long *)&t->sigset;

		switch (how) {
		case SIG_BLOCK:
			for (size_t i = 0; i < sizeof(sigset_t) / sizeof(unsigned long); ++i) {
				y[i] |= x[i];
			}
			break;
		case SIG_SETMASK:
			t->sigset = *set;
			break;
		case SIG_UNBLOCK:
			for (size_t i = 0; i < sizeof(sigset_t) / sizeof(unsigned long); ++i) {
				y[i] &= ~x[i];
			}
			break;
		}
	}

	return ret;
}

extern struct k_thread z_main_thread;

__boot_func
static int posix_thread_pool_init(void)
{
	ARRAY_FOR_EACH_PTR(posix_thread_pool, th) {
		posix_thread_q_set(th, POSIX_THREAD_READY_Q);
	}
	{
		/* z_main_thread is not yet initialized, we rely only on its address */
		struct posix_thread *pmain = posix_thread_add_external(&z_main_thread);

		if (!pmain) {
			LOG_ERR("pthread_init: Failed to add external main-thread"); /* NOSONAR */
			k_fatal_halt(K_ERR_KERNEL_OOPS);
		}
		__ASSERT_NO_MSG(pmain == &posix_thread_pool[0]);
		__ASSERT_NO_MSG(0 == posix_thread_to_offset(pmain));

		/* z_main_thread not yet initialized, skip k_thread_priority_get(&z_main_thread) */
		posix_thread_inherit_sched(CONFIG_MAIN_THREAD_PRIORITY, pmain);
	}

	return 0;
}
SYS_INIT(posix_thread_pool_init, PRE_KERNEL_1, 0);

int sched_yield(void)
{
	k_yield();
	return 0;
}
