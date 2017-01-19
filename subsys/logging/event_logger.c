/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Event logger support.
 */

#include <logging/event_logger.h>
#include <misc/ring_buffer.h>

void sys_event_logger_init(struct event_logger *logger,
			   uint32_t *logger_buffer, uint32_t buffer_size)
{
	sys_ring_buf_init(&logger->ring_buf, buffer_size, logger_buffer);
	k_sem_init(&(logger->sync_sema), 0, UINT_MAX);
}


static void event_logger_put(struct event_logger *logger, uint16_t event_id,
			     uint32_t *event_data, uint8_t data_size,
			     void (*sem_give_fn)(struct k_sem *))
{
	int ret;
	unsigned int key;

	key = irq_lock();

	ret = sys_ring_buf_put(&logger->ring_buf, event_id,
			       logger->ring_buf.dropped_put_count, event_data,
			       data_size);
	if (ret == 0) {
		logger->ring_buf.dropped_put_count = 0;
		/* inform that there is event data available on the buffer */
		sem_give_fn(&(logger->sync_sema));
	}
	irq_unlock(key);
}


void sys_event_logger_put(struct event_logger *logger, uint16_t event_id,
			  uint32_t *event_data, uint8_t data_size)
{
	event_logger_put(logger, event_id, event_data, data_size, k_sem_give);
}


/**
 * @brief Send an event message to the logger with a non preemptible
 * behaviour.
 *
 * @details Add an event message to the ring buffer and signal the sync
 * semaphore using the internal function _sem_give_non_preemptible to inform
 * that there are event messages available, avoiding the preemptible
 * behaviour when the function is called from a task. This function
 * should be only used for special cases where the sys_event_logger_put
 * does not satisfy the needs.
 *
 * @param logger     Pointer to the event logger used.
 * @param event_id   The identification of the profiler event.
 * @param data       Pointer to the data of the message.
 * @param data_size  Size of the buffer in 32-bit words.
 *
 * @return No return value.
 */
void _sys_event_logger_put_non_preemptible(struct event_logger *logger,
					   uint16_t event_id, uint32_t *event_data, uint8_t data_size)
{
	extern void _sem_give_non_preemptible(struct k_sem *sem);

	event_logger_put(logger, event_id, event_data, data_size,
			 _sem_give_non_preemptible);
}


static int event_logger_get(struct event_logger *logger,
			    uint16_t *event_id, uint8_t *dropped_event_count,
			    uint32_t *buffer, uint8_t *buffer_size)
{
	int ret;

	ret = sys_ring_buf_get(&logger->ring_buf, event_id, dropped_event_count,
			       buffer, buffer_size);
	if (likely(!ret)) {
		return *buffer_size;
	}
	switch (ret) {
	case -EMSGSIZE:
		/* if the user can not retrieve the message, we increase the
		 *  semaphore to indicate that the message remains in the buffer
		 */
		k_sem_give(&(logger->sync_sema));
		return -EMSGSIZE;
	case -EAGAIN:
		return 0;
	default:
		return ret;
	}
}


int sys_event_logger_get(struct event_logger *logger, uint16_t *event_id,
			 uint8_t *dropped_event_count, uint32_t *buffer,
			 uint8_t *buffer_size)
{
	if (k_sem_take(&(logger->sync_sema), K_NO_WAIT)) {
		return event_logger_get(logger, event_id, dropped_event_count,
					buffer, buffer_size);
	}
	return 0;
}


int sys_event_logger_get_wait(struct event_logger *logger,  uint16_t *event_id,
			      uint8_t *dropped_event_count, uint32_t *buffer,
			      uint8_t *buffer_size)
{
	k_sem_take(&(logger->sync_sema), K_FOREVER);

	return event_logger_get(logger, event_id, dropped_event_count, buffer,
				buffer_size);
}


#ifdef CONFIG_SYS_CLOCK_EXISTS
int sys_event_logger_get_wait_timeout(struct event_logger *logger,
				      uint16_t *event_id,
				      uint8_t *dropped_event_count,
				      uint32_t *buffer, uint8_t *buffer_size,
				      uint32_t timeout)
{
	if (k_sem_take(&(logger->sync_sema), __ticks_to_ms(timeout))) {
		return event_logger_get(logger, event_id, dropped_event_count,
					buffer, buffer_size);
	}
	return 0;
}
#endif /* CONFIG_SYS_CLOCK_EXISTS */
