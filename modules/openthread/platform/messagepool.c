/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <openthread/platform/messagepool.h>

#define LOG_MODULE_NAME net_otPlat_messagepool
#define LOG_LEVEL       CONFIG_OPENTHREAD_LOG_LEVEL

LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define BUF_TIMEOUT K_MSEC(50)

#define MESSAGE_POOL_SIZE                                                                          \
	(CONFIG_OPENTHREAD_NUM_MESSAGE_BUFFERS * CONFIG_OPENTHREAD_MESSAGE_BUFFER_SIZE)
#define MAX_ALIGNMENT __alignof__(z_max_align_t)

BUILD_ASSERT(CONFIG_OPENTHREAD_MESSAGE_BUFFER_SIZE % MAX_ALIGNMENT == 0,
	     "Invalid message buffer size");

static struct k_mem_slab message_pool;
__aligned(MAX_ALIGNMENT) static uint8_t message_pool_buffer[MESSAGE_POOL_SIZE];

void otPlatMessagePoolInit(otInstance *aInstance, uint16_t aMinNumFreeBuffers, size_t aBufferSize)
{
	ARG_UNUSED(aInstance);

	__ASSERT(aBufferSize == CONFIG_OPENTHREAD_MESSAGE_BUFFER_SIZE,
		 "Message buffer size does not match configuration");

	if (aMinNumFreeBuffers > CONFIG_OPENTHREAD_NUM_MESSAGE_BUFFERS) {
		LOG_WRN("Minimum number of free buffers (%d) is greater than number of allocated "
			"buffers (%d)",
			aMinNumFreeBuffers, CONFIG_OPENTHREAD_NUM_MESSAGE_BUFFERS);
	}

	if (k_mem_slab_init(&message_pool, message_pool_buffer, aBufferSize,
			    CONFIG_OPENTHREAD_NUM_MESSAGE_BUFFERS) != 0) {
		__ASSERT(false, "Failed to initialize message pool");
	}
}

otMessageBuffer *otPlatMessagePoolNew(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	otMessageBuffer *buffer;

	if (k_mem_slab_alloc(&message_pool, (void **)&buffer, BUF_TIMEOUT) != 0) {
		LOG_ERR("Failed to allocate message buffer");
		return NULL;
	}

	buffer->mNext = NULL;
	return buffer;
}

void otPlatMessagePoolFree(otInstance *aInstance, otMessageBuffer *aBuffer)
{
	ARG_UNUSED(aInstance);

	k_mem_slab_free(&message_pool, (void *)aBuffer);
}
