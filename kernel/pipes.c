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
	unsigned char *buffer;           /* Position in src/dest buffer */
	size_t bytes_to_xfer;            /* # bytes left to transfer */
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
 * @brief Put data from @a src into the pipe's circular buffer
 *
 * Modifies the following fields in @a pipe:
 *        buffer, bytes_used, write_index
 *
 * @return Number of bytes written to the pipe's circular buffer
 */
static size_t pipe_buffer_put(struct k_pipe *pipe,
			       const unsigned char *src, size_t src_size)
{
	size_t  bytes_copied;
	size_t  run_length;
	size_t  num_bytes_written = 0;
	int     i;


	for (i = 0; i < 2; i++) {
		run_length = MIN(pipe->size - pipe->bytes_used,
				 pipe->size - pipe->write_index);

		bytes_copied = pipe_xfer(pipe->buffer + pipe->write_index,
					  run_length,
					  src + num_bytes_written,
					  src_size - num_bytes_written);

		num_bytes_written += bytes_copied;
		pipe->bytes_used += bytes_copied;
		pipe->write_index += bytes_copied;
		if (pipe->write_index == pipe->size) {
			pipe->write_index = 0;
		}
	}

	return num_bytes_written;
}

/**
 * @brief Get data from the pipe's circular buffer
 *
 * Modifies the following fields in @a pipe:
 *        bytes_used, read_index
 *
 * @return Number of bytes read from the pipe's circular buffer
 */
static size_t pipe_buffer_get(struct k_pipe *pipe,
			       unsigned char *dest, size_t dest_size)
{
	size_t  bytes_copied;
	size_t  run_length;
	size_t  num_bytes_read = 0;
	size_t  dest_off;
	int     i;

	for (i = 0; i < 2; i++) {
		run_length = MIN(pipe->bytes_used,
				 pipe->size - pipe->read_index);

		dest_off = (dest == NULL) ? 0 : num_bytes_read;

		bytes_copied = pipe_xfer(dest + dest_off,
					 dest_size - num_bytes_read,
					 pipe->buffer + pipe->read_index,
					 run_length);

		num_bytes_read += bytes_copied;
		pipe->bytes_used -= bytes_copied;
		pipe->read_index += bytes_copied;
		if (pipe->read_index == pipe->size) {
			pipe->read_index = 0;
		}
	}

	return num_bytes_read;
}

/**
 * @brief Prepare a working set of readers/writers
 *
 * Prepare a list of "working threads" into/from which the data
 * will be directly copied. This list is useful as it is used to ...
 *
 *  1. avoid double copying
 *  2. minimize interrupt latency as interrupts are unlocked
 *     while copying data
 *  3. ensure a timeout can not make the request impossible to satisfy
 *
 * The list is populated with previously pended threads that will be ready to
 * run after the pipe call is complete.
 *
 * Important things to remember when reading from the pipe ...
 * 1. If there are writers int @a wait_q, then the pipe's buffer is full.
 * 2. Conversely if the pipe's buffer is not full, there are no writers.
 * 3. The amount of available data in the pipe is the sum the bytes used in
 *    the pipe (@a pipe_space) and all the requests from the waiting writers.
 * 4. Since data is read from the pipe's buffer first, the working set must
 *    include writers that will (try to) re-fill the pipe's buffer afterwards.
 *
 * Important things to remember when writing to the pipe ...
 * 1. If there are readers in @a wait_q, then the pipe's buffer is empty.
 * 2. Conversely if the pipe's buffer is not empty, then there are no readers.
 * 3. The amount of space available in the pipe is the sum of the bytes unused
 *    in the pipe (@a pipe_space) and all the requests from the waiting readers.
 *
 * @return false if request is unsatisfiable, otherwise true
 */
static bool pipe_xfer_prepare(sys_dlist_t      *xfer_list,
			       struct k_thread **waiter,
			       _wait_q_t        *wait_q,
			       size_t            pipe_space,
			       size_t            bytes_to_xfer,
			       size_t            min_xfer,
			       k_timeout_t           timeout)
{
	struct k_thread  *thread;
	struct k_pipe_desc *desc;
	size_t num_bytes = 0;

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		_WAIT_Q_FOR_EACH(wait_q, thread) {
			desc = (struct k_pipe_desc *)thread->base.swap_data;

			num_bytes += desc->bytes_to_xfer;

			if (num_bytes >= bytes_to_xfer) {
				break;
			}
		}

		if (num_bytes + pipe_space < min_xfer) {
			return false;
		}
	}

	/*
	 * Either @a timeout is not K_NO_WAIT (so the thread may pend) or
	 * the entire request can be satisfied. Generate the working list.
	 */

	sys_dlist_init(xfer_list);
	num_bytes = 0;

	while ((thread = z_waitq_head(wait_q)) != NULL) {
		desc = (struct k_pipe_desc *)thread->base.swap_data;
		num_bytes += desc->bytes_to_xfer;

		if (num_bytes > bytes_to_xfer) {
			/*
			 * This request can not be fully satisfied.
			 * Do not remove it from the wait_q.
			 * Do not abort its timeout (if applicable).
			 * Do not add it to the transfer list
			 */
			break;
		}

		/*
		 * This request can be fully satisfied.
		 * Remove it from the wait_q.
		 * Abort its timeout.
		 * Add it to the transfer list.
		 */
		z_unpend_thread(thread);
		sys_dlist_append(xfer_list, &thread->base.qnode_dlist);
	}

	*waiter = (num_bytes > bytes_to_xfer) ? thread : NULL;

	return true;
}

/**
 * @brief Determine the correct return code
 *
 * Bytes Xferred   No Wait   Wait
 *   >= Minimum       0       0
 *    < Minimum      -EIO*   -EAGAIN
 *
 * * The "-EIO No Wait" case was already checked when the "working set"
 *   was created in  _pipe_xfer_prepare().
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
	struct k_thread    *reader;
	struct k_pipe_desc *desc;
	sys_dlist_t    xfer_list;
	size_t         num_bytes_written = 0;
	size_t         bytes_copied;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_pipe, put, pipe, timeout);

	CHECKIF((min_xfer > bytes_to_write) || bytes_written == NULL) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe, timeout, -EINVAL);

		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&pipe->lock);

	/*
	 * Create a list of "working readers" into which the data will be
	 * directly copied.
	 */

	if (!pipe_xfer_prepare(&xfer_list, &reader, &pipe->wait_q.readers,
				pipe->size - pipe->bytes_used, bytes_to_write,
				min_xfer, timeout)) {
		k_spin_unlock(&pipe->lock, key);
		*bytes_written = 0;

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe, timeout, -EIO);

		return -EIO;
	}

	SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_pipe, put, pipe, timeout);

	z_sched_lock();
	k_spin_unlock(&pipe->lock, key);

	/*
	 * 1. 'xfer_list' currently contains a list of reader threads that can
	 * have their read requests fulfilled by the current call.
	 * 2. 'reader' if not NULL points to a thread on the reader wait_q
	 * that can get some of its requested data.
	 * 3. Interrupts are unlocked but the scheduler is locked to allow
	 * ticks to be delivered but no scheduling to occur
	 * 4. If 'reader' times out while we are copying data, not only do we
	 * still have a pointer to it, but it can not execute until this call
	 * is complete so it is still safe to copy data to it.
	 */

	struct k_thread *thread = (struct k_thread *)
				  sys_dlist_get(&xfer_list);
	while (thread != NULL) {
		desc = (struct k_pipe_desc *)thread->base.swap_data;
		bytes_copied = pipe_xfer(desc->buffer, desc->bytes_to_xfer,
					  (uint8_t *)data + num_bytes_written,
					  bytes_to_write - num_bytes_written);

		num_bytes_written   += bytes_copied;
		desc->buffer        += bytes_copied;
		desc->bytes_to_xfer -= bytes_copied;

		/* The thread's read request has been satisfied. Ready it. */
		z_ready_thread(thread);

		thread = (struct k_thread *)sys_dlist_get(&xfer_list);
	}

	/*
	 * Copy any data to the reader that we left on the wait_q.
	 * It is possible no data will be copied.
	 */
	if (reader != NULL) {
		desc = (struct k_pipe_desc *)reader->base.swap_data;
		bytes_copied = pipe_xfer(desc->buffer, desc->bytes_to_xfer,
					 (uint8_t *)data + num_bytes_written,
					  bytes_to_write - num_bytes_written);

		num_bytes_written   += bytes_copied;
		desc->buffer        += bytes_copied;
		desc->bytes_to_xfer -= bytes_copied;
	}

	/*
	 * As much data as possible has been directly copied to any waiting
	 * readers. Add as much as possible to the pipe's circular buffer.
	 */

	num_bytes_written +=
		pipe_buffer_put(pipe, (uint8_t *)data + num_bytes_written,
				 bytes_to_write - num_bytes_written);

	if (num_bytes_written == bytes_to_write) {
		*bytes_written = num_bytes_written;
		k_sched_unlock();

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe, timeout, 0);

		return 0;
	}

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT)
	    && num_bytes_written >= min_xfer
	    && min_xfer > 0U) {
		*bytes_written = num_bytes_written;
		k_sched_unlock();

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, put, pipe, timeout, 0);

		return 0;
	}

	/* Not all data was copied */

	struct k_pipe_desc  pipe_desc;

	pipe_desc.buffer         = (uint8_t *)data + num_bytes_written;
	pipe_desc.bytes_to_xfer  = bytes_to_write - num_bytes_written;

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		_current->base.swap_data = &pipe_desc;
		/*
		 * Lock interrupts and unlock the scheduler before
		 * manipulating the writers wait_q.
		 */
		k_spinlock_key_t key2 = k_spin_lock(&pipe->lock);
		z_sched_unlock_no_reschedule();
		(void)z_pend_curr(&pipe->lock, key2,
				  &pipe->wait_q.writers, timeout);
	} else {
		k_sched_unlock();
	}

	*bytes_written = bytes_to_write - pipe_desc.bytes_to_xfer;

	int ret = pipe_return_code(min_xfer, pipe_desc.bytes_to_xfer,
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
	struct k_thread    *writer;
	struct k_pipe_desc *desc;
	sys_dlist_t    xfer_list;
	size_t         num_bytes_read = 0;
	size_t         data_off;
	size_t         bytes_copied;

	/*
	 * Create a list of "working readers" into which the data will be
	 * directly copied.
	 */

	if (!pipe_xfer_prepare(&xfer_list, &writer, &pipe->wait_q.writers,
				pipe->bytes_used, bytes_to_read,
				min_xfer, timeout)) {
		k_spin_unlock(&pipe->lock, key);
		*bytes_read = 0;

		return -EIO;
	}

	z_sched_lock();
	k_spin_unlock(&pipe->lock, key);

	num_bytes_read = pipe_buffer_get(pipe, data, bytes_to_read);

	/*
	 * 1. 'xfer_list' currently contains a list of writer threads that can
	 *     have their write requests fulfilled by the current call.
	 * 2. 'writer' if not NULL points to a thread on the writer wait_q
	 *    that can post some of its requested data.
	 * 3. Data will be copied from each writer's buffer to either the
	 *    reader's buffer and/or to the pipe's circular buffer.
	 * 4. Interrupts are unlocked but the scheduler is locked to allow
	 *    ticks to be delivered but no scheduling to occur
	 * 5. If 'writer' times out while we are copying data, not only do we
	 *    still have a pointer to it, but it can not execute until this
	 *    call is complete so it is still safe to copy data from it.
	 */

	struct k_thread *thread = (struct k_thread *)
				  sys_dlist_get(&xfer_list);
	while ((thread != NULL) && (num_bytes_read < bytes_to_read)) {
		desc = (struct k_pipe_desc *)thread->base.swap_data;
		data_off = (data == NULL) ? 0 : num_bytes_read;
		bytes_copied = pipe_xfer((uint8_t *)data + data_off,
					  bytes_to_read - num_bytes_read,
					  desc->buffer, desc->bytes_to_xfer);

		num_bytes_read       += bytes_copied;
		desc->buffer         += bytes_copied;
		desc->bytes_to_xfer  -= bytes_copied;

		/*
		 * It is expected that the write request will be satisfied.
		 * However, if the read request was satisfied before the
		 * write request was satisfied, then the write request must
		 * finish later when writing to the pipe's circular buffer.
		 */
		if (num_bytes_read == bytes_to_read) {
			break;
		}
		z_ready_thread(thread);

		thread = (struct k_thread *)sys_dlist_get(&xfer_list);
	}

	if ((writer != NULL) && (num_bytes_read < bytes_to_read)) {
		desc = (struct k_pipe_desc *)writer->base.swap_data;
		data_off = (data == NULL) ? 0 : num_bytes_read;
		bytes_copied = pipe_xfer((uint8_t *)data + data_off,
					  bytes_to_read - num_bytes_read,
					  desc->buffer, desc->bytes_to_xfer);

		num_bytes_read       += bytes_copied;
		desc->buffer         += bytes_copied;
		desc->bytes_to_xfer  -= bytes_copied;
	}

	/*
	 * Copy as much data as possible from the writers (if any)
	 * into the pipe's circular buffer.
	 */

	while (thread != NULL) {
		desc = (struct k_pipe_desc *)thread->base.swap_data;
		bytes_copied = pipe_buffer_put(pipe, desc->buffer,
						desc->bytes_to_xfer);

		desc->buffer         += bytes_copied;
		desc->bytes_to_xfer  -= bytes_copied;

		/* Write request has been satisfied */
		z_ready_thread(thread);

		thread = (struct k_thread *)sys_dlist_get(&xfer_list);
	}

	if (writer != NULL) {
		desc = (struct k_pipe_desc *)writer->base.swap_data;
		bytes_copied = pipe_buffer_put(pipe, desc->buffer,
						desc->bytes_to_xfer);

		desc->buffer         += bytes_copied;
		desc->bytes_to_xfer  -= bytes_copied;
	}

	if (num_bytes_read == bytes_to_read) {
		k_sched_unlock();

		*bytes_read = num_bytes_read;

		return 0;
	}

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT)
	    && num_bytes_read >= min_xfer
	    && min_xfer > 0U) {
		k_sched_unlock();

		*bytes_read = num_bytes_read;

		return 0;
	}

	/*
	 * Not all data was read. It is important to note that when this
	 * routine is invoked by either of the flush routines() both the <data>
	 * and <timeout> parameters are set to NULL and K_NO_WAIT respectively.
	 * Consequently, neither k_pipe_flush() nor k_pipe_buffer_flush()
	 * will block.
	 *
	 * However, this routine may also be invoked by k_pipe_get() and there
	 * is no enforcement of <data> being non-NULL when called from
	 * kernel-space. That restriction is enforced when called from
	 * user-space.
	 */

	struct k_pipe_desc  pipe_desc;

	pipe_desc.buffer        = (uint8_t *)data + num_bytes_read;
	pipe_desc.bytes_to_xfer = bytes_to_read - num_bytes_read;

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_pipe, get, pipe, timeout);

		_current->base.swap_data = &pipe_desc;
		k_spinlock_key_t key2 = k_spin_lock(&pipe->lock);

		z_sched_unlock_no_reschedule();
		(void)z_pend_curr(&pipe->lock, key2,
				 &pipe->wait_q.readers, timeout);
	} else {
		k_sched_unlock();
	}

	*bytes_read = bytes_to_read - pipe_desc.bytes_to_xfer;

	int ret = pipe_return_code(min_xfer, pipe_desc.bytes_to_xfer,
				   bytes_to_read);
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_pipe, get, pipe, timeout, ret);
	return ret;
}

int z_impl_k_pipe_get(struct k_pipe *pipe, void *data, size_t bytes_to_read,
		     size_t *bytes_read, size_t min_xfer, k_timeout_t timeout)
{
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
