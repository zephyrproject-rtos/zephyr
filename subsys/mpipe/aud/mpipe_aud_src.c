/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/aud/mpipe_aud_buffer_pool.h>
#include <zephyr/mpipe/aud/mpipe_aud_src.h>

LOG_MODULE_REGISTER(mpipe_aud_src, CONFIG_MPIPE_LOG_LEVEL);

static int mpipe_aud_src_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_src *src = (struct mpipe_src *)obj;
	struct mpipe_aud_buffer_pool *pool =
		CONTAINER_OF(src->pool, struct mpipe_aud_buffer_pool, pool);

	switch (key) {
	case MPIPE_PROP_AUD_SRC_SLAB_PTR:
		pool->mem_slab = (struct k_mem_slab *)val;
		break;
	case MPIPE_PROP_AUD_SRC_DEVICE:
		pool->aud_dev = (const struct device *)val;

		/* Device set, update supported caps */
		mpipe_aud_src_update_caps(src);
		break;
	default:
		return mpipe_src_set_property(obj, key, val);
	}

	return 0;
}

static int mpipe_aud_src_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_src *src = (struct mpipe_src *)obj;
	struct mpipe_aud_buffer_pool *pool =
		CONTAINER_OF(src->pool, struct mpipe_aud_buffer_pool, pool);

	if (val == NULL) {
		return -1;
	}

	switch (key) {
	case MPIPE_PROP_AUD_SRC_SLAB_PTR:
		if (pool->mem_slab != NULL) {
			*(void **)val = (void *)pool->mem_slab;
		} else {
			*(void **)val = NULL;
		}
		break;
	case MPIPE_PROP_AUD_SRC_DEVICE:
		*(const struct device **)val = pool->aud_dev;
		break;
	default:
		return mpipe_src_get_property(obj, key, val);
	}

	return 0;
}

static int mpipe_aud_src_enum_caps(struct mpipe_pad *pad, uint32_t index,
				   const struct mpipe_structure *filter,
				   struct mpipe_structure *out)
{
	struct mpipe_src *src = (struct mpipe_src *)pad->object.container;
	struct mpipe_aud_src *aud_src = (struct mpipe_aud_src *)src;
	struct mpipe_aud_buffer_pool *pool =
		CONTAINER_OF(src->pool, struct mpipe_aud_buffer_pool, pool);
	struct audio_caps src_caps;

	if (aud_src->get_audio_caps == NULL || pool->aud_dev == NULL) {
		LOG_ERR("Audio capabilities and device not configured");
		return -ENODEV;
	}

	if (aud_src->get_audio_caps(pool->aud_dev, &src_caps) != 0) {
		LOG_ERR("Failed to get audio capabilities");
		return -ENODEV;
	}

	return mpipe_aud_enum_caps(&src_caps, index, filter, out);
}

void mpipe_aud_src_update_caps(struct mpipe_src *src)
{
	__ASSERT_NO_MSG(src != NULL);

	/* The capabilities are enumerated from the device, so nothing is built here */
	src->src_pad.enum_caps_fn = mpipe_aud_src_enum_caps;
}

/*
 * Buffer count is not a media capability, so it is settled through the
 * buffer pool query instead of caps: the pool is floored at what the source
 * device needs to keep streaming and raised to what downstream must hold in
 * flight, whichever is larger.
 */
static int mpipe_aud_src_decide_buffer_pool(struct mpipe_src *src, struct mpipe_dispatch *query)
{
	struct mpipe_aud_src *aud_src = (struct mpipe_aud_src *)src;
	struct mpipe_aud_buffer_pool *pool =
		CONTAINER_OF(src->pool, struct mpipe_aud_buffer_pool, pool);
	struct mpipe_buffer_pool_config *pool_config = &src->pool->config;
	struct mpipe_buffer_pool *query_pool = query->pool;
	struct mpipe_buffer_pool_config *qpc =
		(query_pool != NULL) ? &query_pool->config : &query->pool_cfg;
	struct audio_caps src_caps;

	/*
	 * Floor the pool at the source device's own buffering requirement. Read
	 * live rather than held in req_config: the device is only known once the
	 * application has set it, which is after the pool is initialized.
	 */
	if (aud_src->get_audio_caps != NULL && pool->aud_dev != NULL &&
	    aud_src->get_audio_caps(pool->aud_dev, &src_caps) == 0) {
		pool_config->min_buffers = src_caps.min_num_buffers;
	} else {
		pool_config->min_buffers = 0;
	}

	/* Raise it to what downstream needs held in flight, if higher */
	if (qpc != NULL && qpc->min_buffers > pool_config->min_buffers) {
		pool_config->min_buffers = qpc->min_buffers;
	}

	return 0;
}

int mpipe_aud_src_init(struct mpipe_aud_src *aud_src, uint8_t id)
{
	__ASSERT_NO_MSG(aud_src != NULL);

	struct mpipe_element *self = &aud_src->src.element;
	int ret = mpipe_src_init(&aud_src->src, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "aud_src");

	self->object.get_property = mpipe_aud_src_get_property;
	self->object.set_property = mpipe_aud_src_set_property;

	aud_src->src.decide_buffer_pool = mpipe_aud_src_decide_buffer_pool;

	aud_src->get_audio_caps = NULL;

	return 0;
}
