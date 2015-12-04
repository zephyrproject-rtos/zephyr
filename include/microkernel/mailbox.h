/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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
 * @file mailbox.h
 * @brief Microkernel mailbox header file
 */


#ifndef _MAILBOX_H
#define _MAILBOX_H


/**
 * @brief Microkernel Mailboxes
 * @defgroup microkernel_mailbox Microkernel Mailboxes
 * @ingroup microkernel_services
 * @{
 */

/* externs */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond internal
 */
extern void _task_mbox_block_put(kmbox_t mbox,
					kpriority_t prio,
					struct k_msg *M,
					ksem_t sem);

extern void _task_mbox_data_get(struct k_msg *M);

/**
 * @brief Initializer for microkernel mailbox
 */
#define __K_MAILBOX_DEFAULT \
	{ \
	  .writers = NULL, \
	  .readers = NULL, \
	  .count = 0, \
	}
/**
 * @endcond
 */

/**
 * @brief Send a message to a mailbox
 *
 * This routine sends a message to a mailbox and looks for a matching receiver.
 *
 * @param mbox Mailbox.
 * @param prio Priority of data transfer.
 * @param M Pointer to message to send.
 * @param timeout Affects the action taken should there not be a waiting
 * receiver. If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then
 * wait as long as necessary. Otherwise wait up to the specified number of
 * ticks before timing out.
 *
 * @return RC_OK Successfully delivered message
 * @return RC_TIME Timed out while waiting to deliver message
 * @return RC_FAIL Failed to immediately deliver message when
 * @a timeout = TICKS_NONE
 */
extern int task_mbox_put(kmbox_t mbox, kpriority_t prio,
				struct k_msg *M, int32_t timeout);

/**
 * @brief Gets struct k_msg message header structure information from
 * a mailbox and wait with timeout.
 *
 * @param mbox Mailbox
 * @param M Pointer to message
 * @param timeout Affects the action taken should there not be a waiting
 * sender. If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then
 * wait as long as necessary. Otherwise wait up to the specified number of
 * ticks before timing out.
 *
 * @return RC_OK Successfully received message
 * @return RC_TIME Timed out while waiting to receive message
 * @return RC_FAIL Failed to immediately receive message when
 * @a timeout = TICKS_NONE
 */
extern int task_mbox_get(kmbox_t mbox, struct k_msg *M, int32_t timeout);

/**
 * @brief Send a message asynchronously to a mailbox
 *
 * This routine sends a message to a mailbox and does not wait for a matching
 * receiver. There is no exchange header returned to the sender. When the data
 * has been transferred to the receiver, the semaphore signaling is performed.
 *
 * @param b mailbox to which to send message
 * @param p priority of data transfer
 * @param m pointer to message to send
 * @param s semaphore to signal when transfer is complete
 *
 * @return N/A
 */
#define task_mbox_block_put(b, p, m, s) _task_mbox_block_put(b, p, m, s)


/**
 * @brief Get message data
 *
 * This routine is called for either of the two following purposes:
 * 1. To transfer data if the call to task_mbox_get() resulted in a non-zero size
 *    field in the struct k_msg header structure.
 * 2. To wake up and release a transmitting task that is blocked on a call to
 *    task_mbox_put[wait|wait_timeout]().
 *
 * @param m message from which to get data
 *
 * @return N/A
 */
#define task_mbox_data_get(m) _task_mbox_data_get(m)

/**
 * @brief Retrieves message data into a block, with time limited waiting
 *
 * @param M Message from which to get data
 * @param block Block
 * @param pool_id Memory pool name
 * @param timeout Affects the action taken should there not be a waiting
 * sender. If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then
 * wait as long as necessary. Otherwise wait up to the specified number of
 * ticks before timing out.
 *
 * @retval RC_OK Successful retrieval of message data
 * @retval RC_TIME Timed out while waiting to receive message data
 * @retval RC_FAIL Failed to immediately receive message data when
 * @a timeout = TICKS_NONE
 */
extern int task_mbox_data_block_get(struct k_msg *M, struct k_block *block,
					kmemory_pool_t pool_id, int32_t timeout);

/**
 * @brief Define a private microkernel mailbox
 *
 * This declares and initializes a private mailbox. The new mailbox
 * can be passed to the microkernel mailbox functions.
 *
 * @param name Name of the mailbox
 */
#define DEFINE_MAILBOX(name) \
	struct _k_mbox_struct _k_mbox_obj_##name = __K_MAILBOX_DEFAULT; \
	const kmbox_t name = (kmbox_t)&_k_mbox_obj_##name;

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* _MAILBOX_H */
