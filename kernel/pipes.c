/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Pipes
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#include <zephyr/toolchain.h>
#include <ksched.h>
#include <zephyr/wait_q.h>
#include <zephyr/init.h>
#include <zephyr/syscall_handler.h>
#include <kernel_internal.h>
#include <zephyr/sys/check.h>

struct k_pipe_desc {
	sys_dnode_t    node;
	unsigned char *buffer;           /* Position in src/dest buffer */
	size_t bytes_to_xfer;            /* # bytes left to transfer */
	struct k_thread *thread;         /* Owning thread */
};

struct k_pipe_info {
	unsigned char *buffer;
	size_t         start_index;
	size_t         end_index;
	size_t         size;
};

static int pipe_get_internal(k_spinlock_key_t key, struct k_pipe *pipe,
			     void *data, size_t bytes_to_read,
			     size_t *bytes_read, size_t min_xfer,
			     k_timeout_t timeout);

void k_pipe_init(struct k_pipe *pipe, unsigned char *buffer, size_t size)
{
	pipe->buffer = buffer;
	pipe->size = size;
	pipe->bytes_used = 0;
	pipe->read_index = 0;
	pipe->write_index = 0;
	pipe->lock = (struct k_spinlock){};
	z_waitq_init(&pipe->wait_q.writers);
	z_waitq_init(&pipe->wait_q.readers);
	SYS_PORT_TRACING_OBJ_INIT(k_pipe, pipe);

	pipe->flags = 0;
	z_object_init(pipe);
}

int z_impl_k_pipe_alloc_init(struct k_pipe *pipe, size_t size)
{
	void *buffer;
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, alloc_init, pipe);

	if (size != 0U) {
		buffer = z_thread_malloc(size);
		if (buffer != NULL) {
			k_pipe_init(pipe, buffer, size);
			pipe->flags = K_PIPE_FLAG_ALLOC;
			ret = 0;
		} else {
			ret = -ENOMEM;
		}
	} else {
		k_pipe_init(pipe, NULL, 0);
		ret = 0;
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, alloc_init, pipe, ret);

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_pipe_alloc_init(struct k_pipe *pipe, size_t size)
{
	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(pipe, K_OBJ_PIPE));

	return z_impl_k_pipe_alloc_init(pipe, size);
}
#include <syscalls/k_pipe_alloc_init_mrsh.c>
#endif

void z_impl_k_pipe_flush(struct k_pipe *pipe)
{
	size_t  bytes_read;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, flush, pipe);

	k_spinlock_key_t key = k_spin_lock(&pipe->lock);

	(void) pipe_get_internal(key, pipe, NULL, (size_t) -1, &bytes_read, 0,
				 K_NO_WAIT);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, flush, pipe);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_pipe_flush(struct k_pipe *pipe)
{
	Z_OOPS(Z_SYSCALL_OBJ(pipe, K_OBJ_PIPE));

	z_impl_k_pipe_flush(pipe);
}
#include <syscalls/k_pipe_flush_mrsh.c>
#endif

void z_impl_k_pipe_buffer_flush(struct k_pipe *pipe)
{
	size_t  bytes_read;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, buffer_flush, pipe);

	k_spinlock_key_t key = k_spin_lock(&pipe->lock);

	if (pipe->buffer != NULL) {
		(void) pipe_get_internal(key, pipe, NULL, pipe->size,
					 &bytes_read, 0, K_NO_WAIT);
	} else {
		k_spin_unlock(&pipe->lock, key);
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, buffer_flush, pipe);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_pipe_buffer_flush(struct k_pipe *pipe)
{
	Z_OOPS(Z_SYSCALL_OBJ(pipe, K_OBJ_PIPE));

	z_impl_k_pipe_buffer_flush(pipe);
}
#endif

int k_pipe_cleanup(struct k_pipe *pipe)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, cleanup, pipe);

	k_spinlock_key_t key = k_spin_lock(&pipe->lock);

	CHECKIF(z_waitq_head(&pipe->wait_q.readers) != NULL ||
			z_waitq_head(&pipe->wait_q.writers) != NULL) {
		k_spin_unlock(&pipe->lock, key);

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, cleanup, pipe, -EAGAIN);

		return -EAGAIN;
	}

	if ((pipe->flags & K_PIPE_FLAG_ALLOC) != 0U) {
		k_free(pipe->buffer);
		pipe->buffer = NULL;

		/*
		 * Freeing the buffer changes the pipe into a bufferless
		 * pipe. Reset the pipe's counters to prevent malfunction.
		 */

		pipe->size = 0;
		pipe->bytes_used = 0;
		pipe->read_index = 0;
		pipe->write_index = 0;
		pipe->flags &= ~K_PIPE_FLAG_ALLOC;
	}

	k_spin_unlock(&pipe->lock, key);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, cleanup, pipe, 0);

	return 0;
}

/**
 * @brief Copy bytes from @a src to @a dest
 *
 * @return Number of bytes copied
 */
static size_t pipe_xfer(unsigned char *dest, size_t dest_size,
			 const unsigned char *src, size_t src_size)
{
	size_t num_bytes = MIN(dest_size, src_size);
	const unsigned char *end = src + num_bytes;

	if (dest == NULL) {
		/* Data is being flushed. Pretend the data was copied. */
		return num_bytes;
	}

	while (src != end) {
		*dest = *src;
		dest++;
		src++;
	}

	return num_bytes;
}

/**
 * @brief Popluate pipe descriptors for copying to/from waiters' buffers
 *
 * This routine cycles through the waiters on the wait queue and creates
 * a list of threads that will have data directly copied to / read from
 * their buffers. This list helps us avoid double copying later.
 *
 * @return # of bytes available for direct copying
 */
static size_t pipe_waiter_list_populate(sys_dlist_t  *list,
					_wait_q_t    *wait_q,
					size_t        bytes_to_xfer)
{
	struct k_thread  *thread;
	struct k_pipe_desc *curr;
	size_t num_bytes = 0;

	_WAIT_Q_FOR_EACH(wait_q, thread) {
		curr = (struct k_pipe_desc *)thread->base.swap_data;

		sys_dlist_append(list, &curr->node);

		num_bytes += curr->bytes_to_xfer;
		if (num_bytes >= bytes_to_xfer) {
			break;
		}
	}

	return num_bytes;
}

/**
 * @brief Populate pipe descriptors for copying to/from pipe buffer
 *
 * This routine is only called if the pipe buffer is not empty (when reading),
 * or if not full (when writing).
 */
static size_t pipe_buffer_list_populate(sys_dlist_t         *list,
					struct k_pipe_desc  *desc,
					unsigned char       *buffer,
					size_t               size,
					size_t               start,
					size_t               end)
{
	sys_dlist_append(list, &desc[0].node);

	desc[0].thread = NULL;
	desc[0].buffer = &buffer[start];

	if (start < end) {
		desc[0].bytes_to_xfer = end - start;
		return end - start;
	}

	desc[0].bytes_to_xfer = size - start;

	desc[1].thread = NULL;
	desc[1].buffer = &buffer[0];
	desc[1].bytes_to_xfer = end;

	sys_dlist_append(list, &desc[1].node);

	return size - start + end;
}

/**
 * @brief Determine the correct return code
 *
 * Bytes Xferred   No Wait   Wait
 *   >= Minimum       0       0
 *    < Minimum      -EIO*   -EAGAIN
 *
 * * The "-EIO No Wait" case was already checked after the list of pipe
 *   descriptors was created.
 *
 * @return See table above
 */
static int pipe_return_code(size_t min_xfer, size_t bytes_remaining,
			     size_t bytes_requested)
{
	if (bytes_requested - bytes_remaining >= min_xfer) {
		/*
		 * At least the minimum number of requested
		 * bytes have been transferred.
		 */
		return 0;
	}

	return -EAGAIN;
}

int z_impl_k_pipe_put(struct k_pipe *pipe, void *data, size_t bytes_to_write,
		     size_t *bytes_written, size_t min_xfer,
		      k_timeout_t timeout)
{
	struct k_pipe_desc  pipe_desc[2];
	sys_dlist_t         list;
	size_t              bytes_copied;
	size_t              bytes_can_write;
	bool                reschedule_needed = false;

	__ASSERT(((arch_is_in_isr() == false) ||
		  K_TIMEOUT_EQ(timeout, K_NO_WAIT)), "");

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, put, pipe, timeout);

	CHECKIF((min_xfer > bytes_to_write) || bytes_written == NULL) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe, timeout, -EINVAL);

		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&pipe->lock);

	/*
	 * If there are any waiting readers then the pipe's buffer must be
	 * empty and we then want to copy directly to the waiting readers'
	 * buffers. If the pipe's buffer is not empty, we want to copy as
	 * much data as possible into the pipe's buffer.
	 *
	 * For both cases, we create a linked list of pipe descriptors that
	 * can then be traversed to perform a sort of scatter-gather copy.
	 */

	sys_dlist_init(&list);

	bytes_can_write = pipe_waiter_list_populate(&list,
						    &pipe->wait_q.readers,
						    bytes_to_write);

	if (pipe->bytes_used != pipe->size) {
		bytes_can_write += pipe_buffer_list_populate(&list,
							     pipe_desc,
							     pipe->buffer,
							     pipe->size,
							     pipe->write_index,
							     pipe->read_index);
	}

	if ((bytes_can_write < min_xfer) &&
	    (K_TIMEOUT_EQ(timeout, K_NO_WAIT))) {

		/* The request can not be fulfilled. */

		k_spin_unlock(&pipe->lock, key);
		*bytes_written = 0;

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe,
					       timeout, -EIO);

		return -EIO;
	}

	struct k_pipe_desc *desc;
	size_t         num_bytes_written = 0;

	desc = (struct k_pipe_desc *)sys_dlist_get(&list);
	while (desc != NULL) {
		bytes_copied = pipe_xfer(desc->buffer, desc->bytes_to_xfer,
					  (uint8_t *)data + num_bytes_written,
					  bytes_to_write - num_bytes_written);

		num_bytes_written   += bytes_copied;
		desc->buffer        += bytes_copied;
		desc->bytes_to_xfer -= bytes_copied;

		if (desc->thread == NULL) {

			/* Writing to the pipe buffer. Update details. */

			pipe->bytes_used += bytes_copied;
			pipe->write_index += bytes_copied;
			if (pipe->write_index >= pipe->size) {
				pipe->write_index -= pipe->size;
			}
		} else if (desc->bytes_to_xfer == 0) {

			/* A thread's read request has been satisfied. */

			z_unpend_thread(desc->thread);
			z_ready_thread(desc->thread);
			reschedule_needed = true;
		}
		desc = (struct k_pipe_desc *)sys_dlist_get(&list);
	}

	if (num_bytes_written >= min_xfer) {

		/* The minimum amount of data has been copied */

		*bytes_written = num_bytes_written;

		if (reschedule_needed) {
			z_reschedule(&pipe->lock, key);
		} else {
			k_spin_unlock(&pipe->lock, key);
		}

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe, timeout, 0);

		return 0;
	}

	/*
	 * The minimum amount of data has not been copied. Block.
	 * (We never reach here on a flush.)
	 */

	SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_pipe, put, pipe, timeout);

	struct k_pipe_desc  desc_for_waiter;

	desc_for_waiter.buffer        = (uint8_t *)data + num_bytes_written;
	desc_for_waiter.bytes_to_xfer = bytes_to_write - num_bytes_written;
	desc_for_waiter.thread        = _current;

	_current->base.swap_data = &desc_for_waiter;

	z_pend_curr(&pipe->lock, key, &pipe->wait_q.writers, timeout);

	*bytes_written = bytes_to_write - desc_for_waiter.bytes_to_xfer;

	int ret = pipe_return_code(min_xfer, desc_for_waiter.bytes_to_xfer,
				   bytes_to_write);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe, timeout, ret);

	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_pipe_put(struct k_pipe *pipe, void *data, size_t bytes_to_write,
		     size_t *bytes_written, size_t min_xfer,
		      k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(bytes_written, sizeof(*bytes_written)));
	Z_OOPS(Z_SYSCALL_MEMORY_READ((void *)data, bytes_to_write));

	return z_impl_k_pipe_put((struct k_pipe *)pipe, (void *)data,
				bytes_to_write, bytes_written, min_xfer,
				timeout);
}
#include <syscalls/k_pipe_put_mrsh.c>
#endif

static int pipe_get_internal(k_spinlock_key_t key, struct k_pipe *pipe,
			     void *data, size_t bytes_to_read,
			     size_t *bytes_read, size_t min_xfer,
			     k_timeout_t timeout)
{
	struct k_pipe_desc  pipe_desc[2];
	sys_dlist_t         list;
	struct k_pipe_desc *desc;
	size_t         num_bytes_read = 0;
	size_t         data_off;
	size_t         bytes_copied;
	size_t         bytes_can_read = 0;
	bool           reschedule_needed = false;

	/*
	 * If the pipe's buffer is not empty, we want to copy as much data as
	 * possible into the requesting reader's buffer. If there are any
	 * waiting writers, then the pipe's buffer must be full and we then
	 * want to copy directly to the requesting reader's buffer.
	 *
	 * For both cases, we create a linked list of pipe descriptors that
	 * can then be traversed to perform a sort of scatter-gather copy.
	 */

	sys_dlist_init(&list);

	if (pipe->bytes_used != 0) {
		bytes_can_read = pipe_buffer_list_populate(&list,
							   pipe_desc,
							   pipe->buffer,
							   pipe->size,
							   pipe->read_index,
							   pipe->write_index);
	}

	bytes_can_read += pipe_waiter_list_populate(&list,
						    &pipe->wait_q.writers,
						    bytes_to_read);

	if ((bytes_can_read < min_xfer) &&
	    (K_TIMEOUT_EQ(timeout, K_NO_WAIT))) {

		/* The request can not be fulfilled. */

		k_spin_unlock(&pipe->lock, key);
		*bytes_read = 0;

		return -EIO;
	}

	desc = (struct k_pipe_desc *)sys_dlist_get(&list);
	while (desc != NULL) {
		data_off = (data == NULL) ? 0 : num_bytes_read;
		bytes_copied = pipe_xfer((uint8_t *)data + data_off,
					  bytes_to_read - num_bytes_read,
					  desc->buffer, desc->bytes_to_xfer);

		num_bytes_read       += bytes_copied;
		desc->buffer         += bytes_copied;
		desc->bytes_to_xfer  -= bytes_copied;

		if (desc->thread == NULL) {

			/* Reading from the pipe buffer. Update details. */

			pipe->bytes_used -= bytes_copied;
			pipe->read_index += bytes_copied;
			if (pipe->read_index >= pipe->size) {
				pipe->read_index -= pipe->size;
			}
		} else if (desc->bytes_to_xfer == 0) {

			/* The thread's read request has been satisfied. */

			z_unpend_thread(desc->thread);
			z_ready_thread(desc->thread);
			reschedule_needed = true;
		}
		desc = (struct k_pipe_desc *)sys_dlist_get(&list);
	}

	if (num_bytes_read >= min_xfer) {

		/* The minimum amount of data has been copied */

		*bytes_read = num_bytes_read;
		if (reschedule_needed) {
			z_reschedule(&pipe->lock, key);
		} else {
			k_spin_unlock(&pipe->lock, key);
		}

		return 0;
	}

	/* The minimum amount of data has not been copied. Block. */

	SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_pipe, get, pipe, timeout);

	struct k_pipe_desc  desc_for_waiter;

	desc_for_waiter.buffer        = (uint8_t *)data + num_bytes_read;
	desc_for_waiter.bytes_to_xfer = bytes_to_read - num_bytes_read;
	desc_for_waiter.thread        = _current;

	_current->base.swap_data = &desc_for_waiter;

	z_pend_curr(&pipe->lock, key, &pipe->wait_q.readers, timeout);

	*bytes_read = bytes_to_read - desc_for_waiter.bytes_to_xfer;

	int ret = pipe_return_code(min_xfer, desc_for_waiter.bytes_to_xfer,
				   bytes_to_read);

	return ret;
}

int z_impl_k_pipe_get(struct k_pipe *pipe, void *data, size_t bytes_to_read,
		     size_t *bytes_read, size_t min_xfer, k_timeout_t timeout)
{
	__ASSERT(((arch_is_in_isr() == false) ||
		  K_TIMEOUT_EQ(timeout, K_NO_WAIT)), "");

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, get, pipe, timeout);

	CHECKIF((min_xfer > bytes_to_read) || bytes_read == NULL) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, get, pipe,
					       timeout, -EINVAL);

		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&pipe->lock);

	int ret = pipe_get_internal(key, pipe, data, bytes_to_read, bytes_read,
				    min_xfer, timeout);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, get, pipe, timeout, ret);

	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_pipe_get(struct k_pipe *pipe, void *data, size_t bytes_to_read,
		      size_t *bytes_read, size_t min_xfer, k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(bytes_read, sizeof(*bytes_read)));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE((void *)data, bytes_to_read));

	return z_impl_k_pipe_get((struct k_pipe *)pipe, (void *)data,
				bytes_to_read, bytes_read, min_xfer,
				timeout);
}
#include <syscalls/k_pipe_get_mrsh.c>
#endif

size_t z_impl_k_pipe_read_avail(struct k_pipe *pipe)
{
	size_t res;
	k_spinlock_key_t key;

	/* Buffer and size are fixed. No need to spin. */
	if (pipe->buffer == NULL || pipe->size == 0U) {
		res = 0;
		goto out;
	}

	key = k_spin_lock(&pipe->lock);

	if (pipe->read_index == pipe->write_index) {
		res = pipe->bytes_used;
	} else if (pipe->read_index < pipe->write_index) {
		res = pipe->write_index - pipe->read_index;
	} else {
		res = pipe->size - (pipe->read_index - pipe->write_index);
	}

	k_spin_unlock(&pipe->lock, key);

out:
	return res;
}

#ifdef CONFIG_USERSPACE
size_t z_vrfy_k_pipe_read_avail(struct k_pipe *pipe)
{
	Z_OOPS(Z_SYSCALL_OBJ(pipe, K_OBJ_PIPE));

	return z_impl_k_pipe_read_avail(pipe);
}
#include <syscalls/k_pipe_read_avail_mrsh.c>
#endif

size_t z_impl_k_pipe_write_avail(struct k_pipe *pipe)
{
	size_t res;
	k_spinlock_key_t key;

	/* Buffer and size are fixed. No need to spin. */
	if (pipe->buffer == NULL || pipe->size == 0U) {
		res = 0;
		goto out;
	}

	key = k_spin_lock(&pipe->lock);

	if (pipe->write_index == pipe->read_index) {
		res = pipe->size - pipe->bytes_used;
	} else if (pipe->write_index < pipe->read_index) {
		res = pipe->read_index - pipe->write_index;
	} else {
		res = pipe->size - (pipe->write_index - pipe->read_index);
	}

	k_spin_unlock(&pipe->lock, key);

out:
	return res;
}

#ifdef CONFIG_USERSPACE
size_t z_vrfy_k_pipe_write_avail(struct k_pipe *pipe)
{
	Z_OOPS(Z_SYSCALL_OBJ(pipe, K_OBJ_PIPE));

	return z_impl_k_pipe_write_avail(pipe);
}
#include <syscalls/k_pipe_write_avail_mrsh.c>
#endif
