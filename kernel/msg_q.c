/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Message queues.
 */


#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <string.h>
#include <ksched.h>
#include <wait_q.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/init.h>
#include <zephyr/syscall_handler.h>
#include <kernel_internal.h>
#include <zephyr/sys/check.h>

#ifdef CONFIG_OBJ_CORE_MSGQ
static struct k_obj_type obj_type_msgq;
#endif

#ifdef CONFIG_POLL
static inline void handle_poll_events(struct k_msgq *msgq, uint32_t state)
{
	z_handle_obj_poll_events(&msgq->poll_events, state);
}
#endif /* CONFIG_POLL */

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
#ifdef CONFIG_POLL
	sys_dlist_init(&msgq->poll_events);
#endif	/* CONFIG_POLL */

#ifdef CONFIG_OBJ_CORE_MSGQ
	k_obj_core_init_and_link(K_OBJ_CORE(msgq), &obj_type_msgq);
#endif

	SYS_PORT_TRACING_OBJ_INIT(k_msgq, msgq);

	z_object_init(msgq);
}

int z_impl_k_msgq_alloc_init(struct k_msgq *msgq, size_t msg_size,
			    uint32_t max_msgs)
{
	void *buffer;
	int ret;
	size_t total_size;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_msgq, alloc_init, msgq);

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

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_msgq, alloc_init, msgq, ret);

	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_msgq_alloc_init(struct k_msgq *msgq, size_t msg_size,
			    uint32_t max_msgs)
{
	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(msgq, K_OBJ_MSGQ));

	return z_impl_k_msgq_alloc_init(msgq, msg_size, max_msgs);
}
#include <syscalls/k_msgq_alloc_init_mrsh.c>
#endif

int k_msgq_cleanup(struct k_msgq *msgq)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_msgq, cleanup, msgq);

	CHECKIF(z_waitq_head(&msgq->wait_q) != NULL) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_msgq, cleanup, msgq, -EBUSY);

		return -EBUSY;
	}

	if ((msgq->flags & K_MSGQ_FLAG_ALLOC) != 0U) {
		k_free(msgq->buffer_start);
		msgq->flags &= ~K_MSGQ_FLAG_ALLOC;
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_msgq, cleanup, msgq, 0);

	return 0;
}


int z_impl_k_msgq_put(struct k_msgq *msgq, const void *data, k_timeout_t timeout)
{
	__ASSERT(!arch_is_in_isr() || K_TIMEOUT_EQ(timeout, K_NO_WAIT), "");

	struct k_thread *pending_thread;
	k_spinlock_key_t key;
	int result;

	key = k_spin_lock(&msgq->lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_msgq, put, msgq, timeout);

	if (msgq->used_msgs < msgq->max_msgs) {
		/* message queue isn't full */
		pending_thread = z_unpend_first_thread(&msgq->wait_q);
		if (pending_thread != NULL) {
			SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_msgq, put, msgq, timeout, 0);

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
			__ASSERT_NO_MSG(msgq->write_ptr >= msgq->buffer_start &&
					msgq->write_ptr < msgq->buffer_end);
			(void)memcpy(msgq->write_ptr, data, msgq->msg_size);
			msgq->write_ptr += msgq->msg_size;
			if (msgq->write_ptr == msgq->buffer_end) {
				msgq->write_ptr = msgq->buffer_start;
			}
			msgq->used_msgs++;
#ifdef CONFIG_POLL
			handle_poll_events(msgq, K_POLL_STATE_MSGQ_DATA_AVAILABLE);
#endif /* CONFIG_POLL */
		}
		result = 0;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		/* don't wait for message space to become available */
		result = -ENOMSG;
	} else {
		SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_msgq, put, msgq, timeout);

		/* wait for put message success, failure, or timeout */
		_current->base.swap_data = (void *) data;

		result = z_pend_curr(&msgq->lock, key, &msgq->wait_q, timeout);
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_msgq, put, msgq, timeout, result);
		return result;
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_msgq, put, msgq, timeout, result);

	k_spin_unlock(&msgq->lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_msgq_put(struct k_msgq *msgq, const void *data,
				    k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, msgq->msg_size));

	return z_impl_k_msgq_put(msgq, data, timeout);
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
static inline void z_vrfy_k_msgq_get_attrs(struct k_msgq *msgq,
					   struct k_msgq_attrs *attrs)
{
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(attrs, sizeof(struct k_msgq_attrs)));
	z_impl_k_msgq_get_attrs(msgq, attrs);
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

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_msgq, get, msgq, timeout);

	if (msgq->used_msgs > 0U) {
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
			SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_msgq, get, msgq, timeout);

			/* add thread's message to queue */
			__ASSERT_NO_MSG(msgq->write_ptr >= msgq->buffer_start &&
					msgq->write_ptr < msgq->buffer_end);
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

			SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_msgq, get, msgq, timeout, 0);

			return 0;
		}
		result = 0;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		/* don't wait for a message to become available */
		result = -ENOMSG;
	} else {
		SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_msgq, get, msgq, timeout);

		/* wait for get message success or timeout */
		_current->base.swap_data = data;

		result = z_pend_curr(&msgq->lock, key, &msgq->wait_q, timeout);
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_msgq, get, msgq, timeout, result);
		return result;
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_msgq, get, msgq, timeout, result);

	k_spin_unlock(&msgq->lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_msgq_get(struct k_msgq *msgq, void *data,
				    k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, msgq->msg_size));

	return z_impl_k_msgq_get(msgq, data, timeout);
}
#include <syscalls/k_msgq_get_mrsh.c>
#endif

int z_impl_k_msgq_peek(struct k_msgq *msgq, void *data)
{
	k_spinlock_key_t key;
	int result;

	key = k_spin_lock(&msgq->lock);

	if (msgq->used_msgs > 0U) {
		/* take first available message from queue */
		(void)memcpy(data, msgq->read_ptr, msgq->msg_size);
		result = 0;
	} else {
		/* don't wait for a message to become available */
		result = -ENOMSG;
	}

	SYS_PORT_TRACING_OBJ_FUNC(k_msgq, peek, msgq, result);

	k_spin_unlock(&msgq->lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_msgq_peek(struct k_msgq *msgq, void *data)
{
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, msgq->msg_size));

	return z_impl_k_msgq_peek(msgq, data);
}
#include <syscalls/k_msgq_peek_mrsh.c>
#endif

int z_impl_k_msgq_peek_at(struct k_msgq *msgq, void *data, uint32_t idx)
{
	k_spinlock_key_t key;
	int result;
	uint32_t bytes_to_end;
	uint32_t byte_offset;
	char *start_addr;

	key = k_spin_lock(&msgq->lock);

	if (msgq->used_msgs > idx) {
		bytes_to_end = (msgq->buffer_end - msgq->read_ptr);
		byte_offset = idx * msgq->msg_size;
		start_addr = msgq->read_ptr;
		/* check item available in start/end of ring buffer */
		if (bytes_to_end <= byte_offset) {
			/* Tweak the values in case */
			byte_offset -= bytes_to_end;
			/* wrap-around is required */
			start_addr = msgq->buffer_start;
		}
		(void)memcpy(data, start_addr + byte_offset, msgq->msg_size);
		result = 0;
	} else {
		/* don't wait for a message to become available */
		result = -ENOMSG;
	}

	SYS_PORT_TRACING_OBJ_FUNC(k_msgq, peek, msgq, result);

	k_spin_unlock(&msgq->lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_msgq_peek_at(struct k_msgq *msgq, void *data, uint32_t idx)
{
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, msgq->msg_size));

	return z_impl_k_msgq_peek_at(msgq, data, idx);
}
#include <syscalls/k_msgq_peek_at_mrsh.c>
#endif

void z_impl_k_msgq_purge(struct k_msgq *msgq)
{
	k_spinlock_key_t key;
	struct k_thread *pending_thread;

	key = k_spin_lock(&msgq->lock);

	SYS_PORT_TRACING_OBJ_FUNC(k_msgq, purge, msgq);

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
static inline void z_vrfy_k_msgq_purge(struct k_msgq *msgq)
{
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	z_impl_k_msgq_purge(msgq);
}
#include <syscalls/k_msgq_purge_mrsh.c>

static inline uint32_t z_vrfy_k_msgq_num_free_get(struct k_msgq *msgq)
{
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	return z_impl_k_msgq_num_free_get(msgq);
}
#include <syscalls/k_msgq_num_free_get_mrsh.c>

static inline uint32_t z_vrfy_k_msgq_num_used_get(struct k_msgq *msgq)
{
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	return z_impl_k_msgq_num_used_get(msgq);
}
#include <syscalls/k_msgq_num_used_get_mrsh.c>

#endif

#ifdef CONFIG_OBJ_CORE_MSGQ
static int init_msgq_obj_core_list(void)
{
	/* Initialize msgq object type */

	z_obj_type_init(&obj_type_msgq, K_OBJ_TYPE_MSGQ_ID,
			offsetof(struct k_msgq, obj_core));

	/* Initialize and link statically defined message queues */

	STRUCT_SECTION_FOREACH(k_msgq, msgq) {
		k_obj_core_init_and_link(K_OBJ_CORE(msgq), &obj_type_msgq);
	}

	return 0;
};

SYS_INIT(init_msgq_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif
