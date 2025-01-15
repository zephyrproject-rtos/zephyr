/*
 * Copyright (c) 2024 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <ksched.h>
#include <kthread.h>
#include <wait_q.h>

#ifdef CONFIG_OBJ_CORE_PIPE
static struct k_obj_type obj_type_pipe;
#endif /* CONFIG_OBJ_CORE_PIPE */

static inline bool pipe_closed(struct k_pipe *pipe)
{
	return (pipe->flags & PIPE_FLAG_OPEN) == 0;
}

static inline bool pipe_resetting(struct k_pipe *pipe)
{
	return (pipe->flags & PIPE_FLAG_RESET) != 0;
}

static inline bool pipe_full(struct k_pipe *pipe)
{
	return ring_buf_space_get(&pipe->buf) == 0;
}

static inline bool pipe_empty(struct k_pipe *pipe)
{
	return ring_buf_is_empty(&pipe->buf);
}

static inline int wait_for(_wait_q_t *waitq, struct k_pipe *pipe, k_spinlock_key_t *key,
			   k_timepoint_t time_limit)
{
	k_timeout_t timeout = sys_timepoint_timeout(time_limit);

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return -EAGAIN;
	}

	pipe->waiting++;
	SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_pipe, read, pipe, timeout);
	z_pend_curr(&pipe->lock, *key, waitq, timeout);
	*key = k_spin_lock(&pipe->lock);
	pipe->waiting--;
	if (unlikely(pipe_closed(pipe))) {
		return -EPIPE;
	} else if (unlikely(pipe_resetting(pipe))) {
		if (pipe->waiting == 0) {
			pipe->flags &= ~PIPE_FLAG_RESET;
		}
		return -ECANCELED;
	} else if (sys_timepoint_expired(time_limit)) {
		return -EAGAIN;
	}

	return 0;
}

static void notify_waiter(_wait_q_t *waitq)
{
	struct k_thread *thread_to_unblock = z_unpend_first_thread(waitq);

	if (likely(thread_to_unblock != NULL)) {
		z_ready_thread(thread_to_unblock);
	}
}

void z_impl_k_pipe_init(struct k_pipe *pipe, uint8_t *buffer, size_t buffer_size)
{
	ring_buf_init(&pipe->buf, buffer_size, buffer);
	pipe->flags = PIPE_FLAG_OPEN;
	pipe->waiting = 0;

	pipe->lock = (struct k_spinlock){};
	z_waitq_init(&pipe->data);
	z_waitq_init(&pipe->space);
	k_object_init(pipe);

#ifdef CONFIG_POLL
	sys_dlist_init(&pipe->poll_events);
#endif /* CONFIG_POLL */
#ifdef CONFIG_OBJ_CORE_PIPE
	k_obj_core_init_and_link(K_OBJ_CORE(pipe), &obj_type_pipe);
#endif /* CONFIG_OBJ_CORE_PIPE */
	SYS_PORT_TRACING_OBJ_INIT(k_pipe, pipe, buffer, buffer_size);
}

int z_impl_k_pipe_write(struct k_pipe *pipe, const uint8_t *data, size_t len, k_timeout_t timeout)
{
	int rc;
	size_t written = 0;
	k_spinlock_key_t key;
	k_timepoint_t end = sys_timepoint_calc(timeout);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, write, pipe, data, len, timeout);
	while (written < len) {
		key = k_spin_lock(&pipe->lock);
		if (unlikely(pipe_closed(pipe))) {
			k_spin_unlock(&pipe->lock, key);
			rc = -EPIPE;
			goto exit;
		} else if (unlikely(pipe_resetting(pipe))) {
			k_spin_unlock(&pipe->lock, key);
			rc = -ECANCELED;
			goto exit;
		} else if (pipe_full(pipe)) {
			rc = wait_for(&pipe->space, pipe, &key, end);
			if (rc == -EAGAIN) {
				/* the timeout expired */
				k_spin_unlock(&pipe->lock, key);
				rc = written ? written : -EAGAIN;
				goto exit;
			} else if (unlikely(rc != 0)) {
				/* The pipe was closed or reseted while waiting for space */
				k_spin_unlock(&pipe->lock, key);
				goto exit;
			} else if (unlikely(pipe_full(pipe))) {
				/* Timeout has not elapsed, the pipe is open and not resetting,
				 * we've been notified of available space, but the notified space
				 * was consumed by another thread before the calling thread.
				 */
				k_spin_unlock(&pipe->lock, key);
				continue;
			}
			/* The timeout has not elapsed, we've been notified of
			 * available space, and the pipe is not full. Continue writing.
			 */
		}

		rc = ring_buf_put(&pipe->buf, &data[written], len - written);
		if (likely(rc != 0)) {
			notify_waiter(&pipe->data);
#ifdef CONFIG_POLL
			z_handle_obj_poll_events(&pipe->poll_events,
			    K_POLL_STATE_PIPE_DATA_AVAILABLE);
#endif /* CONFIG_POLL */
		}
		k_spin_unlock(&pipe->lock, key);
		written += rc;
	}
	rc = written;
exit:
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, write, pipe, rc);
	return rc;
}

int z_impl_k_pipe_read(struct k_pipe *pipe, uint8_t *data, size_t len, k_timeout_t timeout)
{
	int rc;
	size_t read = 0;
	k_spinlock_key_t key;
	k_timepoint_t end = sys_timepoint_calc(timeout);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, read, pipe, data, len, timeout);
	while (read < len) {
		key = k_spin_lock(&pipe->lock);
		if (unlikely(pipe_resetting(pipe))) {
			k_spin_unlock(&pipe->lock, key);
			rc = -ECANCELED;
			goto exit;
		} else if (pipe_empty(pipe) && !pipe_closed(pipe)) {
			rc = wait_for(&pipe->data, pipe, &key, end);
			if (rc == -EAGAIN) {
				/* The timeout elapsed */
				k_spin_unlock(&pipe->lock, key);
				rc = read ? read : -EAGAIN;
				goto exit;
			} else if (unlikely(rc == -ECANCELED)) {
				/* The pipe is being rested. */
				k_spin_unlock(&pipe->lock, key);
				goto exit;
			} else if (unlikely(rc == 0 && pipe_empty(pipe))) {
				/* Timeout has not elapsed, we've been notified of available bytes
				 * but they have been consumed by another thread before the calling
				 * thread.
				 */
				k_spin_unlock(&pipe->lock, key);
				continue;
			}
			/* The timeout has not elapsed, we've been notified of
			 * available bytes, and the pipe is not empty. Continue reading.
			 */
		}

		if (unlikely(pipe_closed(pipe) && pipe_empty(pipe))) {
			k_spin_unlock(&pipe->lock, key);
			rc = read ? read : -EPIPE;
			goto exit;
		}

		rc = ring_buf_get(&pipe->buf, &data[read], len - read);
		if (likely(rc != 0)) {
			notify_waiter(&pipe->space);
		}
		read += rc;
		k_spin_unlock(&pipe->lock, key);
	}
	rc = read;
exit:
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, read, pipe, rc);
	return rc;
}

void z_impl_k_pipe_reset(struct k_pipe *pipe)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, reset, pipe);
	K_SPINLOCK(&pipe->lock) {
		ring_buf_reset(&pipe->buf);
		if (likely(pipe->waiting != 0)) {
			pipe->flags |= PIPE_FLAG_RESET;
			z_unpend_all(&pipe->data);
			z_unpend_all(&pipe->space);
		}
	}
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, reset, pipe);
}

void z_impl_k_pipe_close(struct k_pipe *pipe)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, close, pipe);
	K_SPINLOCK(&pipe->lock) {
		pipe->flags = 0;
		z_unpend_all(&pipe->data);
		z_unpend_all(&pipe->space);
	}
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, close, pipe);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_pipe_init(struct k_pipe *pipe, uint8_t *buffer, size_t buffer_size)
{
	K_OOPS(K_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(buffer, buffer_size));

	z_impl_k_pipe_init(pipe, buffer, buffer_size);
}
#include <zephyr/syscalls/k_pipe_init_mrsh.c>

int z_vrfy_k_pipe_read(struct k_pipe *pipe, uint8_t *data, size_t len, k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(data, len));

	return z_impl_k_pipe_read(pipe, data, len, timeout);
}
#include <zephyr/syscalls/k_pipe_read_mrsh.c>

int z_vrfy_k_pipe_write(struct k_pipe *pipe, const uint8_t *data, size_t len, k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	K_OOPS(K_SYSCALL_MEMORY_READ(data, len));

	return z_impl_k_pipe_write(pipe, data, len, timeout);
}
#include <zephyr/syscalls/k_pipe_write_mrsh.c>

void z_vrfy_k_pipe_reset(struct k_pipe *pipe)
{
	K_OOPS(K_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	z_impl_k_pipe_reset(pipe);
}
#include <zephyr/syscalls/k_pipe_reset_mrsh.c>

void z_vrfy_k_pipe_close(struct k_pipe *pipe)
{
	K_OOPS(K_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	z_impl_k_pipe_close(pipe);
}
#include <zephyr/syscalls/k_pipe_close_mrsh.c>
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_OBJ_CORE_PIPE
static int init_pipe_obj_core_list(void)
{
	/* Initialize pipe object type */
	z_obj_type_init(&obj_type_pipe, K_OBJ_TYPE_PIPE_ID,
			offsetof(struct k_pipe, obj_core));

	/* Initialize and link statically defined pipes */
	STRUCT_SECTION_FOREACH(k_pipe, pipe) {
		k_obj_core_init_and_link(K_OBJ_CORE(pipe), &obj_type_pipe);
	}

	return 0;
}

SYS_INIT(init_pipe_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
#endif /* CONFIG_OBJ_CORE_PIPE */
