/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Event logger support.
 */

#ifndef __EVENT_LOGGER_H__
#define __EVENT_LOGGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_HEADER_SIZE        1

#ifndef _ASMLANGUAGE

#include <kernel.h>
#include <errno.h>
#include <ring_buffer.h>

struct event_logger {
	struct k_sem sync_sema;
	struct ring_buf ring_buf;
};

/**
 * @brief Event Logger
 * @defgroup event_logger Event Logger
 * @{
 */

/**
 * @brief Initialize the event logger.
 *
 * @details This routine initializes the ring buffer.
 *
 * @param logger        Logger to be initialized.
 * @param logger_buffer Pointer to the buffer to be used by the logger.
 * @param buffer_size   Size of the buffer in 32-bit words.
 *
 * @return N/A
 */
void sys_event_logger_init(struct event_logger *logger,
			   u32_t *logger_buffer, u32_t buffer_size);


/**
 * @brief Send an event message to the logger.
 *
 * @details This routine adds an event message to the ring buffer and signals
 * the sync semaphore to indicate that event messages are available.
 *
 * @param logger     Pointer to the event logger used.
 * @param event_id   The profiler event's ID.
 * @param event_data Pointer to the data of the message.
 * @param data_size  Size of the buffer in 32-bit words.
 *
 * @return N/A
 */
void sys_event_logger_put(struct event_logger *logger, u16_t event_id,
			  u32_t *event_data, u8_t data_size);


/**
 * @brief Retrieve an event message from the logger.
 *
 * @details This routine retrieves an event message from the ring buffer and
 * copies it to the provided buffer. If the provided buffer is smaller
 * than the message size the function returns -EMSGSIZE. Otherwise, it returns
 * the number of 32-bit words copied. The function retrieves messages in
 * FIFO order. If there is no message in the buffer the function returns
 * immediately. It can only be called from a thread.
 *
 * @param logger       Pointer to the event logger used.
 * @param event_id     Pointer to the id of the fetched event.
 * @param dropped_event_count Pointer to the number of events dropped.
 * @param buffer       Pointer to the buffer for the copied message.
 * @param buffer_size  Size of the buffer in 32-bit words. Updated with the
 *                     actual message's size.
 *
 * @retval EMSGSIZE If the buffer size is smaller than the message size.
 * @retval Number of 32-bit words copied.
 * @retval 0 If no message was already available.
 */
int sys_event_logger_get(struct event_logger *logger, u16_t *event_id,
			 u8_t *dropped_event_count, u32_t *buffer,
			 u8_t *buffer_size);

/**
 * @brief Retrieve an event message from the logger, wait if empty.
 *
 * @details This routine retrieves an event message from the ring buffer
 * and copies it to the provided buffer. If the provided buffer is smaller
 * than the message size the function returns -EMSGSIZE. Otherwise, it
 * returns the number of 32-bit words copied.
 *
 * The function retrieves messages in FIFO order. The caller pends if there is
 * no message available in the buffer. It can only be called from a thread.
 *
 * @param logger       Pointer to the event logger used.
 * @param event_id     Pointer to the ID of the fetched event.
 * @param dropped_event_count Pointer to the number of dropped events.
 * @param buffer       Pointer to the buffer for the copied messages.
 * @param buffer_size  Size of the buffer in 32-bit words. Updated with the
 *                     actual message's size.
 *
 * @retval EMSGSIZE If the buffer size is smaller than the message size.
 * @retval Number of DWORDs copied, otherwise.
 */
int sys_event_logger_get_wait(struct event_logger *logger,  u16_t *event_id,
			      u8_t *dropped_event_count, u32_t *buffer,
			      u8_t *buffer_size);

#ifdef CONFIG_SYS_CLOCK_EXISTS
/**
 * @brief Retrieve an event message from the logger, wait with a timeout if
 * empty.
 *
 * @details This routine retrieves an event message from the ring buffer and
 * copies it to the provided buffer. If the provided buffer is smaller than
 * the message size the routine returns -EMSGSIZE. Otherwise, it returns the
 * number of dwords copied. The function retrieves messages in FIFO order.
 * If no message is available in the buffer, the caller pends until a
 * new message is added or the timeout expires. This routine can only be
 * called from a thread.
 *
 * @param logger       Pointer to the event logger used.
 * @param event_id     Pointer to the ID of the event fetched.
 * @param dropped_event_count Pointer to the number of dropped events.
 * @param buffer       Pointer to the buffer for the copied message.
 * @param buffer_size  Size of the buffer in 32-bit words. Updated with the
 *                     actual message size.
 * @param timeout      Timeout in ticks.
 *
 * @retval EMSGSIZE if the buffer size is smaller than the message size.
 * @retval Number of 32-bit words copied.
 * @retval 0 If the timeout expired and there was no message already
 *           available.
 */
int sys_event_logger_get_wait_timeout(struct event_logger *logger,
				      u16_t *event_id,
				      u8_t *dropped_event_count,
				      u32_t *buffer, u8_t *buffer_size,
				      u32_t timeout);
/**
 * @}
 */

#endif  /* CONFIG_SYS_CLOCK_EXISTS */

#endif  /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __EVENT_LOGGER_H__ */
