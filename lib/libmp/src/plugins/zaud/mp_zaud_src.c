/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/logging/log.h>

#include "mp_zaud_property.h"
#include "mp_zaud_src.h"

LOG_MODULE_REGISTER(mp_zaud_src, CONFIG_LIBMP_LOG_LEVEL);

static int mp_zaud_src_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_src *src = MP_SRC(obj);
	struct mp_zaud_buffer_pool *pool = MP_ZAUD_BUFFER_POOL(src->pool);

	switch (key) {
	case PROP_ZAUD_SRC_SLAB_PTR:
		pool->mem_slab = (struct k_mem_slab *)val;
		break;
	default:
		return mp_src_set_property(obj, key, val);
	}

	return 0;
}

static int mp_zaud_src_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_src *src = MP_SRC(obj);
	struct mp_zaud_buffer_pool *pool = MP_ZAUD_BUFFER_POOL(src->pool);

	if (val == NULL) {
		return -1;
	}

	switch (key) {
	case PROP_ZAUD_SRC_SLAB_PTR:
		if (pool->mem_slab != NULL) {
			*(void **)val = (void *)pool->mem_slab;
		} else {
			*(void **)val = NULL;
		}
		break;
	default:
		return mp_src_get_property(obj, key, val);
	}

	return 0;
}

static struct mp_caps *mp_zaud_src_get_caps(struct mp_src *src)
{
	struct mp_zaud_src *zaud_src = MP_ZAUD_SRC(src);
	struct mp_zaud_buffer_pool *pool = MP_ZAUD_BUFFER_POOL(src->pool);
	struct audio_caps src_caps;
	int i = 0;
	uint32_t sr = 0;
	uint32_t bw = 0;

	if (zaud_src->get_audio_caps == NULL || pool->zaud_dev == NULL) {
		LOG_ERR("Audio capabilities and device not configured");
		return NULL;
	}

	if (zaud_src->get_audio_caps(pool->zaud_dev, &src_caps) != 0) {
		LOG_ERR("Failed to get audio capabilities");
		return NULL;
	}

	struct mp_value *supported_sample_rate = mp_value_new(MP_TYPE_LIST, NULL);
	struct mp_value *supported_bit_width = mp_value_new(MP_TYPE_LIST, NULL);

	uint32_t sample_rates = src_caps.supported_sample_rates;
	uint32_t bit_widths = src_caps.supported_bit_widths;

	i = 0;
	while (sample_rates > 0) {
		if ((sample_rates & 0x1U) != 0U) {
			sr = audio2mp_sample_rate(1 << i);

			if (sr > 0) {
				mp_value_list_append(supported_sample_rate,
						     mp_value_new(MP_TYPE_UINT, sr));
			}
		}
		sample_rates >>= 1;
		i++;
	}

	i = 0;
	while (bit_widths > 0) {
		if ((bit_widths & 0x1U) != 0U) {
			bw = audio2mp_bit_width(1 << i);

			if (bw > 0) {
				mp_value_list_append(supported_bit_width,
						     mp_value_new(MP_TYPE_UINT, bw));
			}
		}
		bit_widths >>= 1;
		i++;
	}

	struct mp_caps *caps = mp_caps_new(MP_MEDIA_END);
	struct mp_structure *structure = mp_structure_new(
		MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_LIST, supported_sample_rate,
		MP_CAPS_BITWIDTH, MP_TYPE_LIST, supported_bit_width, MP_CAPS_NUM_OF_CHANNEL,
		MP_TYPE_INT_RANGE, src_caps.min_total_channels, src_caps.max_total_channels, 1,
		MP_CAPS_FRAME_INTERVAL, MP_TYPE_UINT_RANGE, src_caps.min_frame_interval,
		src_caps.max_frame_interval, 1, MP_CAPS_BUFFER_COUNT, MP_TYPE_INT_RANGE,
		src_caps.min_num_buffers, UINT8_MAX, 1, MP_CAPS_INTERLEAVED, MP_TYPE_BOOLEAN,
		src_caps.interleaved, MP_CAPS_END);

	mp_caps_append(caps, structure);

	return caps;
}

void mp_zaud_src_init(struct mp_element *self)
{
	struct mp_src *src = MP_SRC(self);
	struct mp_zaud_src *zaud_src = MP_ZAUD_SRC(self);

	/* Init base class */
	mp_src_init(MP_ELEMENT(&zaud_src->src));

	self->object.get_property = mp_zaud_src_get_property;
	self->object.set_property = mp_zaud_src_set_property;

	src->get_caps = mp_zaud_src_get_caps;

	zaud_src->get_audio_caps = NULL;
}
