/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/atomic.h>

#include "mp_buffer.h"

void mp_buffer_release(MpObject *obj)
{
	MP_BUFFER(obj)->pool->release_buffer(MP_BUFFER(obj)->pool, MP_BUFFER(obj));
}

static bool mp_buffer_pool_configure(MpBufferPool *pool, MpStructure *config)
{
	return true;
}

static bool mp_buffer_pool_start(MpBufferPool *pool)
{
	return true;
}

static bool mp_buffer_pool_stop(MpBufferPool *pool)
{
	return true;
}

static bool mp_buffer_pool_acquire_buffer(MpBufferPool *pool, MpBuffer **buffer)
{
	return true;
}

static void mp_buffer_pool_release_buffer(MpBufferPool *pool, MpBuffer *buffer)
{
}

void mp_buffer_pool_init(MpBufferPool *pool)
{
	pool->configure = mp_buffer_pool_configure;
	pool->start = mp_buffer_pool_start;
	pool->stop = mp_buffer_pool_stop;
	pool->acquire_buffer = mp_buffer_pool_acquire_buffer;
	pool->release_buffer = mp_buffer_pool_release_buffer;
}
