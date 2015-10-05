/*
 * Copyright (c) 2015 Intel Corporation
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
 * 3) Neither the name of Intel Corporation nor the names of its contributors
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

/*
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
