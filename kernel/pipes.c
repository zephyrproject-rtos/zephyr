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
#include <wait_q.h>
#include <zephyr/init.h>
#include <zephyr/syscall_handler.h>
#include <kernel_internal.h>
#include <zephyr/sys/check.h>

struct waitq_walk_data {
	sys_dlist_t *list;
	size_t       bytes_requested;
	size_t       bytes_available;
};

static int pipe_get_internal(k_spinlock_key_t key, struct k_pipe *pipe,
			     void *data, size_t bytes_to_read,
			     size_t *bytes_read, size_t min_xfer,
			     k_timeout_t timeout);
#ifdef CONFIG_OBJ_CORE_PIPE
static struct k_obj_type obj_type_pipe;
#endif


void k_pipe_init(struct k_pipe *pipe, unsigned char *buffer, size_t size)
{
	pipe->buffer = buffer;
	pipe->size = size;
	pipe->bytes_used = 0U;
	pipe->read_index = 0U;
	pipe->write_index = 0U;
	pipe->lock = (struct k_spinlock){};
	z_waitq_init(&pipe->wait_q.writers);
	z_waitq_init(&pipe->wait_q.readers);
	SYS_PORT_TRACING_OBJ_INIT(k_pipe, pipe);

	pipe->flags = 0;

#if defined(CONFIG_POLL)
	sys_dlist_init(&pipe->poll_events);
#endif
	z_object_init(pipe);

#ifdef CONFIG_OBJ_CORE_PIPE
	k_obj_core_init_and_link(K_OBJ_CORE(pipe), &obj_type_pipe);
#endif
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
		k_pipe_init(pipe, NULL, 0U);
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

static inline void handle_poll_events(struct k_pipe *pipe)
{
#ifdef CONFIG_POLL
	z_handle_obj_poll_events(&pipe->poll_events, K_POLL_STATE_PIPE_DATA_AVAILABLE);
#else
	ARG_UNUSED(pipe);
#endif
}

void z_impl_k_pipe_flush(struct k_pipe *pipe)
{
	size_t  bytes_read;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, flush, pipe);

	k_spinlock_key_t key = k_spin_lock(&pipe->lock);

	(void) pipe_get_internal(key, pipe, NULL, (size_t) -1, &bytes_read, 0U,
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
					 &bytes_read, 0U, K_NO_WAIT);
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

		pipe->size = 0U;
		pipe->bytes_used = 0U;
		pipe->read_index = 0U;
		pipe->write_index = 0U;
		pipe->flags &= ~K_PIPE_FLAG_ALLOC;
	}

	k_spin_unlock(&pipe->lock, key);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, cleanup, pipe, 0U);

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

	if (dest == NULL) {
		/* Data is being flushed. Pretend the data was copied. */
		return num_bytes;
	}

	(void) memcpy(dest, src, num_bytes);

	return num_bytes;
}

/**
 * @brief Callback routine used to populate wait list
 *
 * @return 1 to stop further walking; 0 to continue walking
 */
static int pipe_walk_op(struct k_thread *thread, void *data)
{
	struct waitq_walk_data *walk_data = data;
	struct _pipe_desc *desc = (struct _pipe_desc *)thread->base.swap_data;

	sys_dlist_append(walk_data->list, &desc->node);

	walk_data->bytes_available += desc->bytes_to_xfer;

	if (walk_data->bytes_available >= walk_data->bytes_requested) {
		return 1;
	}

	return 0;
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
	struct waitq_walk_data walk_data;

	walk_data.list            = list;
	walk_data.bytes_requested = bytes_to_xfer;
	walk_data.bytes_available = 0;

	(void) z_sched_waitq_walk(wait_q, pipe_walk_op, &walk_data);

	return walk_data.bytes_available;
}

/**
 * @brief Populate pipe descriptors for copying to/from pipe buffer
 *
 * This routine is only called if the pipe buffer is not empty (when reading),
 * or if not full (when writing).
 */
static size_t pipe_buffer_list_populate(sys_dlist_t         *list,
					struct _pipe_desc   *desc,
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

/**
 * @brief Copy data from source(s) to destination(s)
 */

static size_t pipe_write(struct k_pipe *pipe, sys_dlist_t *src_list,
			 sys_dlist_t *dest_list, bool *reschedule)
{
	struct _pipe_desc *src;
	struct _pipe_desc *dest;
	size_t  bytes_copied;
	size_t  num_bytes_written = 0U;

	src = (struct _pipe_desc *)sys_dlist_get(src_list);
	dest = (struct _pipe_desc *)sys_dlist_get(dest_list);

	while ((src != NULL) && (dest != NULL)) {
		bytes_copied = pipe_xfer(dest->buffer, dest->bytes_to_xfer,
					 src->buffer, src->bytes_to_xfer);

		num_bytes_written   += bytes_copied;

		dest->buffer        += bytes_copied;
		dest->bytes_to_xfer -= bytes_copied;

		src->buffer         += bytes_copied;
		src->bytes_to_xfer  -= bytes_copied;

		if (dest->thread == NULL) {

			/* Writing to the pipe buffer. Update details. */

			pipe->bytes_used += bytes_copied;
			pipe->write_index += bytes_copied;
			if (pipe->write_index >= pipe->size) {
				pipe->write_index -= pipe->size;
			}
		} else if (dest->bytes_to_xfer == 0U) {

			/* The thread's read request has been satisfied. */

			z_unpend_thread(dest->thread);
			z_ready_thread(dest->thread);

			*reschedule = true;
		}

		if (src->bytes_to_xfer == 0U) {
			src = (struct _pipe_desc *)sys_dlist_get(src_list);
		}

		if (dest->bytes_to_xfer == 0U) {
			dest = (struct _pipe_desc *)sys_dlist_get(dest_list);
		}
	}

	return num_bytes_written;
}

int z_impl_k_pipe_put(struct k_pipe *pipe, void *data, size_t bytes_to_write,
		     size_t *bytes_written, size_t min_xfer,
		      k_timeout_t timeout)
{
	struct _pipe_desc  pipe_desc[2];
	struct _pipe_desc  isr_desc;
	struct _pipe_desc *src_desc;
	sys_dlist_t        dest_list;
	sys_dlist_t        src_list;
	size_t             bytes_can_write;
	bool               reschedule_needed = false;

	__ASSERT(((arch_is_in_isr() == false) ||
		  K_TIMEOUT_EQ(timeout, K_NO_WAIT)), "");

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, put, pipe, timeout);

	CHECKIF((min_xfer > bytes_to_write) || bytes_written == NULL) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe, timeout,
					       -EINVAL);

		return -EINVAL;
	}

	sys_dlist_init(&src_list);
	sys_dlist_init(&dest_list);

	k_spinlock_key_t key = k_spin_lock(&pipe->lock);

	/*
	 * First, write to any waiting readers, if any exist.
	 * Second, write to the pipe buffer, if it exists.
	 */

	bytes_can_write = pipe_waiter_list_populate(&dest_list,
						    &pipe->wait_q.readers,
						    bytes_to_write);

	if (pipe->bytes_used != pipe->size) {
		bytes_can_write += pipe_buffer_list_populate(&dest_list,
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
		*bytes_written = 0U;

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe,
					       timeout, -EIO);

		return -EIO;
	}

	/*
	 * Do not use the pipe descriptor stored within k_thread if
	 * invoked from within an ISR as that is not safe to do.
	 */

	src_desc = k_is_in_isr() ? &isr_desc : &_current->pipe_desc;

	src_desc->buffer        = data;
	src_desc->bytes_to_xfer = bytes_to_write;
	src_desc->thread        = _current;
	sys_dlist_append(&src_list, &src_desc->node);

	*bytes_written = pipe_write(pipe, &src_list,
				    &dest_list, &reschedule_needed);

	/*
	 * Only handle poll events if the pipe has had some bytes written and
	 * there are bytes remaining after any pending readers have read from it
	 */

	if ((pipe->bytes_used != 0U) && (*bytes_written != 0U)) {
		handle_poll_events(pipe);
	}

	/*
	 * The immediate success conditions below are backwards
	 * compatible with an earlier pipe implementation.
	 */

	if ((*bytes_written == bytes_to_write) ||
	    (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) ||
	    ((*bytes_written >= min_xfer) && (min_xfer > 0U))) {

		/* The minimum amount of data has been copied */

		if (reschedule_needed) {
			z_reschedule(&pipe->lock, key);
		} else {
			k_spin_unlock(&pipe->lock, key);
		}

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe, timeout, 0);

		return 0;
	}

	/* The minimum amount of data has not been copied. Block. */

	SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_pipe, put, pipe, timeout);

	_current->base.swap_data = src_desc;

	z_sched_wait(&pipe->lock, key, &pipe->wait_q.writers, timeout, NULL);

	/*
	 * On SMP systems, threads in the processing list may timeout before
	 * the data has finished copying. The following spin lock/unlock pair
	 * prevents those threads from executing further until the data copying
	 * is complete.
	 */

	key = k_spin_lock(&pipe->lock);
	k_spin_unlock(&pipe->lock, key);

	*bytes_written = bytes_to_write - src_desc->bytes_to_xfer;

	int ret = pipe_return_code(min_xfer, src_desc->bytes_to_xfer,
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
	sys_dlist_t         src_list;
	struct _pipe_desc   pipe_desc[2];
	struct _pipe_desc   isr_desc;
	struct _pipe_desc  *dest_desc;
	struct _pipe_desc  *src_desc;
	size_t         num_bytes_read = 0U;
	size_t         bytes_copied;
	size_t         bytes_can_read = 0U;
	bool           reschedule_needed = false;

	/*
	 * Data copying takes place in the following order.
	 * 1. Copy data from the pipe buffer to the receive buffer.
	 * 2. Copy data from the waiting writer(s) to the receive buffer.
	 * 3. Refill the pipe buffer from the waiting writer(s).
	 */

	sys_dlist_init(&src_list);

	if (pipe->bytes_used != 0) {
		bytes_can_read = pipe_buffer_list_populate(&src_list,
							   pipe_desc,
							   pipe->buffer,
							   pipe->size,
							   pipe->read_index,
							   pipe->write_index);
	}

	bytes_can_read += pipe_waiter_list_populate(&src_list,
						    &pipe->wait_q.writers,
						    bytes_to_read);

	if ((bytes_can_read < min_xfer) &&
	    (K_TIMEOUT_EQ(timeout, K_NO_WAIT))) {

		/* The request can not be fulfilled. */

		k_spin_unlock(&pipe->lock, key);
		*bytes_read = 0;

		return -EIO;
	}

	/*
	 * Do not use the pipe descriptor stored within k_thread if
	 * invoked from within an ISR as that is not safe to do.
	 */

	dest_desc = k_is_in_isr() ? &isr_desc : &_current->pipe_desc;

	dest_desc->buffer = data;
	dest_desc->bytes_to_xfer = bytes_to_read;
	dest_desc->thread = _current;

	src_desc = (struct _pipe_desc *)sys_dlist_get(&src_list);
	while (src_desc != NULL) {
		bytes_copied = pipe_xfer(dest_desc->buffer,
					  dest_desc->bytes_to_xfer,
					  src_desc->buffer,
					  src_desc->bytes_to_xfer);

		num_bytes_read += bytes_copied;

		src_desc->buffer += bytes_copied;
		src_desc->bytes_to_xfer -= bytes_copied;

		if (dest_desc->buffer != NULL) {
			dest_desc->buffer += bytes_copied;
		}
		dest_desc->bytes_to_xfer -= bytes_copied;

		if (src_desc->thread == NULL) {

			/* Reading from the pipe buffer. Update details. */

			pipe->bytes_used -= bytes_copied;
			pipe->read_index += bytes_copied;
			if (pipe->read_index >= pipe->size) {
				pipe->read_index -= pipe->size;
			}
		} else if (src_desc->bytes_to_xfer == 0U) {

			/* The thread's write request has been satisfied. */

			z_unpend_thread(src_desc->thread);
			z_ready_thread(src_desc->thread);

			reschedule_needed = true;
		}
		src_desc = (struct _pipe_desc *)sys_dlist_get(&src_list);
	}

	if (pipe->bytes_used != pipe->size) {
		sys_dlist_t         pipe_list;

		/*
		 * The pipe is not full. If there are any waiting writers,
		 * refill the pipe.
		 */

		sys_dlist_init(&src_list);
		sys_dlist_init(&pipe_list);

		(void) pipe_waiter_list_populate(&src_list,
						 &pipe->wait_q.writers,
						 pipe->size - pipe->bytes_used);

		(void) pipe_buffer_list_populate(&pipe_list, pipe_desc,
						 pipe->buffer, pipe->size,
						 pipe->write_index,
						 pipe->read_index);

		(void) pipe_write(pipe, &src_list,
				  &pipe_list, &reschedule_needed);
	}

	/*
	 * The immediate success conditions below are backwards
	 * compatible with an earlier pipe implementation.
	 */

	if ((num_bytes_read == bytes_to_read) ||
	    (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) ||
	    ((num_bytes_read >= min_xfer) && (min_xfer > 0U))) {

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

	_current->base.swap_data = dest_desc;

	z_sched_wait(&pipe->lock, key, &pipe->wait_q.readers, timeout, NULL);

	/*
	 * On SMP systems, threads in the processing list may timeout before
	 * the data has finished copying. The following spin lock/unlock pair
	 * prevents those threads from executing further until the data copying
	 * is complete.
	 */

	key = k_spin_lock(&pipe->lock);
	k_spin_unlock(&pipe->lock, key);

	*bytes_read = bytes_to_read - dest_desc->bytes_to_xfer;

	int ret = pipe_return_code(min_xfer, dest_desc->bytes_to_xfer,
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
#endif
