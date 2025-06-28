/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_QUEUE_H__
#define __OS_WRAPPER_QUEUE_H__

/**
 * @brief  queue handle type
 */
typedef void *rtos_queue_t;

/**
 * @brief  Creates a new queue instance
 * @note   Usage example:
 * Create:
 *          rtos_queue_t queue_handle;
 *          rtos_queue_create(&queue_handle, 5, sizeof(uint32_t));
 * Send:
 *          rtos_queue_send(queue_handle, p_msg, portMAX_DELAY);
 * Receive:
 *          rtos_queue_receive(queue_handle, p_msg, portMAX_DELAY);
 * Delete:
 *          rtos_queue_delete(queue_handle);
 * @param  pp_handle: The handle itself is a pointer, and the pp_handle means a pointer to the
 * handle.
 * @param  msg_num:  Maximum number of messages that can be queued.
 * @param  msg_size: Message size (in bytes).
 * @retval The status is SUCCESS or FAIL
 */
int rtos_queue_create(rtos_queue_t *pp_handle, uint32_t msg_num, uint32_t msg_size);

/**
 * @brief  Delete a queue - freeing all the memory allocated for storing of items placed on the
 * queue.
 * @param  p_handle: A handle to the queue to be deleted.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_queue_delete(rtos_queue_t p_handle);

/**
 * @brief  Return the number of messages stored in a queue.
 * @param  p_handle: A handle to the queue being queried.
 * @retval The number of messages available in the queue.
 */
uint32_t rtos_queue_message_waiting(rtos_queue_t p_handle);

/**
 * @brief  Send a message to a message queue in a "first in, first out" manner.
 * @param  p_handle: Address of the message queue.
 * @param  p_msg: Pointer to the message.
 * @param  wait_ms: Waiting period to add the message, 0xFFFFFFFF means Block infinitely.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_queue_send(rtos_queue_t p_handle, void *p_msg, uint32_t wait_ms);

/**
 * @brief  Send a message to the head of message queue in a "last in, first out" manner.
 * @param  p_handle: Address of the message queue.
 * @param  p_msg: Pointer to the message.
 * @param  wait_ms: Waiting period to add the message, 0xFFFFFFFF means Block infinitely.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_queue_send_to_front(rtos_queue_t p_handle, void *p_msg, uint32_t wait_ms);

/**
 * @brief  Receive a message from a message queue in a "first in, first out" manner.
 *         Messages are received from the queue and removed from the queue, so the queue's state
 * changes.
 * @param  p_handle: Address of the message queue.
 * @param  p_msg: Address of area to hold the received message.
 * @param  wait_ms: Waiting period to add the message, 0xFFFFFFFF means Block infinitely.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_queue_receive(rtos_queue_t p_handle, void *p_msg, uint32_t wait_ms);

/**
 * @brief  Peek/read a message from a message queue in a "first in, first out" manner.
 *         Simply viewing the next message in the queue does not remove it from the queue,
 *         so the state of the queue remains unchanged.
 * @param  p_handle: Address of the message queue.
 * @param  p_msg: Address of area to hold the received message.
 * @param  wait_ms: Waiting period to add the message, 0xFFFFFFFF means Block infinitely.
 * @retval The status is SUCCESS or FAIL
 */
int rtos_queue_peek(rtos_queue_t p_handle, void *p_msg, uint32_t wait_ms);

#endif
