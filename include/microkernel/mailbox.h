/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file mailbox.h
 * @brief Microkernel mailbox header file
 */


#ifndef _MAILBOX_H
#define _MAILBOX_H


/**
 * @brief Mailbox Kernel Services
 * @defgroup microkernel_mailbox Mailbox Kernel Services
 * @{
 */

/* externs */

#ifdef __cplusplus
extern "C" {
#endif

extern int _task_mbox_put(kmbox_t mbox,
		                  kpriority_t prio,
		                  struct k_msg *M,
		                  int32_t time);

extern int _task_mbox_get(kmbox_t mbox, struct k_msg *M, int32_t time);

extern void _task_mbox_block_put(kmbox_t mbox,
		                         kpriority_t prio,
		                         struct k_msg *M,
		                         ksem_t sem);

extern void _task_mbox_data_get(struct k_msg *M);

extern int _task_mbox_data_block_get(struct k_msg *M,
				 struct k_block *rxblock,
				 kmemory_pool_t pid,
				 int32_t time);

/**
 * @brief Send a message to a mailbox
 *
 * This routine sends a message to a mailbox and looks for a matching receiver.
 *
 * @param b mailbox
 * @param p priority of data transfer
 * @param m pointer to message to send
 *
 * @return RC_OK, RC_FAIL on success, failure respectively
 */
#define task_mbox_put(b, p, m) _task_mbox_put(b, p, m, TICKS_NONE)

/**
 * @brief Send a message to a mailbox and wait
 *
 * This routine sends a message to a mailbox and looks for a matching receiver.
 *
 * @param b mailbox
 * @param p priority of data transfer
 * @param m pointer to message to send
 *
 * @return RC_OK, RC_FAIL on success, failure respectively
 */
#define task_mbox_put_wait(b, p, m) _task_mbox_put(b, p, m, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 * @brief Send a message to a mailbox and wait for timeout
 *
 * This routine sends a message to a mailbox and looks for a matching receiver.
 *
 * @param b mailbox
 * @param p priority of data transfer
 * @param m pointer to message to send
 * @param t maximum number of ticks to wait
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
#define task_mbox_put_wait_timeout(b, p, m, t) _task_mbox_put(b, p, m, t)
#endif

/**
 * @brief Gets struct k_msg message header structure information from
 * a mailbox
 * @param b mailbox
 * @param m pointer to message
 *
 * @return RC_OK, RC_FAIL on success, failure respectively
 */
#define task_mbox_get(b, m) _task_mbox_get(b, m, TICKS_NONE)

/**
 * @brief Gets struct k_msg message header structure information from
 * a mailbox and wait
 * @param b mailbox
 * @param m pointer to message
 * @param time maximum number of ticks to wait
 *
 * @return RC_OK, RC_FAIL on success, failure respectively
 */
#define task_mbox_get_wait(b, m) _task_mbox_get(b, m, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 * @brief Gets struct k_msg message header structure information from
 * a mailbox and wait
 * @param b mailbox
 * @param m pointer to message
 * @param t maximum number of ticks to wait
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
#define task_mbox_get_wait_timeout(b, m, t) _task_mbox_get(b, m, t)
#endif

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
 * @param m message from which to get data
 *
 * @return N/A
 */
#define task_mbox_data_get(m) _task_mbox_data_get(m)

/**
 * @brief Get the mailbox data and place in a memory pool block
 *
 * @param m message from which to get data
 * @param b block
 * @param p pool
 * @return RC_OK upon success, RC_FAIL upon failure
 */
#define task_mbox_data_block_get(m, b, p) \
		_task_mbox_data_block_get(m, b, p, TICKS_NONE)

/**
 * @brief Get the mailbox data and place in a memory pool block and wait
 *
 * @param m message from which to get data
 * @param b block
 * @param p pool
 * @return RC_OK upon success, RC_FAIL upon failure
 */
#define task_mbox_data_block_get_wait(m, b, p) \
	_task_mbox_data_block_get(m, b, p, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS
/**
 * @brief Get the mailbox data and place in a memory pool block and wait
 *
 * @param m message from which to get data
 * @param b block
 * @param p pool
 * @param t timeout
 * @return RC_OK upon success, RC_FAIL upon failure
 */
#define task_mbox_data_block_get_wait_timeout(m, b, p, t) \
		_task_mbox_data_block_get(m, b, p, t)
#endif

/**
 * @brief Initializer for microkernel mailbox
 */
#define __K_MAILBOX_DEFAULT \
	{ \
	  .Writers = NULL, \
	  .Readers = NULL, \
	  .Count = 0, \
	}

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
