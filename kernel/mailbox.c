/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Mailboxes.
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <debug/object_tracing_common.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <string.h>
#include <ksched.h>
#include <wait_q.h>
#include <sys/dlist.h>
#include <init.h>

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)

/* asynchronous message descriptor type */
struct k_mbox_async {
	struct _thread_base thread;		/* dummy thread object */
	struct k_mbox_msg tx_msg;	/* transmit message descriptor */
};

/* stack of unused asynchronous message descriptors */
K_STACK_DEFINE(async_msg_free, CONFIG_NUM_MBOX_ASYNC_MSGS);

/* allocate an asynchronous message descriptor */
static inline void mbox_async_alloc(struct k_mbox_async **async)
{
	(void)k_stack_pop(&async_msg_free, (stack_data_t *)async, K_FOREVER);
}

/* free an asynchronous message descriptor */
static inline void mbox_async_free(struct k_mbox_async *async)
{
	k_stack_push(&async_msg_free, (stack_data_t)async);
}

#endif /* CONFIG_NUM_MBOX_ASYNC_MSGS > 0 */

#ifdef CONFIG_OBJECT_TRACING
struct k_mbox *_trace_list_k_mbox;
#endif	/* CONFIG_OBJECT_TRACING */

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0) || \
	defined(CONFIG_OBJECT_TRACING)

/*
 * Do run-time initialization of mailbox object subsystem.
 */
static int init_mbox_module(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* array of asynchronous message descriptors */
	static struct k_mbox_async __noinit async_msg[CONFIG_NUM_MBOX_ASYNC_MSGS];

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)
	/*
	 * Create pool of asynchronous message descriptors.
	 *
	 * A dummy thread requires minimal initialization, since it never gets
	 * to execute. The _THREAD_DUMMY flag is sufficient to distinguish a
	 * dummy thread from a real one. The threads are *not* added to the
	 * kernel's list of known threads.
	 *
	 * Once initialized, the address of each descriptor is added to a stack
	 * that governs access to them.
	 */

	int i;

	for (i = 0; i < CONFIG_NUM_MBOX_ASYNC_MSGS; i++) {
		z_init_thread_base(&async_msg[i].thread, 0, _THREAD_DUMMY, 0);
		k_stack_push(&async_msg_free, (stack_data_t)&async_msg[i]);
	}
#endif /* CONFIG_NUM_MBOX_ASYNC_MSGS > 0 */

	/* Complete initialization of statically defined mailboxes. */

#ifdef CONFIG_OBJECT_TRACING
	Z_STRUCT_SECTION_FOREACH(k_mbox, mbox) {
		SYS_TRACING_OBJ_INIT(k_mbox, mbox);
	}
#endif /* CONFIG_OBJECT_TRACING */

	return 0;
}

SYS_INIT(init_mbox_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_NUM_MBOX_ASYNC_MSGS or CONFIG_OBJECT_TRACING */

void k_mbox_init(struct k_mbox *mbox_ptr)
{
	z_waitq_init(&mbox_ptr->tx_msg_queue);
	z_waitq_init(&mbox_ptr->rx_msg_queue);
	mbox_ptr->lock = (struct k_spinlock) {};
	SYS_TRACING_OBJ_INIT(k_mbox, mbox_ptr);
}

/**
 * @brief Check compatibility of sender's and receiver's message descriptors.
 *
 * Compares sender's and receiver's message descriptors to see if they are
 * compatible. If so, the descriptor fields are updated to reflect that a
 * match has occurred.
 *
 * @param tx_msg Pointer to transmit message descriptor.
 * @param rx_msg Pointer to receive message descriptor.
 *
 * @return 0 if successfully matched, otherwise -1.
 */
static int mbox_message_match(struct k_mbox_msg *tx_msg,
			       struct k_mbox_msg *rx_msg)
{
	uint32_t temp_info;

	if (((tx_msg->tx_target_thread == (k_tid_t)K_ANY) ||
	     (tx_msg->tx_target_thread == rx_msg->tx_target_thread)) &&
	    ((rx_msg->rx_source_thread == (k_tid_t)K_ANY) ||
	     (rx_msg->rx_source_thread == tx_msg->rx_source_thread))) {

		/* update thread identifier fields for both descriptors */
		rx_msg->rx_source_thread = tx_msg->rx_source_thread;
		tx_msg->tx_target_thread = rx_msg->tx_target_thread;

		/* update application info fields for both descriptors */
		temp_info = rx_msg->info;
		rx_msg->info = tx_msg->info;
		tx_msg->info = temp_info;

		/* update data size field for receiver only */
		if (rx_msg->size > tx_msg->size) {
			rx_msg->size = tx_msg->size;
		}

		/* update data location fields for receiver only */
		rx_msg->tx_data = tx_msg->tx_data;
		rx_msg->tx_block = tx_msg->tx_block;
		if (rx_msg->tx_data != NULL) {
			rx_msg->tx_block.data = NULL;
		} else if (rx_msg->tx_block.data != NULL) {
			rx_msg->tx_data = rx_msg->tx_block.data;
		} else {
			/* no data */
		}

		/* update syncing thread field for receiver only */
		rx_msg->_syncing_thread = tx_msg->_syncing_thread;

		return 0;
	}

	return -1;
}

/**
 * @brief Dispose of received message.
 *
 * Releases any memory pool block still associated with the message,
 * then notifies the sender that message processing is complete.
 *
 * @param rx_msg Pointer to receive message descriptor.
 *
 * @return N/A
 */
static void mbox_message_dispose(struct k_mbox_msg *rx_msg)
{
	struct k_thread *sending_thread;
	struct k_mbox_msg *tx_msg;

	/* do nothing if message was disposed of when it was received */
	if (rx_msg->_syncing_thread == NULL) {
		return;
	}

	/* release sender's memory pool block */
	if (rx_msg->tx_block.data != NULL) {
		k_mem_pool_free(&rx_msg->tx_block);
		rx_msg->tx_block.data = NULL;
	}

	/* recover sender info */
	sending_thread = rx_msg->_syncing_thread;
	rx_msg->_syncing_thread = NULL;
	tx_msg = (struct k_mbox_msg *)sending_thread->base.swap_data;

	/* update data size field for sender */
	tx_msg->size = rx_msg->size;

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)
	/*
	 * asynchronous send: free asynchronous message descriptor +
	 * dummy thread pair, then give semaphore (if needed)
	 */
	if ((sending_thread->base.thread_state & _THREAD_DUMMY) != 0U) {
		struct k_sem *async_sem = tx_msg->_async_sem;

		mbox_async_free((struct k_mbox_async *)sending_thread);
		if (async_sem != NULL) {
			k_sem_give(async_sem);
		}
		return;
	}
#endif

	/* synchronous send: wake up sending thread */
	arch_thread_return_value_set(sending_thread, 0);
	z_mark_thread_as_not_pending(sending_thread);
	z_ready_thread(sending_thread);
	z_reschedule_unlocked();
}

/**
 * @brief Send a mailbox message.
 *
 * Helper routine that handles both synchronous and asynchronous sends.
 *
 * @param mbox Pointer to the mailbox object.
 * @param tx_msg Pointer to transmit message descriptor.
 * @param timeout Maximum time (milliseconds) to wait for the message to be
 *        received (although not necessarily completely processed).
 *        Use K_NO_WAIT to return immediately, or K_FOREVER to wait as long
 *        as necessary.
 *
 * @return 0 if successful, -ENOMSG if failed immediately, -EAGAIN if timed out
 */
static int mbox_message_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
			     k_timeout_t timeout)
{
	struct k_thread *sending_thread;
	struct k_thread *receiving_thread;
	struct k_mbox_msg *rx_msg;
	k_spinlock_key_t key;

	/* save sender id so it can be used during message matching */
	tx_msg->rx_source_thread = _current;

	/* finish readying sending thread (actual or dummy) for send */
	sending_thread = tx_msg->_syncing_thread;
	sending_thread->base.swap_data = tx_msg;

	/* search mailbox's rx queue for a compatible receiver */
	key = k_spin_lock(&mbox->lock);

	_WAIT_Q_FOR_EACH(&mbox->rx_msg_queue, receiving_thread) {
		rx_msg = (struct k_mbox_msg *)receiving_thread->base.swap_data;

		if (mbox_message_match(tx_msg, rx_msg) == 0) {
			/* take receiver out of rx queue */
			z_unpend_thread(receiving_thread);

			/* ready receiver for execution */
			arch_thread_return_value_set(receiving_thread, 0);
			z_ready_thread(receiving_thread);

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)
			/*
			 * asynchronous send: swap out current thread
			 * if receiver has priority, otherwise let it continue
			 *
			 * note: dummy sending thread sits (unqueued)
			 * until the receiver consumes the message
			 */
			if ((sending_thread->base.thread_state & _THREAD_DUMMY)
			    != 0U) {
				z_reschedule(&mbox->lock, key);
				return 0;
			}
#endif

			/*
			 * synchronous send: pend current thread (unqueued)
			 * until the receiver consumes the message
			 */
			return z_pend_curr(&mbox->lock, key, NULL, K_FOREVER);

		}
	}

	/* didn't find a matching receiver: don't wait for one */
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_spin_unlock(&mbox->lock, key);
		return -ENOMSG;
	}

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)
	/* asynchronous send: dummy thread waits on tx queue for receiver */
	if ((sending_thread->base.thread_state & _THREAD_DUMMY) != 0U) {
		z_pend_thread(sending_thread, &mbox->tx_msg_queue, K_FOREVER);
		k_spin_unlock(&mbox->lock, key);
		return 0;
	}
#endif

	/* synchronous send: sender waits on tx queue for receiver or timeout */
	return z_pend_curr(&mbox->lock, key, &mbox->tx_msg_queue, timeout);
}

int k_mbox_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
	       k_timeout_t timeout)
{
	/* configure things for a synchronous send, then send the message */
	tx_msg->_syncing_thread = _current;

	return mbox_message_put(mbox, tx_msg, timeout);
}

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)
void k_mbox_async_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
		      struct k_sem *sem)
{
	struct k_mbox_async *async;

	/*
	 * allocate an asynchronous message descriptor, configure both parts,
	 * then send the message asynchronously
	 */
	mbox_async_alloc(&async);

	async->thread.prio = _current->base.prio;

	async->tx_msg = *tx_msg;
	async->tx_msg._syncing_thread = (struct k_thread *)&async->thread;
	async->tx_msg._async_sem = sem;

	(void)mbox_message_put(mbox, &async->tx_msg, K_FOREVER);
}
#endif

void k_mbox_data_get(struct k_mbox_msg *rx_msg, void *buffer)
{
	/* handle case where data is to be discarded */
	if (buffer == NULL) {
		rx_msg->size = 0;
		mbox_message_dispose(rx_msg);
		return;
	}

	/* copy message data to buffer, then dispose of message */
	if ((rx_msg->tx_data != NULL) && (rx_msg->size > 0)) {
		(void)memcpy(buffer, rx_msg->tx_data, rx_msg->size);
	}
	mbox_message_dispose(rx_msg);
}

int k_mbox_data_block_get(struct k_mbox_msg *rx_msg, struct k_mem_pool *pool,
			  struct k_mem_block *block, k_timeout_t timeout)
{
	int result;

	/* handle case where data is to be discarded */
	if (pool == NULL) {
		rx_msg->size = 0;
		mbox_message_dispose(rx_msg);
		return 0;
	}

	/* handle case where data is already in a memory pool block */
	if (rx_msg->tx_block.data != NULL) {
		/* give ownership of the block to receiver */
		*block = rx_msg->tx_block;
		rx_msg->tx_block.data = NULL;

		/* now dispose of message */
		mbox_message_dispose(rx_msg);
		return 0;
	}

	/* allocate memory pool block (even when message size is 0!) */
	result = k_mem_pool_alloc(pool, block, rx_msg->size, timeout);
	if (result != 0) {
		return result;
	}

	/* retrieve non-block data into new block, then dispose of message */
	k_mbox_data_get(rx_msg, block->data);
	return 0;
}

/**
 * @brief Handle immediate consumption of received mailbox message data.
 *
 * Checks to see if received message data should be kept for later retrieval,
 * or if the data should consumed immediately and the message disposed of.
 *
 * The data is consumed immediately in either of the following cases:
 *     1) The receiver requested immediate retrieval by suppling a buffer
 *        to receive the data.
 *     2) There is no data to be retrieved. (i.e. Data size is 0 bytes.)
 *
 * @param rx_msg Pointer to receive message descriptor.
 * @param buffer Pointer to buffer to receive data.
 *
 * @return 0
 */
static int mbox_message_data_check(struct k_mbox_msg *rx_msg, void *buffer)
{
	if (buffer != NULL) {
		/* retrieve data now, then dispose of message */
		k_mbox_data_get(rx_msg, buffer);
	} else if (rx_msg->size == 0) {
		/* there is no data to get, so just dispose of message */
		mbox_message_dispose(rx_msg);
	} else {
		/* keep message around for later data retrieval */
	}

	return 0;
}

int k_mbox_get(struct k_mbox *mbox, struct k_mbox_msg *rx_msg, void *buffer,
	       k_timeout_t timeout)
{
	struct k_thread *sending_thread;
	struct k_mbox_msg *tx_msg;
	k_spinlock_key_t key;
	int result;

	/* save receiver id so it can be used during message matching */
	rx_msg->tx_target_thread = _current;

	/* search mailbox's tx queue for a compatible sender */
	key = k_spin_lock(&mbox->lock);

	_WAIT_Q_FOR_EACH(&mbox->tx_msg_queue, sending_thread) {
		tx_msg = (struct k_mbox_msg *)sending_thread->base.swap_data;

		if (mbox_message_match(tx_msg, rx_msg) == 0) {
			/* take sender out of mailbox's tx queue */
			z_unpend_thread(sending_thread);

			k_spin_unlock(&mbox->lock, key);

			/* consume message data immediately, if needed */
			return mbox_message_data_check(rx_msg, buffer);
		}
	}

	/* didn't find a matching sender */

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		/* don't wait for a matching sender to appear */
		k_spin_unlock(&mbox->lock, key);
		return -ENOMSG;
	}

	/* wait until a matching sender appears or a timeout occurs */
	_current->base.swap_data = rx_msg;
	result = z_pend_curr(&mbox->lock, key, &mbox->rx_msg_queue, timeout);

	/* consume message data immediately, if needed */
	if (result == 0) {
		result = mbox_message_data_check(rx_msg, buffer);
	}

	return result;
}
