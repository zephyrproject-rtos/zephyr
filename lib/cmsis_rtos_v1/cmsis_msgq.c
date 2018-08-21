/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <cmsis_os.h>

/**
 * @brief Create and Initialize Message queue.
 */
osMessageQId osMessageCreate(const osMessageQDef_t *queue_def,
				osThreadId thread_id)
{
	if (queue_def == NULL) {
		return NULL;
	}

	if (_is_in_isr()) {
		return NULL;
	}

	k_msgq_init(queue_def->msgq, queue_def->pool,
			queue_def->item_sz, queue_def->queue_sz);
	return (osMessageQId)(queue_def);
}

/**
 * @brief Put a message to a Queue.
 */
osStatus osMessagePut(osMessageQId queue_id, uint32_t info, uint32_t millisec)
{
	osMessageQDef_t *queue_def = (osMessageQDef_t *)queue_id;
	int retval;

	if (queue_def == NULL) {
		return osErrorParameter;
	}

	if (millisec == 0) {
		retval = k_msgq_put(queue_def->msgq, (void *)&info, K_NO_WAIT);
	} else if (millisec == osWaitForever) {
		retval = k_msgq_put(queue_def->msgq, (void *)&info, K_FOREVER);
	} else {
		retval = k_msgq_put(queue_def->msgq, (void *)&info, millisec);
	}

	if (retval == 0) {
		return osOK;
	} else if (retval == -EAGAIN) {
		return osErrorTimeoutResource;
	} else {
		return osErrorResource;
	}
}

/**
 * @brief Get a message or Wait for a Message from a Queue.
 */
osEvent osMessageGet(osMessageQId queue_id, uint32_t millisec)
{
	osMessageQDef_t *queue_def = (osMessageQDef_t *)queue_id;
	u32_t info;
	osEvent evt = {0};
	int retval;

	if (queue_def == NULL) {
		evt.status = osErrorParameter;
		return evt;
	}

	if (millisec == 0) {
		retval = k_msgq_get(queue_def->msgq, &info, K_NO_WAIT);
	} else if (millisec == osWaitForever) {
		retval = k_msgq_get(queue_def->msgq, &info, K_FOREVER);
	} else {
		retval = k_msgq_get(queue_def->msgq, &info, millisec);
	}

	if (retval == 0) {
		evt.status = osEventMessage;
		evt.value.v = info;
	} else if (retval == -EAGAIN) {
		evt.status = osEventTimeout;
	} else if (retval == -ENOMSG) {
		evt.status = osOK;
	}

	evt.def.message_id = queue_id;

	return evt;
}
