/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "mp_transform_client.h"

LOG_MODULE_REGISTER(mp_transform_client, CONFIG_LIBMP_LOG_LEVEL);

static bool mp_transform_client_chainfn(struct mp_pad *pad, struct mp_buffer *in_buf,
					struct mp_buffer **out_buf)
{
	struct mp_transform *transform = MP_TRANSFORM(pad->object.container);
	struct mp_transform_client *transform_client = MP_TRANSFORM_CLIENT(transform);

	/* Support only normal mode for now */
	if (transform->mode != MP_MODE_NORMAL) {
		return false;
	}

	if (!transform->outpool->acquire_buffer(transform->outpool, out_buf)) {
		LOG_ERR("Failed to acquire an output buffer");
		return false;
	}

	if (!transform_client->chainfn_rpc((uint32_t)in_buf->data, in_buf->bytes_used,
					   (uint32_t)(*out_buf)->data, &(*out_buf)->bytes_used)) {
		LOG_ERR("Failed to process buffer via RPC");
		mp_buffer_unref(*out_buf);
		*out_buf = NULL;
		return false;
	}
	(*out_buf)->timestamp = k_uptime_get_32();

	mp_buffer_unref(in_buf);

	LOG_DBG("output buffer's timestamp: %u", (*out_buf)->timestamp);

	return true;
}

static bool mp_transform_client_propose_allocation(struct mp_transform *self,
						   struct mp_query *query)
{
	return mp_query_set_pool(query, self->inpool);
}

static bool mp_transform_client_decide_allocation(struct mp_transform *self, struct mp_query *query)
{
	struct mp_buffer_pool *query_pool = mp_query_get_pool(query);
	struct mp_buffer_pool_config *pool_config = &self->outpool->config;
	struct mp_buffer_pool_config *qpc = NULL;

	if (query_pool == NULL) {
		qpc = mp_query_get_pool_config(query);
	} else {
		qpc = &query_pool->config;
	}

	/* Always use its own pool, just negotiatie the configs */
	if (qpc != NULL) {
		/* Decide min buffers */
		if (qpc->min_buffers > pool_config->min_buffers) {
			pool_config->min_buffers = qpc->min_buffers;
		}

		/* Decide alignment */
		int align = sys_lcm(qpc->align, pool_config->align);

		if (align == -1) {
			return false;
		} else if (align == 0 && qpc->align != 0) {
			pool_config->align = qpc->align;
		} else if (align != 0) {
			pool_config->align = align;
		} else {
			return true;
		}
	}

	return true;
}

void mp_transform_client_init(struct mp_element *self)
{
	struct mp_transform *transform = MP_TRANSFORM(self);
	struct mp_transform_client *transform_client = MP_TRANSFORM_CLIENT(self);

	/*
	 * Wait a little bit here to give the opportunity to the remote core to reset
	 */
	k_msleep(300);
	transform_client->init_rpc();

	/* Init base class */
	mp_transform_init(self);

	/* Support only normal mode for now */
	transform->mode = MP_MODE_NORMAL;

	transform->sinkpad.chainfn = mp_transform_client_chainfn;
	transform->decide_allocation = mp_transform_client_decide_allocation;
	transform->propose_allocation = mp_transform_client_propose_allocation;
}
