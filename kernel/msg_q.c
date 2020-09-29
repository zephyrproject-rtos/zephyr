/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Message queues.
 */


#include <kernel.h>
#include <kernel_structs.h>
#include <debug/object_tracing_common.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <string.h>
#include <ksched.h>
#include <wait_q.h>
#include <sys/dlist.h>
#include <sys/math_extras.h>
#include <init.h>
#include <syscall_handler.h>
#include <kernel_internal.h>
#include <sys/check.h>

#ifdef CONFIG_OBJECT_TRACING

struct k_msgq *_trace_list_k_msgq;

/*
 * Complete initialization of statically defined message queues.
 */
static int init_msgq_module(const struct device *dev)
{
	ARG_UNUSED(dev);

	Z_STRUCT_SECTION_FOREACH(k_msgq, msgq) {
		SYS_TRACING_OBJ_INIT(k_msgq, msgq);
	}
	return 0;
}

SYS_INIT(init_msgq_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

void k_msgq_init(struct k_msgq *msgq, char *buffer, size_t msg_size,
		 uint32_t max_msgs)
{
	msgq->msg_size = msg_size;
	msgq->max_msgs = max_msgs;
	msgq->buffer_start = buffer;
	msgq->buffer_end = buffer + (max_msgs * msg_size);
	msgq->read_ptr = buffer;
	msgq->write_ptr = buffer;
	msgq->used_msgs = 0;
	msgq->flags = 0;
	z_waitq_init(&msgq->wait_q);
	msgq->lock = (struct k_spinlock) {};

	SYS_TRACING_OBJ_INIT(k_msgq, msgq);

	z_object_init(msgq);
}

int z_impl_k_msgq_alloc_init(struct k_msgq *msgq, size_t msg_size,
			    uint32_t max_msgs)
{
	void *buffer;
	int ret;
	size_t total_size;

	if (size_mul_overflow(msg_size, max_msgs, &total_size)) {
		ret = -EINVAL;
	} else {
		buffer = z_thread_malloc(total_size);
		if (buffer != NULL) {
			k_msgq_init(msgq, buffer, msg_size, max_msgs);
			msgq->flags = K_MSGQ_FLAG_ALLOC;
			ret = 0;
		} else {
			ret = -ENOMEM;
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_msgq_alloc_init(struct k_msgq *q, size_t msg_size,
			    uint32_t max_msgs)
{
	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(q, K_OBJ_MSGQ));

	return z_impl_k_msgq_alloc_init(q, msg_size, max_msgs);
}
#include <syscalls/k_msgq_alloc_init_mrsh.c>
#endif

int k_msgq_cleanup(struct k_msgq *msgq)
{
	CHECKIF(z_waitq_head(&msgq->wait_q) != NULL) {
		return -EBUSY;
	}

	if ((msgq->flags & K_MSGQ_FLAG_ALLOC) != 0) {
		k_free(msgq->buffer_start);
		msgq->flags &= ~K_MSGQ_FLAG_ALLOC;
	}
	return 0;
}


int z_impl_k_msgq_put(struct k_msgq *msgq, const void *data, k_timeout_t timeout)
{
	__ASSERT(!arch_is_in_isr() || K_TIMEOUT_EQ(timeout, K_NO_WAIT), "");

	struct k_thread *pending_thread;
	k_spinlock_key_t key;
	int result;

	key = k_spin_lock(&msgq->lock);

	if (msgq->used_msgs < msgq->max_msgs) {
		/* message queue isn't full */
		pending_thread = z_unpend_first_thread(&msgq->wait_q);
		if (pending_thread != NULL) {
			/* give message to waiting thread */
			(void)memcpy(pending_thread->base.swap_data, data,
			       msgq->msg_size);
			/* wake up waiting thread */
			arch_thread_return_value_set(pending_thread, 0);
			z_ready_thread(pending_thread);
			z_reschedule(&msgq->lock, key);
			return 0;
		} else {
			/* put message in queue */
			(void)memcpy(msgq->write_ptr, data, msgq->msg_size);
			msgq->write_ptr += msgq->msg_size;
			if (msgq->write_ptr == msgq->buffer_end) {
				msgq->write_ptr = msgq->buffer_start;
			}
			msgq->used_msgs++;
		}
		result = 0;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		/* don't wait for message space to become available */
		result = -ENOMSG;
	} else {
		/* wait for put message success, failure, or timeout */
		_current->base.swap_data = (void *) data;
		return z_pend_curr(&msgq->lock, key, &msgq->wait_q, timeout);
	}

	k_spin_unlock(&msgq->lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_msgq_put(struct k_msgq *q, const void *data,
				    k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(q, K_OBJ_MSGQ));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, q->msg_size));

	return z_impl_k_msgq_put(q, data, timeout);
}
#include <syscalls/k_msgq_put_mrsh.c>
#endif

void z_impl_k_msgq_get_attrs(struct k_msgq *msgq, struct k_msgq_attrs *attrs)
{
	attrs->msg_size = msgq->msg_size;
	attrs->max_msgs = msgq->max_msgs;
	attrs->used_msgs = msgq->used_msgs;
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_msgq_get_attrs(struct k_msgq *q,
					   struct k_msgq_attrs *attrs)
{
	Z_OOPS(Z_SYSCALL_OBJ(q, K_OBJ_MSGQ));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(attrs, sizeof(struct k_msgq_attrs)));
	z_impl_k_msgq_get_attrs(q, attrs);
}
#include <syscalls/k_msgq_get_attrs_mrsh.c>
#endif

int z_impl_k_msgq_get(struct k_msgq *msgq, void *data, k_timeout_t timeout)
{
	__ASSERT(!arch_is_in_isr() || K_TIMEOUT_EQ(timeout, K_NO_WAIT), "");

	k_spinlock_key_t key;
	struct k_thread *pending_thread;
	int result;

	key = k_spin_lock(&msgq->lock);

	if (msgq->used_msgs > 0) {
		/* take first available message from queue */
		(void)memcpy(data, msgq->read_ptr, msgq->msg_size);
		msgq->read_ptr += msgq->msg_size;
		if (msgq->read_ptr == msgq->buffer_end) {
			msgq->read_ptr = msgq->buffer_start;
		}
		msgq->used_msgs--;

		/* handle first thread waiting to write (if any) */
		pending_thread = z_unpend_first_thread(&msgq->wait_q);
		if (pending_thread != NULL) {
			/* add thread's message to queue */
			(void)memcpy(msgq->write_ptr, pending_thread->base.swap_data,
			       msgq->msg_size);
			msgq->write_ptr += msgq->msg_size;
			if (msgq->write_ptr == msgq->buffer_end) {
				msgq->write_ptr = msgq->buffer_start;
			}
			msgq->used_msgs++;

			/* wake up waiting thread */
			arch_thread_return_value_set(pending_thread, 0);
			z_ready_thread(pending_thread);
			z_reschedule(&msgq->lock, key);
			return 0;
		}
		result = 0;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		/* don't wait for a message to become available */
		result = -ENOMSG;
	} else {
		/* wait for get message success or timeout */
		_current->base.swap_data = data;
		return z_pend_curr(&msgq->lock, key, &msgq->wait_q, timeout);
	}

	k_spin_unlock(&msgq->lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_msgq_get(struct k_msgq *q, void *data,
				    k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(q, K_OBJ_MSGQ));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, q->msg_size));

	return z_impl_k_msgq_get(q, data, timeout);
}
#include <syscalls/k_msgq_get_mrsh.c>
#endif

int z_impl_k_msgq_peek(struct k_msgq *msgq, void *data)
{
	k_spinlock_key_t key;
	int result;

	key = k_spin_lock(&msgq->lock);

	if (msgq->used_msgs > 0) {
		/* take first available message from queue */
		(void)memcpy(data, msgq->read_ptr, msgq->msg_size);
		result = 0;
	} else {
		/* don't wait for a message to become available */
		result = -ENOMSG;
	}

	k_spin_unlock(&msgq->lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_msgq_peek(struct k_msgq *q, void *data)
{
	Z_OOPS(Z_SYSCALL_OBJ(q, K_OBJ_MSGQ));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, q->msg_size));

	return z_impl_k_msgq_peek(q, data);
}
#include <syscalls/k_msgq_peek_mrsh.c>
#endif

void z_impl_k_msgq_purge(struct k_msgq *msgq)
{
	k_spinlock_key_t key;
	struct k_thread *pending_thread;

	key = k_spin_lock(&msgq->lock);

	/* wake up any threads that are waiting to write */
	while ((pending_thread = z_unpend_first_thread(&msgq->wait_q)) != NULL) {
		arch_thread_return_value_set(pending_thread, -ENOMSG);
		z_ready_thread(pending_thread);
	}

	msgq->used_msgs = 0;
	msgq->read_ptr = msgq->write_ptr;

	z_reschedule(&msgq->lock, key);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_msgq_purge(struct k_msgq *q)
{
	Z_OOPS(Z_SYSCALL_OBJ(q, K_OBJ_MSGQ));
	z_impl_k_msgq_purge(q);
}
#include <syscalls/k_msgq_purge_mrsh.c>

static inline uint32_t z_vrfy_k_msgq_num_free_get(struct k_msgq *q)
{
	Z_OOPS(Z_SYSCALL_OBJ(q, K_OBJ_MSGQ));
	return z_impl_k_msgq_num_free_get(q);
}
#include <syscalls/k_msgq_num_free_get_mrsh.c>

static inline uint32_t z_vrfy_k_msgq_num_used_get(struct k_msgq *q)
{
	Z_OOPS(Z_SYSCALL_OBJ(q, K_OBJ_MSGQ));
	return z_impl_k_msgq_num_used_get(q);
}
#include <syscalls/k_msgq_num_used_get_mrsh.c>

#endif
