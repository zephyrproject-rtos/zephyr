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

#include <misc/event_logger.h>


void sys_event_logger_init(struct event_logger *logger,
	uint32_t *logger_buffer, uint32_t buffer_size)
{
	logger->head = logger->tail = 0;
	logger->dropped_event_count = 0;
	logger->buffer = logger_buffer;
	logger->buffer_size = buffer_size;
	nano_sem_init(&(logger->sync_sema));
}


static void event_logger_put(struct event_logger *logger, uint16_t event_id,
	uint32_t *event_data, uint8_t data_size,
	void(*sem_give_fn)(struct nano_sem *))
{
	unsigned int key;
	unsigned int buffer_capacity_used;
	union event_header header;
	int i;

	/* Lock interrupt to be sure this function will be atomic */
	key = irq_lock();

	buffer_capacity_used = (logger->head - logger->tail +
		logger->buffer_size) % logger->buffer_size;

	/* check if there is space to the new event */
	if (buffer_capacity_used + EVENT_HEADER_SIZE + data_size >=
		logger->buffer_size) {
		(logger->dropped_event_count)++;
	} else {
		/* build the header */
		header.bits.data_length = data_size;
		header.bits.event_id = event_id;
		header.bits.dropped_count = logger->dropped_event_count;
		logger->dropped_event_count = 0;

		/* copy the header to the buffer */
		logger->buffer[logger->head] = header.block;
		logger->head = (logger->head + EVENT_HEADER_SIZE) %
			logger->buffer_size;

		/* copy the extra data to the buffer */
		for (i=0; i < header.bits.data_length; ++i) {
			logger->buffer[(logger->head + i) %
				logger->buffer_size] = event_data[i];
		}
		logger->head = (logger->head + header.bits.data_length) %
			logger->buffer_size;

		/* inform that there is event data available on the buffer */
		sem_give_fn(&(logger->sync_sema));
	}

	irq_unlock(key);
}


void sys_event_logger_put(struct event_logger *logger, uint16_t event_id,
	uint32_t *event_data, uint8_t data_size)
{
	event_logger_put(logger, event_id, event_data, data_size, nano_sem_give);
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
	extern void _sem_give_non_preemptible(struct nano_sem *sem);

	event_logger_put(logger, event_id, event_data, data_size,
		_sem_give_non_preemptible);
}


static inline int event_logger_get(struct event_logger *logger,
	uint32_t *buffer, uint8_t buffer_size)
{
	int i;
	union event_header *header;

	/* obtain the header */
	header = (union event_header *) &(logger->buffer[logger->tail]);
	if (buffer_size < EVENT_HEADER_SIZE + header->bits.data_length) {
		/* if the user can not retrieve the message, we increase the semaphore
		 * to indicate that the message remains in the buffer */
		nano_fiber_sem_give(&(logger->sync_sema));
		return -EMSGSIZE;
	}

	for (i=0; i < header->bits.data_length + EVENT_HEADER_SIZE; i++) {
		buffer[i] = logger->buffer[(logger->tail + i) %
			logger->buffer_size];
	}
	logger->tail = (logger->tail + header->bits.data_length +
		EVENT_HEADER_SIZE) % logger->buffer_size;

	return header->bits.data_length + EVENT_HEADER_SIZE;
}


int sys_event_logger_get(struct event_logger *logger, uint32_t *buffer,
	uint8_t buffer_size)
{
	if (nano_fiber_sem_take(&(logger->sync_sema))) {
		return sys_event_logger_get(logger, buffer, buffer_size);
	}
	return 0;
}


int sys_event_logger_get_wait(struct event_logger *logger, uint32_t *buffer,
	uint8_t buffer_size)
{
	nano_fiber_sem_take_wait(&(logger->sync_sema));

	return event_logger_get(logger, buffer, buffer_size);
}


#ifdef CONFIG_NANO_TIMEOUTS
int sys_event_logger_get_wait_timeout(struct event_logger *logger,
	uint32_t *buffer, uint8_t buffer_size, uint32_t timeout)
{
	if (nano_fiber_sem_take_wait_timeout(&(logger->sync_sema), timeout)) {
		return event_logger_get(logger, buffer, buffer_size);
	}
	return 0;
}
#endif /* CONFIG_NANO_TIMEOUTS */
