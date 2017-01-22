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

#include <kernel.h>
#include <kernel_structs.h>
#include <debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <misc/dlist.h>
#include <init.h>

struct k_pipe_desc {
	unsigned char *buffer;           /* Position in src/dest buffer */
	size_t bytes_to_xfer;            /* # bytes left to transfer */
#if (CONFIG_NUM_PIPE_ASYNC_MSGS > 0)
	struct k_mem_block *block;       /* Pointer to memory block */
	struct k_mem_block  copy_block;  /* For backwards compatibility */
	struct k_sem *sem;               /* Semaphore to give if async */
#endif
};

struct k_pipe_async {
	struct _thread_base thread;   /* Dummy thread object */
	struct k_pipe_desc  desc;     /* Pipe message descriptor */
};

extern struct k_pipe _k_pipe_list_start[];
extern struct k_pipe _k_pipe_list_end[];

struct k_pipe *_trace_list_k_pipe;

#if (CONFIG_NUM_PIPE_ASYNC_MSGS > 0)

/* Array of asynchronous message descriptors */
static struct k_pipe_async __noinit async_msg[CONFIG_NUM_PIPE_ASYNC_MSGS];

/* stack of unused asynchronous message descriptors */
K_STACK_DEFINE(pipe_async_msgs, CONFIG_NUM_PIPE_ASYNC_MSGS);

/* Allocate an asynchronous message descriptor */
static void _pipe_async_alloc(struct k_pipe_async **async)
{
	k_stack_pop(&pipe_async_msgs, (uint32_t *)async, K_FOREVER);
}

/* Free an asynchronous message descriptor */
static void _pipe_async_free(struct k_pipe_async *async)
{
	k_stack_push(&pipe_async_msgs, (uint32_t)async);
}

/* Finish an asynchronous operation */
static void _pipe_async_finish(struct k_pipe_async *async_desc)
{
	/*
	 * An asynchronous operation is finished with the scheduler locked
	 * to prevent the called routines from scheduling a new thread.
	 */

	k_mem_pool_free(async_desc->desc.block);

	if (async_desc->desc.sem != NULL) {
		k_sem_give(async_desc->desc.sem);
	}

	_pipe_async_free(async_desc);
}
#endif /* CONFIG_NUM_PIPE_ASYNC_MSGS > 0 */

#if (CONFIG_NUM_PIPE_ASYNC_MSGS > 0) || \
	defined(CONFIG_OBJECT_TRACING)

/*
 * Do run-time initialization of pipe object subsystem.
 */
static int init_pipes_module(struct device *dev)
{
	ARG_UNUSED(dev);

#if (CONFIG_NUM_PIPE_ASYNC_MSGS > 0)
	/*
	 * Create pool of asynchronous pipe message descriptors.
	 *
	 * A dummy thread requires minimal initialization, since it never gets
	 * to execute. The _THREAD_DUMMY flag is sufficient to distinguish a
	 * dummy thread from a real one. The threads are *not* added to the
	 * kernel's list of known threads.
	 *
	 * Once initialized, the address of each descriptor is added to a stack
	 * that governs access to them.
	 */

	for (int i = 0; i < CONFIG_NUM_PIPE_ASYNC_MSGS; i++) {
		async_msg[i].thread.thread_state = _THREAD_DUMMY;
		async_msg[i].thread.swap_data = &async_msg[i].desc;
		k_stack_push(&pipe_async_msgs, (uint32_t)&async_msg[i]);
	}
#endif /* CONFIG_NUM_PIPE_ASYNC_MSGS > 0 */

	/* Complete initialization of statically defined mailboxes. */

#ifdef CONFIG_OBJECT_TRACING
	struct k_pipe *pipe;

	for (pipe = _k_pipe_list_start; pipe < _k_pipe_list_end; pipe++) {
		SYS_TRACING_OBJ_INIT(k_pipe, pipe);
	}
#endif /* CONFIG_OBJECT_TRACING */

	return 0;
}

SYS_INIT(init_pipes_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_NUM_PIPE_ASYNC_MSGS or CONFIG_OBJECT_TRACING */

void k_pipe_init(struct k_pipe *pipe, unsigned char *buffer, size_t size)
{
	pipe->buffer = buffer;
	pipe->size = size;
	pipe->bytes_used = 0;
	pipe->read_index = 0;
	pipe->write_index = 0;
	sys_dlist_init(&pipe->wait_q.writers);
	sys_dlist_init(&pipe->wait_q.readers);
	SYS_TRACING_OBJ_INIT(k_pipe, pipe);
}

/**
 * @brief Copy bytes from @a src to @a dest
 *
 * @return Number of bytes copied
 */
static size_t _pipe_xfer(unsigned char *dest, size_t dest_size,
			 const unsigned char *src, size_t src_size)
{
	size_t num_bytes = min(dest_size, src_size);
	const unsigned char *end = src + num_bytes;

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
static size_t _pipe_buffer_put(struct k_pipe *pipe,
			       const unsigned char *src, size_t src_size)
{
	size_t  bytes_copied;
	size_t  run_length;
	size_t  num_bytes_written = 0;
	int     i;


	for (i = 0; i < 2; i++) {
		run_length = min(pipe->size - pipe->bytes_used,
				 pipe->size - pipe->write_index);

		bytes_copied = _pipe_xfer(pipe->buffer + pipe->write_index,
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
static size_t _pipe_buffer_get(struct k_pipe *pipe,
			       unsigned char *dest, size_t dest_size)
{
	size_t  bytes_copied;
	size_t  run_length;
	size_t  num_bytes_read = 0;
	int     i;

	for (i = 0; i < 2; i++) {
		run_length = min(pipe->bytes_used,
				 pipe->size - pipe->read_index);

		bytes_copied = _pipe_xfer(dest + num_bytes_read,
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
static bool _pipe_xfer_prepare(sys_dlist_t      *xfer_list,
			       struct k_thread **waiter,
			       _wait_q_t        *wait_q,
			       size_t            pipe_space,
			       size_t            bytes_to_xfer,
			       size_t            min_xfer,
			       int32_t           timeout)
{
	sys_dnode_t      *node;
	struct k_thread  *thread;
	struct k_pipe_desc *desc;
	size_t num_bytes = 0;

	if (timeout == K_NO_WAIT) {
		for (node = sys_dlist_peek_head(wait_q); node != NULL;
		     node = sys_dlist_peek_next(wait_q, node)) {
			thread = (struct k_thread *)node;
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

	while ((thread = (struct k_thread *) sys_dlist_peek_head(wait_q))) {
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
		_unpend_thread(thread);
		_abort_thread_timeout(thread);
		sys_dlist_append(xfer_list, &thread->base.k_q_node);
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
static int _pipe_return_code(size_t min_xfer, size_t bytes_remaining,
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
 * @brief Ready a pipe thread
 *
 * If the pipe thread is a real thread, then add it to the ready queue.
 * If it is a dummy thread, then finish the asynchronous work.
 *
 * @return N/A
 */
static void _pipe_thread_ready(struct k_thread *thread)
{
	unsigned int  key;

#if (CONFIG_NUM_PIPE_ASYNC_MSGS > 0)
	if (thread->base.thread_state & _THREAD_DUMMY) {
		_pipe_async_finish((struct k_pipe_async *)thread);
		return;
	}
#endif

	key = irq_lock();
	_ready_thread(thread);
	irq_unlock(key);
}

/**
 * @brief Internal API used to send data to a pipe
 */
int _k_pipe_put_internal(struct k_pipe *pipe, struct k_pipe_async *async_desc,
			 unsigned char *data, size_t bytes_to_write,
			 size_t *bytes_written, size_t min_xfer,
			 int32_t timeout)
{
	struct k_thread    *reader;
	struct k_pipe_desc *desc;
	sys_dlist_t    xfer_list;
	unsigned int   key;
	size_t         num_bytes_written = 0;
	size_t         bytes_copied;

#if (CONFIG_NUM_PIPE_ASYNC_MSGS == 0)
	ARG_UNUSED(async_desc);
#endif

	key = irq_lock();

	/*
	 * Create a list of "working readers" into which the data will be
	 * directly copied.
	 */

	if (!_pipe_xfer_prepare(&xfer_list, &reader, &pipe->wait_q.readers,
				pipe->size - pipe->bytes_used, bytes_to_write,
				min_xfer, timeout)) {
		irq_unlock(key);
		*bytes_written = 0;
		return -EIO;
	}

	_sched_lock();
	irq_unlock(key);

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
	while (thread) {
		desc = (struct k_pipe_desc *)thread->base.swap_data;
		bytes_copied = _pipe_xfer(desc->buffer, desc->bytes_to_xfer,
					  data + num_bytes_written,
					  bytes_to_write - num_bytes_written);

		num_bytes_written   += bytes_copied;
		desc->buffer        += bytes_copied;
		desc->bytes_to_xfer -= bytes_copied;

		/* The thread's read request has been satisfied. Ready it. */
		key = irq_lock();
		_ready_thread(thread);
		irq_unlock(key);

		thread = (struct k_thread *)sys_dlist_get(&xfer_list);
	}

	/*
	 * Copy any data to the reader that we left on the wait_q.
	 * It is possible no data will be copied.
	 */
	if (reader) {
		desc = (struct k_pipe_desc *)reader->base.swap_data;
		bytes_copied = _pipe_xfer(desc->buffer, desc->bytes_to_xfer,
					  data + num_bytes_written,
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
		_pipe_buffer_put(pipe, data + num_bytes_written,
				 bytes_to_write - num_bytes_written);

	if (num_bytes_written == bytes_to_write) {
		*bytes_written = num_bytes_written;
#if (CONFIG_NUM_PIPE_ASYNC_MSGS > 0)
		if (async_desc != NULL) {
			_pipe_async_finish(async_desc);
		}
#endif
		k_sched_unlock();
		return 0;
	}

	/* Not all data was copied. */

#if (CONFIG_NUM_PIPE_ASYNC_MSGS > 0)
	if (async_desc != NULL) {
		/*
		 * Lock interrupts and unlock the scheduler before
		 * manipulating the writers wait_q.
		 */
		key = irq_lock();
		_sched_unlock_no_reschedule();
		_pend_thread((struct k_thread *) &async_desc->thread,
			     &pipe->wait_q.writers, K_FOREVER);
		_reschedule_threads(key);
		return 0;
	}
#endif

	struct k_pipe_desc  pipe_desc;

	pipe_desc.buffer         = data + num_bytes_written;
	pipe_desc.bytes_to_xfer  = bytes_to_write - num_bytes_written;

	if (timeout != K_NO_WAIT) {
		_current->base.swap_data = &pipe_desc;
		/*
		 * Lock interrupts and unlock the scheduler before
		 * manipulating the writers wait_q.
		 */
		key = irq_lock();
		_sched_unlock_no_reschedule();
		_pend_current_thread(&pipe->wait_q.writers, timeout);
		_Swap(key);
	} else {
		k_sched_unlock();
	}

	*bytes_written = bytes_to_write - pipe_desc.bytes_to_xfer;

	return _pipe_return_code(min_xfer, pipe_desc.bytes_to_xfer,
				 bytes_to_write);
}

int k_pipe_get(struct k_pipe *pipe, void *data, size_t bytes_to_read,
	       size_t *bytes_read, size_t min_xfer, int32_t timeout)
{
	struct k_thread    *writer;
	struct k_pipe_desc *desc;
	sys_dlist_t    xfer_list;
	unsigned int   key;
	size_t         num_bytes_read = 0;
	size_t         bytes_copied;

	__ASSERT(min_xfer <= bytes_to_read, "");
	__ASSERT(bytes_read != NULL, "");

	key = irq_lock();

	/*
	 * Create a list of "working readers" into which the data will be
	 * directly copied.
	 */

	if (!_pipe_xfer_prepare(&xfer_list, &writer, &pipe->wait_q.writers,
				pipe->bytes_used, bytes_to_read,
				min_xfer, timeout)) {
		irq_unlock(key);
		*bytes_read = 0;
		return -EIO;
	}

	_sched_lock();
	irq_unlock(key);

	num_bytes_read = _pipe_buffer_get(pipe, data, bytes_to_read);

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
	while (thread && (num_bytes_read < bytes_to_read)) {
		desc = (struct k_pipe_desc *)thread->base.swap_data;
		bytes_copied = _pipe_xfer(data + num_bytes_read,
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
		_pipe_thread_ready(thread);

		thread = (struct k_thread *)sys_dlist_get(&xfer_list);
	}

	if (writer && (num_bytes_read < bytes_to_read)) {
		desc = (struct k_pipe_desc *)writer->base.swap_data;
		bytes_copied = _pipe_xfer(data + num_bytes_read,
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

	while (thread) {
		desc = (struct k_pipe_desc *)thread->base.swap_data;
		bytes_copied = _pipe_buffer_put(pipe, desc->buffer,
						desc->bytes_to_xfer);

		desc->buffer         += bytes_copied;
		desc->bytes_to_xfer  -= bytes_copied;

		/* Write request has been satsified */
		_pipe_thread_ready(thread);

		thread = (struct k_thread *)sys_dlist_get(&xfer_list);
	}

	if (writer) {
		desc = (struct k_pipe_desc *)writer->base.swap_data;
		bytes_copied = _pipe_buffer_put(pipe, desc->buffer,
						desc->bytes_to_xfer);

		desc->buffer         += bytes_copied;
		desc->bytes_to_xfer  -= bytes_copied;
	}

	if (num_bytes_read == bytes_to_read) {
		k_sched_unlock();

		*bytes_read = num_bytes_read;

		return 0;
	}

	/* Not all data was read. */

	struct k_pipe_desc  pipe_desc;

	pipe_desc.buffer        = data + num_bytes_read;
	pipe_desc.bytes_to_xfer = bytes_to_read - num_bytes_read;

	if (timeout != K_NO_WAIT) {
		_current->base.swap_data = &pipe_desc;
		key = irq_lock();
		_sched_unlock_no_reschedule();
		_pend_current_thread(&pipe->wait_q.readers, timeout);
		_Swap(key);
	} else {
		k_sched_unlock();
	}

	*bytes_read = bytes_to_read - pipe_desc.bytes_to_xfer;

	return _pipe_return_code(min_xfer, pipe_desc.bytes_to_xfer,
				 bytes_to_read);
}

int k_pipe_put(struct k_pipe *pipe, void *data, size_t bytes_to_write,
	       size_t *bytes_written, size_t min_xfer, int32_t timeout)
{
	__ASSERT(min_xfer <= bytes_to_write, "");
	__ASSERT(bytes_written != NULL, "");

	return _k_pipe_put_internal(pipe, NULL, data,
				    bytes_to_write, bytes_written,
				    min_xfer, timeout);
}

#if (CONFIG_NUM_PIPE_ASYNC_MSGS > 0)
void k_pipe_block_put(struct k_pipe *pipe, struct k_mem_block *block,
		      size_t bytes_to_write, struct k_sem *sem)
{
	struct k_pipe_async  *async_desc;
	size_t                dummy_bytes_written;

	ARG_UNUSED(bytes_to_write);

	/* For simplicity, always allocate an asynchronous descriptor */
	_pipe_async_alloc(&async_desc);

	async_desc->desc.block = &async_desc->desc.copy_block;
	async_desc->desc.copy_block = *block;
	async_desc->desc.sem = sem;
	async_desc->thread.prio = k_thread_priority_get(_current);

	(void) _k_pipe_put_internal(pipe, async_desc, block->data,
				    block->req_size, &dummy_bytes_written,
				    block->req_size, K_FOREVER);
}
#endif
