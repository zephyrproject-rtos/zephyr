/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/atomic.h>

#include "mp_buffer.h"

void mp_buffer_release(struct mp_object *obj)
{
	MP_BUFFER(obj)->pool->release_buffer(MP_BUFFER(obj)->pool, MP_BUFFER(obj));
}

static bool mp_buffer_pool_configure(struct mp_buffer_pool *pool, struct mp_structure *config)
{
	return true;
}

static bool mp_buffer_pool_start(struct mp_buffer_pool *pool)
{
	return true;
}

static bool mp_buffer_pool_stop(struct mp_buffer_pool *pool)
{
	return true;
}

static bool mp_buffer_pool_acquire_buffer(struct mp_buffer_pool *pool, struct mp_buffer **buffer)
{
	return true;
}

static void mp_buffer_pool_release_buffer(struct mp_buffer_pool *pool, struct mp_buffer *buffer)
{
	/* TODO */
}

void mp_buffer_pool_init(struct mp_buffer_pool *pool)
{
	pool->configure = mp_buffer_pool_configure;
	pool->start = mp_buffer_pool_start;
	pool->stop = mp_buffer_pool_stop;
	pool->acquire_buffer = mp_buffer_pool_acquire_buffer;
	pool->release_buffer = mp_buffer_pool_release_buffer;
}
