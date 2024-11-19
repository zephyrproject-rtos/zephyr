/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Mailboxes.
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <string.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/init.h>
/* private kernel APIs */
#include <ksched.h>
#include <kthread.h>
#include <wait_q.h>

#ifdef CONFIG_OBJ_CORE_MAILBOX
static struct k_obj_type  obj_type_mailbox;
#endif /* CONFIG_OBJ_CORE_MAILBOX */

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

/*
 * Do run-time initialization of mailbox object subsystem.
 */
static int init_mbox_module(void)
{
	/* array of asynchronous message descriptors */
	static struct k_mbox_async __noinit async_msg[CONFIG_NUM_MBOX_ASYNC_MSGS];

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

	/* Complete initialization of statically defined mailboxes. */

	return 0;
}

SYS_INIT(init_mbox_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_NUM_MBOX_ASYNC_MSGS > 0 */

void k_mbox_init(struct k_mbox *mbox)
{
	z_waitq_init(&mbox->tx_msg_queue);
	z_waitq_init(&mbox->rx_msg_queue);
	mbox->lock = (struct k_spinlock) {};

#ifdef CONFIG_OBJ_CORE_MAILBOX
	k_obj_core_init_and_link(K_OBJ_CORE(mbox), &obj_type_mailbox);
#endif /* CONFIG_OBJ_CORE_MAILBOX */

	SYS_PORT_TRACING_OBJ_INIT(k_mbox, mbox);
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

		/* update syncing thread field for receiver only */
		rx_msg->_syncing_thread = tx_msg->_syncing_thread;

		return 0;
	}

	return -1;
}

/**
 * @brief Dispose of received message.
 *
 * Notifies the sender that message processing is complete.
 *
 * @param rx_msg Pointer to receive message descriptor.
 */
static void mbox_message_dispose(struct k_mbox_msg *rx_msg)
{
	struct k_thread *sending_thread;
	struct k_mbox_msg *tx_msg;

	/* do nothing if message was disposed of when it was received */
	if (rx_msg->_syncing_thread == NULL) {
		return;
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
#endif /* CONFIG_NUM_MBOX_ASYNC_MSGS */

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
	tx_msg->rx_source_thread = arch_current_thread();

	/* finish readying sending thread (actual or dummy) for send */
	sending_thread = tx_msg->_syncing_thread;
	sending_thread->base.swap_data = tx_msg;

	/* search mailbox's rx queue for a compatible receiver */
	key = k_spin_lock(&mbox->lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mbox, message_put, mbox, timeout);

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
#endif /* CONFIG_NUM_MBOX_ASYNC_MSGS */
			SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_mbox, message_put, mbox, timeout);

			/*
			 * synchronous send: pend current thread (unqueued)
			 * until the receiver consumes the message
			 */
			int ret = z_pend_curr(&mbox->lock, key, NULL, K_FOREVER);

			SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mbox, message_put, mbox, timeout, ret);

			return ret;
		}
	}

	/* didn't find a matching receiver: don't wait for one */
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mbox, message_put, mbox, timeout, -ENOMSG);

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
#endif /* CONFIG_NUM_MBOX_ASYNC_MSGS */
	SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_mbox, message_put, mbox, timeout);

	/* synchronous send: sender waits on tx queue for receiver or timeout */
	int ret = z_pend_curr(&mbox->lock, key, &mbox->tx_msg_queue, timeout);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mbox, message_put, mbox, timeout, ret);

	return ret;
}

int k_mbox_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
	       k_timeout_t timeout)
{
	/* configure things for a synchronous send, then send the message */
	tx_msg->_syncing_thread = arch_current_thread();

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mbox, put, mbox, timeout);

	int ret = mbox_message_put(mbox, tx_msg, timeout);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mbox, put, mbox, timeout, ret);

	return ret;
}

#if (CONFIG_NUM_MBOX_ASYNC_MSGS > 0)
void k_mbox_async_put(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
		      struct k_sem *sem)
{
	struct k_mbox_async *async;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mbox, async_put, mbox, sem);

	/*
	 * allocate an asynchronous message descriptor, configure both parts,
	 * then send the message asynchronously
	 */
	mbox_async_alloc(&async);

	async->thread.prio = arch_current_thread()->base.prio;

	async->tx_msg = *tx_msg;
	async->tx_msg._syncing_thread = (struct k_thread *)&async->thread;
	async->tx_msg._async_sem = sem;

	(void)mbox_message_put(mbox, &async->tx_msg, K_FOREVER);
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mbox, async_put, mbox, sem);
}
#endif /* CONFIG_NUM_MBOX_ASYNC_MSGS */

void k_mbox_data_get(struct k_mbox_msg *rx_msg, void *buffer)
{
	/* handle case where data is to be discarded */
	if (buffer == NULL) {
		rx_msg->size = 0;
		mbox_message_dispose(rx_msg);
		return;
	}

	/* copy message data to buffer, then dispose of message */
	if ((rx_msg->tx_data != NULL) && (rx_msg->size > 0U)) {
		(void)memcpy(buffer, rx_msg->tx_data, rx_msg->size);
	}
	mbox_message_dispose(rx_msg);
}

/**
 * @brief Handle immediate consumption of received mailbox message data.
 *
 * Checks to see if received message data should be kept for later retrieval,
 * or if the data should consumed immediately and the message disposed of.
 *
 * The data is consumed immediately in either of the following cases:
 *     1) The receiver requested immediate retrieval by supplying a buffer
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
	} else if (rx_msg->size == 0U) {
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
	rx_msg->tx_target_thread = arch_current_thread();

	/* search mailbox's tx queue for a compatible sender */
	key = k_spin_lock(&mbox->lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mbox, get, mbox, timeout);

	_WAIT_Q_FOR_EACH(&mbox->tx_msg_queue, sending_thread) {
		tx_msg = (struct k_mbox_msg *)sending_thread->base.swap_data;

		if (mbox_message_match(tx_msg, rx_msg) == 0) {
			/* take sender out of mailbox's tx queue */
			z_unpend_thread(sending_thread);

			k_spin_unlock(&mbox->lock, key);

			/* consume message data immediately, if needed */
			result = mbox_message_data_check(rx_msg, buffer);

			SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mbox, get, mbox, timeout, result);
			return result;
		}
	}

	/* didn't find a matching sender */

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mbox, get, mbox, timeout, -ENOMSG);

		/* don't wait for a matching sender to appear */
		k_spin_unlock(&mbox->lock, key);
		return -ENOMSG;
	}

	SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_mbox, get, mbox, timeout);

	/* wait until a matching sender appears or a timeout occurs */
	arch_current_thread()->base.swap_data = rx_msg;
	result = z_pend_curr(&mbox->lock, key, &mbox->rx_msg_queue, timeout);

	/* consume message data immediately, if needed */
	if (result == 0) {
		result = mbox_message_data_check(rx_msg, buffer);
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mbox, get, mbox, timeout, result);

	return result;
}

#ifdef CONFIG_OBJ_CORE_MAILBOX

static int init_mailbox_obj_core_list(void)
{
	/* Initialize mailbox object type */

	z_obj_type_init(&obj_type_mailbox, K_OBJ_TYPE_MBOX_ID,
			offsetof(struct k_mbox, obj_core));

	/* Initialize and link statically defined mailboxes */

	STRUCT_SECTION_FOREACH(k_mbox, mbox) {
		k_obj_core_init_and_link(K_OBJ_CORE(mbox), &obj_type_mailbox);
	}

	return 0;
}

SYS_INIT(init_mailbox_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
#endif /* CONFIG_OBJ_CORE_MAILBOX */
