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

typedef bool (*wait_cond_t)(struct k_pipe *pipe);
static bool pipe_full(struct k_pipe *pipe)
{
	return ring_buf_space_get(&pipe->buf) == 0;
}

static bool pipe_empty(struct k_pipe *pipe)
{
	return ring_buf_capacity_get(&pipe->buf) == ring_buf_space_get(&pipe->buf);
}

static int wait_for(_wait_q_t *waitq, struct k_pipe *pipe, k_spinlock_key_t *key,
		wait_cond_t cond, k_timeout_t timeout)
{
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT) || pipe->flags & PIPE_FLAG_RESET) {
		k_spin_unlock(&pipe->lock, *key);
		return -EAGAIN;
	}
	pipe->waiting++;
	z_pend_curr(&pipe->lock, *key, waitq, timeout);
	*key = k_spin_lock(&pipe->lock);
	pipe->waiting--;
	if (unlikely(!(pipe->flags & PIPE_FLAG_OPEN))) {
		return -EPIPE;
	} else if (unlikely(pipe->flags & PIPE_FLAG_RESET)) {
		if (!pipe->waiting) {
			pipe->flags &= ~PIPE_FLAG_RESET;
		}
		return -ECANCELED;
	} else if (!cond(pipe)) {
		return 0;
	}
	return -EAGAIN;
}

static void notify_waiter(_wait_q_t *waitq)
{
	struct k_thread *thread_to_unblock = z_unpend_first_thread(waitq);

	if (likely(thread_to_unblock)) {
		z_ready_thread(thread_to_unblock);
	}
}

void z_impl_k_pipe_init(struct k_pipe *pipe, uint8_t *buffer, size_t buffer_size)
{
	ring_buf_init(&pipe->buf, buffer_size, buffer);
	pipe->flags = PIPE_FLAG_OPEN;

	pipe->lock = (struct k_spinlock){};
	z_waitq_init(&pipe->data);
	z_waitq_init(&pipe->space);
#if defined(CONFIG_POLL)
	sys_dlist_init(&pipe->poll_events);
#endif /* CONFIG_POLL */
#ifdef CONFIG_OBJ_CORE_PIPE
	k_obj_core_init_and_link(K_OBJ_CORE(pipe), &obj_type_pipe);
#endif /* CONFIG_OBJ_CORE_PIPE */
}

int z_impl_k_pipe_write(struct k_pipe *pipe, const uint8_t *data, size_t len, k_timeout_t timeout)
{
	int rc;
	size_t written = 0;
	k_spinlock_key_t key;
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (written < len) {
		key = k_spin_lock(&pipe->lock);
		timeout = sys_timepoint_timeout(end);
		if (unlikely(pipe_full(pipe))) {
			rc = wait_for(&pipe->space, pipe, &key, pipe_full, timeout);
			if (rc == -EAGAIN) {
				k_spin_unlock(&pipe->lock, key);
				return written ? written : -EAGAIN;
			} else if (rc) {
				k_spin_unlock(&pipe->lock, key);
				return rc;
			}
		}

		if (!(pipe->flags & PIPE_FLAG_OPEN)) {
			k_spin_unlock(&pipe->lock, key);
			return -EPIPE;
		}

		rc = ring_buf_put(&pipe->buf, &data[written], len - written);
		if (rc) {
			notify_waiter(&pipe->data);
#ifdef CONFIG_POLL
			z_handle_obj_poll_events(&pipe->poll_events,
			    K_POLL_STATE_PIPE_DATA_AVAILABLE);
#endif /* CONFIG_POLL */
		}
		k_spin_unlock(&pipe->lock, key);
		written += rc;
	}
	return written;
}

int z_impl_k_pipe_read(struct k_pipe *pipe, uint8_t *data, size_t len, k_timeout_t timeout)
{
	int rc;
	size_t read = 0;
	k_spinlock_key_t key;
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (read < len) {
		key = k_spin_lock(&pipe->lock);
		timeout = sys_timepoint_timeout(end);
		if (pipe_empty(pipe) && pipe->flags & PIPE_FLAG_OPEN) {
			rc = wait_for(&pipe->data, pipe, &key, pipe_empty, timeout);
			if (rc == -EAGAIN) {
				k_spin_unlock(&pipe->lock, key);
				return read ? read : -EAGAIN;
			} else if (rc && rc != -EPIPE) {
				k_spin_unlock(&pipe->lock, key);
				return rc;
			}
		}

		if (pipe_empty(pipe) && !(pipe->flags & PIPE_FLAG_OPEN)) {
			k_spin_unlock(&pipe->lock, key);
			return read ? read : -EPIPE;
		}

		rc = ring_buf_get(&pipe->buf, &data[read], len - read);
		if (rc) {
			notify_waiter(&pipe->space);
		}
		read += rc;
		k_spin_unlock(&pipe->lock, key);
	}

	return read;
}

void z_impl_k_pipe_flush(struct k_pipe *pipe)
{
	K_SPINLOCK(&pipe->lock) {
		pipe->flags |= PIPE_FLAG_RESET;
		ring_buf_reset(&pipe->buf);
		z_unpend_all(&pipe->data);
		z_unpend_all(&pipe->space);
	}
}

void z_impl_k_pipe_close(struct k_pipe *pipe)
{
	K_SPINLOCK(&pipe->lock) {
		pipe->flags = 0;
		z_unpend_all(&pipe->data);
		z_unpend_all(&pipe->space);
	}
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

void z_vrfy_k_pipe_flush(struct k_pipe *pipe)
{
	K_OOPS(K_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	z_impl_k_pipe_flush(pipe);
}
#include <zephyr/syscalls/k_pipe_flush_mrsh.c>

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
