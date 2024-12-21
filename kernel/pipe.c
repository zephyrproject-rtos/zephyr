#include <zephyr/kernel.h>
#include <ksched.h>
#include <kthread.h>
#include <wait_q.h>
#include <errno.h>

typedef bool (*wait_cond_t)(struct k_pipe *pipe);
static bool pipe_full(struct k_pipe *pipe)
{
	return ring_buf_space_get(&pipe->buf) == 0;
}

static bool pipe_empty(struct k_pipe *pipe)
{
	return ring_buf_capacity_get(&pipe->buf) - ring_buf_space_get(&pipe->buf) == 0;
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

	pipe->lock = (struct k_spinlock){0};
	z_waitq_init(&pipe->data);
	z_waitq_init(&pipe->space);
}

int z_impl_k_pipe_write(struct k_pipe *pipe, const uint8_t *data, size_t len, k_timeout_t timeout)
{
	int rc;

	__ASSERT_NO_MSG(pipe != NULL);
	__ASSERT_NO_MSG(data != NULL);

	k_spinlock_key_t key = k_spin_lock(&pipe->lock);
	if (unlikely(pipe_full(pipe))) {
		rc = wait_for(&pipe->space, pipe, &key, pipe_full, timeout);
		if (rc) {
			k_spin_unlock(&pipe->lock, key);
			return rc;
		}
	}

	if (!(pipe->flags & PIPE_FLAG_OPEN)) {
		k_spin_unlock(&pipe->lock, key);
		return -EPIPE;
	}

	rc = ring_buf_put(&pipe->buf, data, len);
	if (rc) {
		notify_waiter(&pipe->data);
	}
	k_spin_unlock(&pipe->lock, key);
	return rc;
}

int z_impl_k_pipe_read(struct k_pipe *pipe, uint8_t *data, size_t len, k_timeout_t timeout)
{
	int rc;

	__ASSERT_NO_MSG(pipe != NULL);
	__ASSERT_NO_MSG(data != NULL);

	k_spinlock_key_t key = k_spin_lock(&pipe->lock);
	if (pipe_empty(pipe) && pipe->flags & PIPE_FLAG_OPEN) {
		rc = wait_for(&pipe->data, pipe, &key, pipe_empty, timeout);
		if (rc && rc != -EPIPE) {
			k_spin_unlock(&pipe->lock, key);
			return rc;
		}
	}
	if (pipe_empty(pipe) && !(pipe->flags & PIPE_FLAG_OPEN)) {
		k_spin_unlock(&pipe->lock, key);
		return -EPIPE;
	}

	rc = ring_buf_get(&pipe->buf, data, len);
	if (rc) {
		notify_waiter(&pipe->space);
	}
	k_spin_unlock(&pipe->lock, key);
	return rc;
}

int z_impl_k_pipe_reset(struct k_pipe *pipe)
{
	__ASSERT_NO_MSG(pipe != NULL);
	K_SPINLOCK(&pipe->lock) {
		ring_buf_reset(&pipe->buf);
		pipe->flags |= PIPE_FLAG_RESET;
		z_unpend_all(&pipe->data);
		z_unpend_all(&pipe->space);
	}

	return 0;
}

int z_impl_k_pipe_close(struct k_pipe *pipe)
{
	int rc = 0;

	__ASSERT_NO_MSG(pipe != NULL);
	K_SPINLOCK(&pipe->lock) {
		if (!(pipe->flags & PIPE_FLAG_OPEN)) {
			rc = -EALREADY;
			K_SPINLOCK_BREAK;
		}
		pipe->flags = 0;
		z_unpend_all(&pipe->data);
		z_unpend_all(&pipe->space);
	}
	return rc;
}
