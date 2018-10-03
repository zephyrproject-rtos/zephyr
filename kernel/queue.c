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
#include <linker/sections.h>
#include <wait_q.h>
#include <ksched.h>
#include <misc/sflist.h>
#include <init.h>
#include <syscall_handler.h>

extern struct k_queue _k_queue_list_start[];
extern struct k_queue _k_queue_list_end[];

struct alloc_node {
	sys_sfnode_t node;
	void *data;
};

void *z_queue_node_peek(sys_sfnode_t *node, bool needs_free)
{
	void *ret;

	if (node && sys_sfnode_flags_get(node)) {
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
		/* Data was directly placed in the queue, the first 4 bytes
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
static int init_queue_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_queue *queue;

	for (queue = _k_queue_list_start; queue < _k_queue_list_end; queue++) {
		SYS_TRACING_OBJ_INIT(k_queue, queue);
	}
	return 0;
}

SYS_INIT(init_queue_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

void _impl_k_queue_init(struct k_queue *queue)
{
	sys_sflist_init(&queue->data_q);
	_waitq_init(&queue->wait_q);
#if defined(CONFIG_POLL)
	sys_dlist_init(&queue->poll_events);
#endif

	SYS_TRACING_OBJ_INIT(k_queue, queue);
	_k_object_init(queue);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_queue_init, queue_ptr)
{
	struct k_queue *queue = (struct k_queue *)queue_ptr;

	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(queue, K_OBJ_QUEUE));
	_impl_k_queue_init(queue);
	return 0;
}
#endif

#if !defined(CONFIG_POLL)
static void prepare_thread_to_run(struct k_thread *thread, void *data)
{
	_ready_thread(thread);
	_set_thread_return_value_with_data(thread, 0, data);
}
#endif /* CONFIG_POLL */

#ifdef CONFIG_POLL
static inline void handle_poll_events(struct k_queue *queue, u32_t state)
{
	_handle_obj_poll_events(&queue->poll_events, state);
}
#endif

void _impl_k_queue_cancel_wait(struct k_queue *queue)
{
	unsigned int key = irq_lock();
#if !defined(CONFIG_POLL)
	struct k_thread *first_pending_thread;

	first_pending_thread = _unpend_first_thread(&queue->wait_q);

	if (first_pending_thread != NULL) {
		prepare_thread_to_run(first_pending_thread, NULL);
	}
#else
	handle_poll_events(queue, K_POLL_STATE_CANCELLED);
#endif /* !CONFIG_POLL */

	_reschedule(key);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER1_SIMPLE_VOID(k_queue_cancel_wait, K_OBJ_QUEUE,
			       struct k_queue *);
#endif

static int queue_insert(struct k_queue *queue, void *prev, void *data,
			bool alloc)
{
	unsigned int key = irq_lock();
#if !defined(CONFIG_POLL)
	struct k_thread *first_pending_thread;

	first_pending_thread = _unpend_first_thread(&queue->wait_q);

	if (first_pending_thread != NULL) {
		prepare_thread_to_run(first_pending_thread, data);
		_reschedule(key);
		return 0;
	}
#endif /* !CONFIG_POLL */

	/* Only need to actually allocate if no threads are pending */
	if (alloc) {
		struct alloc_node *anode;

		anode = z_thread_malloc(sizeof(*anode));
		if (anode == NULL) {
			return -ENOMEM;
		}
		anode->data = data;
		sys_sfnode_init(&anode->node, 0x1);
		data = anode;
	} else {
		sys_sfnode_init(data, 0x0);
	}
	sys_sflist_insert(&queue->data_q, prev, data);

#if defined(CONFIG_POLL)
	handle_poll_events(queue, K_POLL_STATE_DATA_AVAILABLE);
#endif /* CONFIG_POLL */

	_reschedule(key);
	return 0;
}

void k_queue_insert(struct k_queue *queue, void *prev, void *data)
{
	(void)queue_insert(queue, prev, data, false);
}

void k_queue_append(struct k_queue *queue, void *data)
{
	(void)queue_insert(queue, sys_sflist_peek_tail(&queue->data_q),
			   data, false);
}

void k_queue_prepend(struct k_queue *queue, void *data)
{
	(void)queue_insert(queue, NULL, data, false);
}

int _impl_k_queue_alloc_append(struct k_queue *queue, void *data)
{
	return queue_insert(queue, sys_sflist_peek_tail(&queue->data_q), data,
			    true);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_queue_alloc_append, queue, data)
{
	Z_OOPS(Z_SYSCALL_OBJ(queue, K_OBJ_QUEUE));

	return _impl_k_queue_alloc_append((struct k_queue *)queue,
					  (void *)data);
}
#endif

int _impl_k_queue_alloc_prepend(struct k_queue *queue, void *data)
{
	return queue_insert(queue, NULL, data, true);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_queue_alloc_prepend, queue, data)
{
	Z_OOPS(Z_SYSCALL_OBJ(queue, K_OBJ_QUEUE));

	return _impl_k_queue_alloc_prepend((struct k_queue *)queue,
					   (void *)data);
}
#endif

void k_queue_append_list(struct k_queue *queue, void *head, void *tail)
{
	__ASSERT(head && tail, "invalid head or tail");

	unsigned int key = irq_lock();
#if !defined(CONFIG_POLL)
	struct k_thread *thread;

	while ((head != NULL) &&
		(thread = _unpend_first_thread(&queue->wait_q))) {
		prepare_thread_to_run(thread, head);
		head = *(void **)head;
	}

	if (head != NULL) {
		sys_sflist_append_list(&queue->data_q, head, tail);
	}

#else
	sys_sflist_append_list(&queue->data_q, head, tail);
	handle_poll_events(queue, K_POLL_STATE_DATA_AVAILABLE);
#endif /* !CONFIG_POLL */

	_reschedule(key);
}

void k_queue_merge_slist(struct k_queue *queue, sys_slist_t *list)
{
	__ASSERT(!sys_slist_is_empty(list), "list must not be empty");

	/*
	 * note: this works as long as:
	 * - the slist implementation keeps the next pointer as the first
	 *   field of the node object type
	 * - list->tail->next = NULL.
	 * - sflist implementation only differs from slist by stuffing
	 *   flag bytes in the lower order bits of the data pointer
	 * - source list is really an slist and not an sflist with flags set
	 */
	k_queue_append_list(queue, list->head, list->tail);
	sys_slist_init(list);
}

#if defined(CONFIG_POLL)
static void *k_queue_poll(struct k_queue *queue, s32_t timeout)
{
	struct k_poll_event event;
	int err, elapsed = 0, done = 0;
	unsigned int key;
	void *val;
	u32_t start;

	k_poll_event_init(&event, K_POLL_TYPE_FIFO_DATA_AVAILABLE,
			  K_POLL_MODE_NOTIFY_ONLY, queue);

	if (timeout != K_FOREVER) {
		start = k_uptime_get_32();
	}

	do {
		event.state = K_POLL_STATE_NOT_READY;

		err = k_poll(&event, 1, timeout - elapsed);

		if (err && err != -EAGAIN) {
			return NULL;
		}

		/* sys_sflist_* aren't threadsafe, so must be always protected
		 * by irq_lock.
		 */
		key = irq_lock();
		val = z_queue_node_peek(sys_sflist_get(&queue->data_q), true);
		irq_unlock(key);

		if ((val == NULL) && (timeout != K_FOREVER)) {
			elapsed = k_uptime_get_32() - start;
			done = elapsed > timeout;
		}
	} while (!val && !done);

	return val;
}
#endif /* CONFIG_POLL */

void *_impl_k_queue_get(struct k_queue *queue, s32_t timeout)
{
	unsigned int key;
	void *data;

	key = irq_lock();

	if (likely(!sys_sflist_is_empty(&queue->data_q))) {
		sys_sfnode_t *node;

		node = sys_sflist_get_not_empty(&queue->data_q);
		data = z_queue_node_peek(node, true);
		irq_unlock(key);
		return data;
	}

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		return NULL;
	}

#if defined(CONFIG_POLL)
	irq_unlock(key);

	return k_queue_poll(queue, timeout);

#else
	int ret = _pend_current_thread(key, &queue->wait_q, timeout);

	return ret ? NULL : _current->base.swap_data;
#endif /* CONFIG_POLL */
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_queue_get, queue, timeout_p)
{
	s32_t timeout = timeout_p;

	Z_OOPS(Z_SYSCALL_OBJ(queue, K_OBJ_QUEUE));

	return (u32_t)_impl_k_queue_get((struct k_queue *)queue, timeout);
}

Z_SYSCALL_HANDLER1_SIMPLE(k_queue_is_empty, K_OBJ_QUEUE, struct k_queue *);
Z_SYSCALL_HANDLER1_SIMPLE(k_queue_peek_head, K_OBJ_QUEUE, struct k_queue *);
Z_SYSCALL_HANDLER1_SIMPLE(k_queue_peek_tail, K_OBJ_QUEUE, struct k_queue *);
#endif /* CONFIG_USERSPACE */
