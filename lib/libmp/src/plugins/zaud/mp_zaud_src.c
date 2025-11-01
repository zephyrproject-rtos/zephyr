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

static int mp_zaud_src_set_property(MpObject *obj, uint32_t key, const void *val)
{
	MpSrc *src = MP_SRC(obj);
	MpZaudBufferPool *pool = MP_ZAUD_BUFFER_POOL(src->pool);

	switch (key) {
	case PROP_ZAUD_SRC_SLAB_PTR:
		pool->mem_slab = (struct k_mem_slab *)val;
		break;
	default:
		return -1; /* Return error for unknown properties */
	}

	return 0;
}

static int mp_zaud_src_get_property(MpObject *obj, uint32_t key, void *val)
{
	MpSrc *src = MP_SRC(obj);
	MpZaudBufferPool *pool = MP_ZAUD_BUFFER_POOL(src->pool);

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
		return -1; /* Return error for unknown properties */
	}

	return 0;
}

static MpCaps *mp_zaud_src_get_caps(MpSrc *src)
{
	MpZaudSrc *zaud_src = MP_ZAUD_SRC(src);
	MpZaudBufferPool *pool = MP_ZAUD_BUFFER_POOL(src->pool);
	struct audio_caps src_caps;
	int i = 0;

	if (zaud_src->get_audio_caps == NULL || pool->zaud_dev == NULL) {
		LOG_ERR("Audio capabilities and device not configured");
		return NULL;
	}

	if (zaud_src->get_audio_caps(pool->zaud_dev, &src_caps) != 0) {
		LOG_ERR("Failed to get audio capabilities");
		return NULL;
	}

	MpValue *supported_sample_rate = mp_value_new(MP_TYPE_LIST, NULL);
	MpValue *supported_bit_width = mp_value_new(MP_TYPE_LIST, NULL);

	uint32_t sample_rates = src_caps.supported_sample_rates;
	uint32_t bit_widths = src_caps.supported_bit_widths;

	i = 0;
	while (sample_rates > 0) {
		if (sample_rates & 1) {
			mp_value_list_append(
				supported_sample_rate,
				mp_value_new(MP_TYPE_UINT, audio2mp_sample_rate(1 << i)));
		}
		sample_rates >>= 1;
		i++;
	}

	i = 0;
	while (bit_widths > 0) {
		if (bit_widths & 1) {
			mp_value_list_append(
				supported_bit_width,
				mp_value_new(MP_TYPE_UINT, audio2mp_bit_width(1 << i)));
		}
		bit_widths >>= 1;
		i++;
	}

	MpCaps *caps = mp_caps_new(NULL);
	MpStructure *structure = mp_structure_new(
		"audio/pcm", "samplerate", MP_TYPE_LIST, supported_sample_rate, "bitwidth",
		MP_TYPE_LIST, supported_bit_width, "numOfchannel", MP_TYPE_INT_RANGE,
		src_caps.min_total_channels, src_caps.max_total_channels, 1, "frameinterval",
		MP_TYPE_INT_RANGE, src_caps.min_frame_interval, src_caps.max_frame_interval, 1,
		"buffercount", MP_TYPE_INT_RANGE, src_caps.min_num_buffers, UINT8_MAX, 1,
		"interleaved", MP_TYPE_BOOLEAN, src_caps.interleaved, NULL);

	mp_caps_append(caps, structure);

	return caps;
}

static bool mp_zaud_src_set_caps(MpSrc *src, MpCaps *caps)
{
	return true;
}

static bool mp_zaud_src_start(MpSrc *src)
{
	return true;
}

void mp_zaud_src_init(MpElement *self)
{
	MpSrc *src = MP_SRC(self);
	MpZaudSrc *zaud_src = MP_ZAUD_SRC(self);

	/* Init base class */
	mp_src_init(MP_ELEMENT_CAST(&zaud_src->src));

	self->object.get_property = mp_zaud_src_get_property;
	self->object.set_property = mp_zaud_src_set_property;

	src->get_caps = mp_zaud_src_get_caps;
	src->set_caps = mp_zaud_src_set_caps;
	src->start = mp_zaud_src_start;

	zaud_src->get_audio_caps = NULL;
}
