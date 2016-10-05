/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Message queues.
 */


#include <kernel.h>
#include <nano_private.h>
#include <misc/debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <wait_q.h>
#include <misc/dlist.h>

/**
 * @brief Initialize a message queue.
 *
 * @param q Pointer to the message queue object.
 * @param msg_size Message size, in bytes.
 * @param max_msgs Maximum number of messages that can be queued.
 * @param buffer Pointer to memory area that holds queued messages.
 *
 * @return N/A
 */
void k_msgq_init(struct k_msgq *q, uint32_t msg_size, uint32_t max_msgs,
		 char *buffer)
{
	q->msg_size = msg_size;
	q->max_msgs = max_msgs;
	q->buffer_start = buffer;
	q->buffer_end = buffer + (max_msgs * msg_size);
	q->read_ptr = buffer;
	q->write_ptr = buffer;
	q->used_msgs = 0;
	sys_dlist_init(&q->wait_q);
	SYS_TRACING_OBJ_INIT(msgq, q);
}

/**
 * @brief Adds a message to a message queue.
 *
 * @param q Pointer to the message queue object.
 * @param data Pointer to message data area.
 * @param timeout Maximum time (nanoseconds) to wait for operation to complete.
 *        Use K_NO_WAIT to return immediately, or K_FOREVER to wait as long as
 *        necessary.
 *
 * @return 0 if successful, -ENOMSG if failed immediately or after queue purge,
 *         -EAGAIN if timed out
 */
int k_msgq_put(struct k_msgq *q, void *data, int32_t timeout)
{
	unsigned int key = irq_lock();
	struct tcs *pending_thread;
	int result;

	if (q->used_msgs < q->max_msgs) {
		/* message queue isn't full */
		pending_thread = _unpend_first_thread(&q->wait_q);
		if (pending_thread) {
			/* give message to waiting thread */
			memcpy(pending_thread->swap_data, data, q->msg_size);
			/* wake up waiting thread */
			_set_thread_return_value(pending_thread, 0);
			_abort_thread_timeout(pending_thread);
			_ready_thread(pending_thread);
			if (_must_switch_threads()) {
				_Swap(key);
				return 0;
			}
		} else {
			/* put message in queue */
			memcpy(q->write_ptr, data, q->msg_size);
			q->write_ptr += q->msg_size;
			if (q->write_ptr == q->buffer_end) {
				q->write_ptr = q->buffer_start;
			}
			q->used_msgs++;
		}
		result = 0;
	} else if (timeout == K_NO_WAIT) {
		/* don't wait for message space to become available */
		result = -ENOMSG;
	} else {
		/* wait for put message success, failure, or timeout */
		_pend_current_thread(&q->wait_q, timeout);
		_current->swap_data = data;
		return _Swap(key);
	}

	irq_unlock(key);

	return result;
}

/**
 * @brief Removes a message from a message queue.
 *
 * @param q Pointer to the message queue object.
 * @param data Pointer to message data area.
 * @param timeout Maximum time (nanoseconds) to wait for operation to complete.
 *        Use K_NO_WAIT to return immediately, or K_FOREVER to wait as long as
 *        necessary.
 *
 * @return 0 if successful, -ENOMSG if failed immediately, -EAGAIN if timed out
 */
int k_msgq_get(struct k_msgq *q, void *data, int32_t timeout)
{
	unsigned int key = irq_lock();
	struct tcs *pending_thread;
	int result;

	if (q->used_msgs > 0) {
		/* take first available message from queue */
		memcpy(data, q->read_ptr, q->msg_size);
		q->read_ptr += q->msg_size;
		if (q->read_ptr == q->buffer_end) {
			q->read_ptr = q->buffer_start;
		}
		q->used_msgs--;

		/* handle first thread waiting to write (if any) */
		pending_thread = _unpend_first_thread(&q->wait_q);
		if (pending_thread) {
			/* add thread's message to queue */
			memcpy(q->write_ptr, pending_thread->swap_data,
			       q->msg_size);
			q->write_ptr += q->msg_size;
			if (q->write_ptr == q->buffer_end) {
				q->write_ptr = q->buffer_start;
			}
			q->used_msgs++;

			/* wake up waiting thread */
			_set_thread_return_value(pending_thread, 0);
			_abort_thread_timeout(pending_thread);
			_ready_thread(pending_thread);
			if (_must_switch_threads()) {
				_Swap(key);
				return 0;
			}
		}
		result = 0;
	} else if (timeout == K_NO_WAIT) {
		/* don't wait for a message to become available */
		result = -ENOMSG;
	} else {
		/* wait for get message success or timeout */
		_pend_current_thread(&q->wait_q, timeout);
		_current->swap_data = data;
		return _Swap(key);
	}

	irq_unlock(key);

	return result;
}

/**
 * @brief Purge contents of a message queue.
 *
 * Discards all messages currently in the message queue, and cancels
 * any "add message" operations initiated by waiting threads.
 *
 * @param q Pointer to the message queue object.
 *
 * @return N/A
 */
void k_msgq_purge(struct k_msgq *q)
{
	unsigned int key = irq_lock();
	struct tcs *pending_thread;

	/* wake up any threads that are waiting to write */
	while ((pending_thread = _unpend_first_thread(&q->wait_q)) != NULL) {
		_set_thread_return_value(pending_thread, -ENOMSG);
		_abort_thread_timeout(pending_thread);
		_ready_thread(pending_thread);
	}

	q->used_msgs = 0;
	q->read_ptr = q->write_ptr;

	_reschedule_threads(key);
}
