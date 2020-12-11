/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief dynamic-size QUEUE object.
 */


#include <kernel.h>
#include <kernel_structs.h>
#include <debug/object_tracing_common.h>
#include <toolchain.h>
#include <wait_q.h>
#include <ksched.h>
#include <init.h>
#include <syscall_handler.h>
#include <kernel_internal.h>
#include <sys/check.h>

struct alloc_node {
	sys_sfnode_t node;
	void *data;
};

void *z_queue_node_peek(sys_sfnode_t *node, bool needs_free)
{
	void *ret;

	if ((node != NULL) && (sys_sfnode_flags_get(node) != (uint8_t)0)) {
		/* If the flag is set, then the enqueue operation for this item
		 * did a behind-the scenes memory allocation of an alloc_node
		 * struct, which is what got put in the queue. Free it and pass
		 * back the data pointer.
		 */
		struct alloc_node *anode;

		anode = CONTAINER_OF(node, struct alloc_node, node);
		ret = anode->data;
		if (needs_free) {
			k_free(anode);
		}
	} else {
		/* Data was directly placed in the queue, the first word
		 * reserved for the linked list. User mode isn't allowed to
		 * do this, although it can get data sent this way.
		 */
		ret = (void *)node;
	}

	return ret;
}

#ifdef CONFIG_OBJECT_TRACING

struct k_queue *_trace_list_k_queue;

/*
 * Complete initialization of statically defined queues.
 */
static int init_queue_module(const struct device *dev)
{
	ARG_UNUSED(dev);

	Z_STRUCT_SECTION_FOREACH(k_queue, queue) {
		SYS_TRACING_OBJ_INIT(k_queue, queue);
	}
	return 0;
}

SYS_INIT(init_queue_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

void z_impl_k_queue_init(struct k_queue *queue)
{
	sys_sflist_init(&queue->data_q);
	queue->lock = (struct k_spinlock) {};
	z_waitq_init(&queue->wait_q);
#if defined(CONFIG_POLL)
	sys_dlist_init(&queue->poll_events);
#endif

	SYS_TRACING_OBJ_INIT(k_queue, queue);
	z_object_init(queue);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_queue_init(struct k_queue *queue)
{
	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(queue, K_OBJ_QUEUE));
	z_impl_k_queue_init(queue);
}
#include <syscalls/k_queue_init_mrsh.c>
#endif

static void prepare_thread_to_run(struct k_thread *thread, void *data)
{
	z_thread_return_value_set_with_data(thread, 0, data);
	z_ready_thread(thread);
}

static inline void handle_poll_events(struct k_queue *queue, uint32_t state)
{
#ifdef CONFIG_POLL
	z_handle_obj_poll_events(&queue->poll_events, state);
#endif
}

void z_impl_k_queue_cancel_wait(struct k_queue *queue)
{
	k_spinlock_key_t key = k_spin_lock(&queue->lock);
	struct k_thread *first_pending_thread;

	first_pending_thread = z_unpend_first_thread(&queue->wait_q);

	if (first_pending_thread != NULL) {
		prepare_thread_to_run(first_pending_thread, NULL);
	}

	handle_poll_events(queue, K_POLL_STATE_CANCELLED);
	z_reschedule(&queue->lock, key);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_queue_cancel_wait(struct k_queue *queue)
{
	Z_OOPS(Z_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	z_impl_k_queue_cancel_wait(queue);
}
#include <syscalls/k_queue_cancel_wait_mrsh.c>
#endif

static int32_t queue_insert(struct k_queue *queue, void *prev, void *data,
			    bool alloc, bool is_append)
{
	struct k_thread *first_pending_thread;
	k_spinlock_key_t key = k_spin_lock(&queue->lock);

	if (is_append) {
		prev = sys_sflist_peek_tail(&queue->data_q);
	}
	first_pending_thread = z_unpend_first_thread(&queue->wait_q);

	if (first_pending_thread != NULL) {
		prepare_thread_to_run(first_pending_thread, data);
		z_reschedule(&queue->lock, key);
		return 0;
	}

	/* Only need to actually allocate if no threads are pending */
	if (alloc) {
		struct alloc_node *anode;

		anode = z_thread_malloc(sizeof(*anode));
		if (anode == NULL) {
			k_spin_unlock(&queue->lock, key);
			return -ENOMEM;
		}
		anode->data = data;
		sys_sfnode_init(&anode->node, 0x1);
		data = anode;
	} else {
		sys_sfnode_init(data, 0x0);
	}

	sys_sflist_insert(&queue->data_q, prev, data);
	handle_poll_events(queue, K_POLL_STATE_DATA_AVAILABLE);
	z_reschedule(&queue->lock, key);
	return 0;
}

void k_queue_insert(struct k_queue *queue, void *prev, void *data)
{
	(void)queue_insert(queue, prev, data, false, false);
}

void k_queue_append(struct k_queue *queue, void *data)
{
	(void)queue_insert(queue, NULL, data, false, true);
}

void k_queue_prepend(struct k_queue *queue, void *data)
{
	(void)queue_insert(queue, NULL, data, false, false);
}

int32_t z_impl_k_queue_alloc_append(struct k_queue *queue, void *data)
{
	return queue_insert(queue, NULL, data, true, true);
}

#ifdef CONFIG_USERSPACE
static inline int32_t z_vrfy_k_queue_alloc_append(struct k_queue *queue,
						  void *data)
{
	Z_OOPS(Z_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_alloc_append(queue, data);
}
#include <syscalls/k_queue_alloc_append_mrsh.c>
#endif

int32_t z_impl_k_queue_alloc_prepend(struct k_queue *queue, void *data)
{
	return queue_insert(queue, NULL, data, true, false);

}

#ifdef CONFIG_USERSPACE
static inline int32_t z_vrfy_k_queue_alloc_prepend(struct k_queue *queue,
						   void *data)
{
	Z_OOPS(Z_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_alloc_prepend(queue, data);
}
#include <syscalls/k_queue_alloc_prepend_mrsh.c>
#endif

int k_queue_append_list(struct k_queue *queue, void *head, void *tail)
{
	/* invalid head or tail of list */
	CHECKIF(head == NULL || tail == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&queue->lock);
	struct k_thread *thread = NULL;

	if (head != NULL) {
		thread = z_unpend_first_thread(&queue->wait_q);
	}

	while ((head != NULL) && (thread != NULL)) {
		prepare_thread_to_run(thread, head);
		head = *(void **)head;
		thread = z_unpend_first_thread(&queue->wait_q);
	}

	if (head != NULL) {
		sys_sflist_append_list(&queue->data_q, head, tail);
	}

	handle_poll_events(queue, K_POLL_STATE_DATA_AVAILABLE);
	z_reschedule(&queue->lock, key);
	return 0;
}

int k_queue_merge_slist(struct k_queue *queue, sys_slist_t *list)
{
	int ret;

	/* list must not be empty */
	CHECKIF(sys_slist_is_empty(list)) {
		return -EINVAL;
	}

	/*
	 * note: this works as long as:
	 * - the slist implementation keeps the next pointer as the first
	 *   field of the node object type
	 * - list->tail->next = NULL.
	 * - sflist implementation only differs from slist by stuffing
	 *   flag bytes in the lower order bits of the data pointer
	 * - source list is really an slist and not an sflist with flags set
	 */
	ret = k_queue_append_list(queue, list->head, list->tail);
	CHECKIF(ret != 0) {
		return ret;
	}
	sys_slist_init(list);

	return 0;
}

void *z_impl_k_queue_get(struct k_queue *queue, k_timeout_t timeout)
{
	k_spinlock_key_t key = k_spin_lock(&queue->lock);
	void *data;

	if (likely(!sys_sflist_is_empty(&queue->data_q))) {
		sys_sfnode_t *node;

		node = sys_sflist_get_not_empty(&queue->data_q);
		data = z_queue_node_peek(node, true);
		k_spin_unlock(&queue->lock, key);
		return data;
	}

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_spin_unlock(&queue->lock, key);
		return NULL;
	}

	int ret = z_pend_curr(&queue->lock, key, &queue->wait_q, timeout);

	return (ret != 0) ? NULL : _current->base.swap_data;
}

#ifdef CONFIG_USERSPACE
static inline void *z_vrfy_k_queue_get(struct k_queue *queue,
				       k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_get(queue, timeout);
}
#include <syscalls/k_queue_get_mrsh.c>

static inline int z_vrfy_k_queue_is_empty(struct k_queue *queue)
{
	Z_OOPS(Z_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_is_empty(queue);
}
#include <syscalls/k_queue_is_empty_mrsh.c>

static inline void *z_vrfy_k_queue_peek_head(struct k_queue *queue)
{
	Z_OOPS(Z_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_peek_head(queue);
}
#include <syscalls/k_queue_peek_head_mrsh.c>

static inline void *z_vrfy_k_queue_peek_tail(struct k_queue *queue)
{
	Z_OOPS(Z_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_peek_tail(queue);
}
#include <syscalls/k_queue_peek_tail_mrsh.c>

#endif /* CONFIG_USERSPACE */
