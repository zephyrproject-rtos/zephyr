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

static int wait_for(_wait_q_t *waitq, struct k_pipe *pipe, k_spinlock_key_t *key,
		    k_timepoint_t time_limit, bool *need_resched)
{
	k_timeout_t timeout = sys_timepoint_timeout(time_limit);
	int rc;

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return -EAGAIN;
	}

	pipe->waiting++;
	*need_resched = false;
	if (waitq == &pipe->space) {
		SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_pipe, write, pipe, timeout);
	} else {
		SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_pipe, read, pipe, timeout);
	}
	rc = z_pend_curr(&pipe->lock, *key, waitq, timeout);
	*key = k_spin_lock(&pipe->lock);
	pipe->waiting--;
	if (unlikely(pipe_resetting(pipe))) {
		if (pipe->waiting == 0) {
			pipe->flags &= ~PIPE_FLAG_RESET;
		}
		rc = -ECANCELED;
	}

	return rc;
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

struct pipe_buf_spec {
	uint8_t * const data;
	const size_t len;
	size_t used;
};

static size_t copy_to_pending_readers(struct k_pipe *pipe, bool *need_resched,
				      const uint8_t *data, size_t len)
{
	struct k_thread *reader = NULL;
	struct pipe_buf_spec *reader_buf;
	size_t copy_size, written = 0;

	/*
	 * Attempt a direct data copy to waiting readers if any.
	 * The copy has to be done under the scheduler lock to ensure all the
	 * needed data is copied to the target thread whose buffer spec lives
	 * on that thread's stack, and then the thread unpended only if it
	 * received all the data it wanted, without racing with a potential
	 * thread timeout/cancellation event.
	 */
	do {
		LOCK_SCHED_SPINLOCK {
			reader = _priq_wait_best(&pipe->data.waitq);
			if (reader == NULL) {
				K_SPINLOCK_BREAK;
			}

			reader_buf = reader->base.swap_data;
			copy_size = MIN(len - written,
					reader_buf->len - reader_buf->used);
			memcpy(&reader_buf->data[reader_buf->used],
			       &data[written], copy_size);
			written += copy_size;
			reader_buf->used += copy_size;

			if (reader_buf->used < reader_buf->len) {
				/* This reader wants more: don't unpend. */
				reader = NULL;
			} else {
				/*
				 * This reader has received all the data
				 * it was waiting for: wake it up with
				 * the scheduler lock still held.
				 */
				unpend_thread_no_timeout(reader);
				z_abort_thread_timeout(reader);
			}
		}
		if (reader != NULL) {
			/* rest of thread wake-up outside the scheduler lock */
			z_thread_return_value_set_with_data(reader, 0, NULL);
			z_ready_thread(reader);
			*need_resched = true;
		}
	} while (reader != NULL && written < len);

	return written;
}

int z_impl_k_pipe_write(struct k_pipe *pipe, const uint8_t *data, size_t len, k_timeout_t timeout)
{
	int rc;
	size_t written = 0;
	k_timepoint_t end = sys_timepoint_calc(timeout);
	k_spinlock_key_t key = k_spin_lock(&pipe->lock);
	bool need_resched = false;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, write, pipe, data, len, timeout);

	if (unlikely(pipe_resetting(pipe))) {
		rc = -ECANCELED;
		goto exit;
	}

	for (;;) {
		if (unlikely(pipe_closed(pipe))) {
			rc = -EPIPE;
			break;
		}

		if (pipe_empty(pipe)) {
			if (IS_ENABLED(CONFIG_KERNEL_COHERENCE)) {
				/*
				 * Systems that enabled this option don't have
				 * their stacks in coherent memory. Given our
				 * pipe_buf_spec is stored on the stack, and
				 * readers may also have their destination
				 * buffer on their stack too, it is not worth
				 * supporting direct-to-readers copy with them.
				 * Simply wake up all pending readers instead.
				 */
				need_resched = z_sched_wake_all(&pipe->data, 0, NULL);
			} else if (pipe->waiting != 0) {
				written += copy_to_pending_readers(pipe, &need_resched,
								   &data[written],
								   len - written);
				if (written >= len) {
					rc = written;
					break;
				}
			}
#ifdef CONFIG_POLL
			z_handle_obj_poll_events(&pipe->poll_events,
						 K_POLL_STATE_PIPE_DATA_AVAILABLE);
#endif /* CONFIG_POLL */
		}

		written += ring_buf_put(&pipe->buf, &data[written], len - written);
		if (likely(written == len)) {
			rc = written;
			break;
		}

		rc = wait_for(&pipe->space, pipe, &key, end, &need_resched);
		if (rc != 0) {
			if (rc == -EAGAIN) {
				rc = written ? written : -EAGAIN;
			}
			break;
		}
	}
exit:
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, write, pipe, rc);
	if (need_resched) {
		z_reschedule(&pipe->lock, key);
	} else {
		k_spin_unlock(&pipe->lock, key);
	}
	return rc;
}

int z_impl_k_pipe_read(struct k_pipe *pipe, uint8_t *data, size_t len, k_timeout_t timeout)
{
	struct pipe_buf_spec buf = { data, len, 0 };
	int rc;
	k_timepoint_t end = sys_timepoint_calc(timeout);
	k_spinlock_key_t key = k_spin_lock(&pipe->lock);
	bool need_resched = false;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, read, pipe, data, len, timeout);

	if (unlikely(pipe_resetting(pipe))) {
		rc = -ECANCELED;
		goto exit;
	}

	for (;;) {
		if (pipe_full(pipe)) {
			/* One or more pending writers may exist. */
			need_resched = z_sched_wake_all(&pipe->space, 0, NULL);
		}

		buf.used += ring_buf_get(&pipe->buf, &data[buf.used], len - buf.used);
		if (likely(buf.used == len)) {
			rc = buf.used;
			break;
		}

		if (unlikely(pipe_closed(pipe))) {
			rc = buf.used ? buf.used : -EPIPE;
			break;
		}

		/* provide our "direct copy" info to potential writers */
		_current->base.swap_data = &buf;

		rc = wait_for(&pipe->data, pipe, &key, end, &need_resched);
		if (rc != 0) {
			if (rc == -EAGAIN) {
				rc = buf.used ? buf.used : -EAGAIN;
			}
			break;
		}
	}
exit:
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, read, pipe, rc);
	if (need_resched) {
		z_reschedule(&pipe->lock, key);
	} else {
		k_spin_unlock(&pipe->lock, key);
	}
	return rc;
}

void z_impl_k_pipe_reset(struct k_pipe *pipe)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, reset, pipe);
	K_SPINLOCK(&pipe->lock) {
		ring_buf_reset(&pipe->buf);
		if (likely(pipe->waiting != 0)) {
			pipe->flags |= PIPE_FLAG_RESET;
			z_sched_wake_all(&pipe->data, 0, NULL);
			z_sched_wake_all(&pipe->space, 0, NULL);
		}
	}
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, reset, pipe);
}

void z_impl_k_pipe_close(struct k_pipe *pipe)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, close, pipe);
	K_SPINLOCK(&pipe->lock) {
		pipe->flags = 0;
		z_sched_wake_all(&pipe->data, 0, NULL);
		z_sched_wake_all(&pipe->space, 0, NULL);
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
