/*
 * Copyright (c) 2015 Intel Corporation
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
 * @brief Event logger support.
 */

#ifndef __EVENT_LOGGER_H__
#define __EVENT_LOGGER_H__

#define EVENT_HEADER_SIZE        1

#ifndef _ASMLANGUAGE

#include <nanokernel.h>
#include <errno.h>
#include <misc/ring_buffer.h>

struct event_logger {
	struct nano_sem sync_sema;
	struct ring_buf ring_buf;
};


/**
 * @brief Initialize the event logger.
 *
 * @details Initialize the ring buffer.
 *
 * @param logger        Logger to be initialized.
 * @param logger_buffer Pointer to the buffer to be used by the logger.
 * @param buffer_size   Size of the buffer in 32-bit words.
 *
 * @return No return value.
 */
void sys_event_logger_init(struct event_logger *logger,
	uint32_t *logger_buffer, uint32_t buffer_size);


/**
 * @brief Send an event message to the logger.
 *
 * @details Add an event message to the ring buffer and signal the sync
 * semaphore to inform that there are event messages available.
 *
 * @param logger     Pointer to the event logger used.
 * @param event_id   The identification of the profiler event.
 * @param event_data Pointer to the data of the message.
 * @param data_size  Size of the buffer in 32-bit words.
 *
 * @return No return value.
 */
void sys_event_logger_put(struct event_logger *logger, uint16_t event_id,
	uint32_t *event_data, uint8_t data_size);


/**
 * @brief Retrieve an event message from the logger.
 *
 * @details Retrieve an event message from the ring buffer and copy it to the
 * provided buffer. If the provided buffer is smaller than the message
 * size the function returns -EMSGSIZE. Otherwise return the number of 32-bit
 * words copied. The function retrieves messages in FIFO order. If there is no
 * message in the buffer the function returns immediately. It can only be called
 * from a fiber.
 *
 * @param logger       Pointer to the event logger used.
 * @param event_id     Pointer to the id of the event fetched
 * @param dropped_event_count Pointer to how many events were dropped
 * @param buffer       Pointer to the buffer where the message will be copied.
 * @param buffer_size  Size of the buffer in 32-bit words. Updated with the
 *		       actual message size.
 *
 * @return -EMSGSIZE if the buffer size is smaller than the message size, or
 * the amount of 32-bit words copied. 0 (zero) if there was no message already
 * available.
 */
int sys_event_logger_get(struct event_logger *logger, uint16_t *event_id,
			 uint8_t *dropped_event_count, uint32_t *buffer,
			 uint8_t *buffer_size);

/**
 * @brief Retrieve an event message from the logger, wait if empty.
 *
 * @details Retrieve an event message from the ring buffer and copy it to the
 * provided buffer. If the provided buffer is smaller than the message
 * size the function returns -EMSGSIZE. Otherwise return the number of 32-bit
 * words copied. The function retrieves messages in FIFO order. The caller pends
 * if there is no message available in the buffer. It can only be called from a
 * fiber.
 *
 * @param logger       Pointer to the event logger used.
 * @param event_id     Pointer to the id of the event fetched
 * @param dropped_event_count Pointer to how many events were dropped
 * @param buffer       Pointer to the buffer where the message will be copied.
 * @param buffer_size  Size of the buffer in 32-bit words. Updated with the
 *		       actual message size.
 *
 * @return -EMSGSIZE if the buffer size is smaller than the message size. Or
 * the amount of DWORDs copied.
 */
int sys_event_logger_get_wait(struct event_logger *logger,  uint16_t *event_id,
			      uint8_t *dropped_event_count, uint32_t *buffer,
			      uint8_t *buffer_size);

#ifdef CONFIG_NANO_TIMEOUTS
/**
 * @brief Retrieve an event message from the logger, wait with a timeout if
 * empty.
 *
 * @details Retrieve an event message from the ring buffer and copy it to the
 * provided buffer. If the provided buffer is smaller than the message
 * size the function returns -EMSGSIZE. Otherwise return the number of dwords
 * copied. The function retrieves messages in FIFO order. The caller pends if
 * there is no message available in the buffer until a new message is added or
 * the timeout expires. It can only be called from a fiber.
 *
 * @param logger       Pointer to the event logger used.
 * @param event_id     Pointer to the id of the event fetched
 * @param dropped_event_count Pointer to how many events were dropped
 * @param buffer       Pointer to the buffer where the message will be copied.
 * @param buffer_size  Size of the buffer in 32-bit words. Updated with the
 *		       actual message size.
 * @param timeout      Timeout in ticks.
 *
 * @return -EMSGSIZE if the buffer size is smaller than the message size. Or
 * the amount of 32-bit words copied. 0 (zero) if the timeout expired and there
 * was no message already available.
 */
int sys_event_logger_get_wait_timeout(struct event_logger *logger,
				      uint16_t *event_id,
				      uint8_t *dropped_event_count,
				      uint32_t *buffer, uint8_t *buffer_size,
				      uint32_t timeout);
#endif /* CONFIG_NANO_TIMEOUTS */

#endif /* _ASMLANGUAGE */
#endif /* __EVENT_LOGGER_H__ */
