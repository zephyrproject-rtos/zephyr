/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_P4WQ_H_
#define ZEPHYR_INCLUDE_SYS_P4WQ_H_

#include <zephyr/kernel.h>

/* Zephyr Pooled Parallel Preemptible Priority-based Work Queues */

struct k_p4wq_work;

/**
 * P4 Queue handler callback
 */
typedef void (*k_p4wq_handler_t)(struct k_p4wq_work *work);

/**
 * @brief P4 Queue Work Item
 *
 * User-populated struct representing a single work item.  The
 * priority and deadline fields are interpreted as thread scheduling
 * priorities, exactly as per k_thread_priority_set() and
 * k_thread_deadline_set().
 */
struct k_p4wq_work {
	/* Filled out by submitting code */
	uint32_t group_id;
	int32_t priority;
	int32_t deadline;
	k_p4wq_handler_t handler;
	bool sync;
	struct k_sem done_sem;
	void *priv_data;

	/* reserved for implementation */
	union {
		struct rbnode rbnode;
		sys_dlist_t dlnode;
	};
	struct k_thread *thread;
	struct k_p4wq *queue;
};

#define K_P4WQ_QUEUE_PER_THREAD		BIT(0)
#define K_P4WQ_DELAYED_START		BIT(1)
#define K_P4WQ_USER_CPU_MASK		BIT(2)

/**
 * @brief P4 Queue
 *
 * Kernel pooled parallel preemptible priority-based work queue
 */
struct k_p4wq {
	struct k_spinlock lock;

	/* Pending threads waiting for work items
	 *
	 * FIXME: a waitq isn't really the right data structure here.
	 * Wait queues are priority-sorted, but we don't want that
	 * sorting overhead since we're effectively doing it ourselves
	 * by directly mutating the priority when a thread is
	 * unpended.  We just want "blocked threads on a list", but
	 * there's no clean scheduler API for that.
	 */
	_wait_q_t waitq;

	/* Work items waiting for processing */
	struct rbtree queue;

	/* Work items in progress */
	sys_dlist_t active;

	/* K_P4WQ_* flags above */
	uint32_t flags;
};

struct k_p4wq_initparam {
	uint32_t num;
	uintptr_t stack_size;
	struct k_p4wq *queue;
	struct k_thread *threads;
	struct z_thread_stack_element *stacks;
	uint32_t flags;
};

/**
 * @brief Statically initialize a P4 Work Queue
 *
 * Statically defines a struct k_p4wq object with the specified number
 * of threads which will be initialized at boot and ready for use on
 * entry to main().
 *
 * @param name Symbol name of the struct k_p4wq that will be defined
 * @param n_threads Number of threads in the work queue pool
 * @param stack_sz Requested stack size of each thread, in bytes
 */
#define K_P4WQ_DEFINE(name, n_threads, stack_sz)			\
	static K_THREAD_STACK_ARRAY_DEFINE(_p4stacks_##name,		\
					   n_threads, stack_sz);	\
	static struct k_thread _p4threads_##name[n_threads];		\
	static struct k_p4wq name;					\
	static const STRUCT_SECTION_ITERABLE(k_p4wq_initparam,		\
					     _init_##name) = {		\
		.num = n_threads,					\
		.stack_size = stack_sz,					\
		.threads = _p4threads_##name,				\
		.stacks = &(_p4stacks_##name[0][0]),			\
		.queue = &name,						\
		.flags = 0,						\
	}

/**
 * @brief Statically initialize an array of P4 Work Queues
 *
 * Statically defines an array of struct k_p4wq objects with the specified
 * number of threads which will be initialized at boot and ready for use on
 * entry to main().
 *
 * @param name Symbol name of the struct k_p4wq array that will be defined
 * @param n_threads Number of threads and work queues
 * @param stack_sz Requested stack size of each thread, in bytes
 * @param flg Flags
 */
#define K_P4WQ_ARRAY_DEFINE(name, n_threads, stack_sz, flg)		\
	static K_THREAD_STACK_ARRAY_DEFINE(_p4stacks_##name,		\
					   n_threads, stack_sz);	\
	static struct k_thread _p4threads_##name[n_threads];		\
	static struct k_p4wq name[n_threads];				\
	static const STRUCT_SECTION_ITERABLE(k_p4wq_initparam,		\
					     _init_##name) = {		\
		.num = n_threads,					\
		.stack_size = stack_sz,					\
		.threads = _p4threads_##name,				\
		.stacks = &(_p4stacks_##name[0][0]),			\
		.queue = name,						\
		.flags = K_P4WQ_QUEUE_PER_THREAD | flg,			\
	}

/**
 * @brief Initialize P4 Queue
 *
 * Initializes a P4 Queue object.  These objects must be initialized
 * via this function (or statically using K_P4WQ_DEFINE) before any
 * other API calls are made on it.
 *
 * @param queue P4 Queue to initialize
 */
void k_p4wq_init(struct k_p4wq *queue);

/**
 * @brief Dynamically add a thread object to a P4 Queue pool
 *
 * Adds a thread to the pool managed by a P4 queue.  The thread object
 * must not be in use.  If k_thread_create() has previously been
 * called on it, it must be aborted before being given to the queue.
 *
 * @param queue P4 Queue to which to add the thread
 * @param thread Uninitialized/aborted thread object to add
 * @param stack Thread stack memory
 * @param stack_size Thread stack size
 */
void k_p4wq_add_thread(struct k_p4wq *queue, struct k_thread *thread,
		       k_thread_stack_t *stack,
		       size_t stack_size);

/**
 * @brief Submit work item to a P4 queue
 *
 * Submits the specified work item to the queue.  The caller must have
 * already initialized the relevant fields of the struct.  The queue
 * will execute the handler when CPU time is available and when no
 * higher-priority work items are available.  The handler may be
 * invoked on any CPU.
 *
 * The caller must not mutate the struct while it is stored in the
 * queue.  The memory should remain unchanged until k_p4wq_cancel() is
 * called or until the entry to the handler function.
 *
 * @note This call is a scheduling point, so if the submitted item (or
 * any other ready thread) has a higher priority than the current
 * thread and the current thread has a preemptible priority then the
 * caller will yield.
 *
 * @param queue P4 Queue to which to submit
 * @param item P4 work item to be submitted
 */
void k_p4wq_submit(struct k_p4wq *queue, struct k_p4wq_work *item);

/**
 * @brief Cancel submitted P4 work item
 *
 * Cancels a previously-submitted work item and removes it from the
 * queue.  Returns true if the item was found in the queue and
 * removed.  If the function returns false, either the item was never
 * submitted, has already been executed, or is still running.
 *
 * @return true if the item was successfully removed, otherwise false
 */
bool k_p4wq_cancel(struct k_p4wq *queue, struct k_p4wq_work *item);

/**
 * @brief Regain ownership of the work item, wait for completion if it's synchronous
 */
int k_p4wq_wait(struct k_p4wq_work *work, k_timeout_t timeout);

void k_p4wq_enable_static_thread(struct k_p4wq *queue, struct k_thread *thread,
				 uint32_t cpu_mask);

#endif /* ZEPHYR_INCLUDE_SYS_P4WQ_H_ */
