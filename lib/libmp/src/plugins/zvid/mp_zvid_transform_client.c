/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "mp_zvid_buffer_pool_client.h"
#include "mp_zvid_object.h"
#include "mp_zvid_transform_client.h"

LOG_MODULE_REGISTER(mp_zvid_transform_client, CONFIG_LIBMP_LOG_LEVEL);

static struct mp_caps *mp_zvid_transform_client_get_caps(struct mp_transform *transform,
							 enum mp_pad_direction direction)
{
	struct mp_zvid_transform_client *vtc = MP_ZVID_TRANSFORM_CLIENT(transform);
	struct mp_caps *caps = mp_caps_new(MP_MEDIA_END);
	struct mp_structure *caps_item = NULL;
	struct mp_buffer_pool *pool = NULL;
	struct video_format_cap vfc;
	uint8_t ind = 0;

	if (direction == MP_PAD_SINK) {
		pool = transform->inpool;
	} else if (direction == MP_PAD_SRC) {
		pool = transform->outpool;
	} else {
		LOG_ERR("Pad direction is invalid");
		return NULL;
	}

	/* Get buffer pool caps */
	if (vtc->get_buf_caps_rpc(direction, &pool->config.min_buffers, &pool->config.align) != 0) {
		LOG_ERR("Unable to retrieve buffer pool capabilities");
		return NULL;
	}

	/* Get format caps */
	while (vtc->get_format_caps_rpc(direction, ind++, &vfc) == 0) {
		caps_item = mp_structure_new(
			MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT, vfc.pixelformat,
			MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT_RANGE, vfc.width_min, vfc.width_max,
			vfc.width_step, MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT_RANGE, vfc.height_min,
			vfc.height_max, vfc.height_step, MP_CAPS_END);
		mp_caps_append(caps, caps_item);
	};

	return caps;
}

static bool mp_zvid_transform_client_set_caps(struct mp_transform *transform,
					    enum mp_pad_direction direction, struct mp_caps *caps)
{
	struct mp_zvid_transform_client *vtc = MP_ZVID_TRANSFORM_CLIENT(transform);
	struct mp_buffer_pool *pool = NULL;
	struct video_format_cap vfc = {0};
	struct video_format fmt;
	struct mp_structure *first_structure;

	if (!mp_caps_is_fixed(caps)) {
		return false;
	}

	if (direction == MP_PAD_SINK) {
		fmt.type = VIDEO_BUF_TYPE_INPUT;
		pool = transform->inpool;
	} else if (direction == MP_PAD_SRC) {
		fmt.type = VIDEO_BUF_TYPE_OUTPUT;
		pool = transform->outpool;
	} else {
		LOG_ERR("Pad direction is invalid");
		return NULL;
	}

	first_structure = mp_caps_get_structure(caps, 0);

	if (mp_structure_to_vfc(first_structure, &vfc) < 0) {
		return false;
	}

	fmt.pixelformat = vfc.pixelformat;
	fmt.width = vfc.width_min;
	fmt.height = vfc.height_min;

	if (vtc->set_format_rpc(&fmt) < 0) {
		LOG_ERR("Unable to set format: type=%u pixfmt=%u w=%u h=%u", fmt.type,
			fmt.pixelformat, fmt.width, fmt.height);
		return false;
	}

	/* Set buffer pool size */
	pool->config.size = fmt.size;

	/* Set pad's caps only when everything is OK */
	mp_caps_replace(
		direction == MP_PAD_SRC ? &transform->srcpad.caps : &transform->sinkpad.caps, caps);

	return true;
}

static struct mp_caps *mp_zvid_transform_client_transform_caps(struct mp_transform *self,
							     enum mp_pad_direction direction,
							     struct mp_caps *caps)
{
	struct mp_zvid_transform_client *vtc = MP_ZVID_TRANSFORM_CLIENT(self);
	struct mp_caps *other_caps = mp_caps_new(MP_MEDIA_END);
	struct mp_structure *caps_item = NULL;
	struct mp_cap_structure *cs;
	struct video_format_cap vfc, other_vfc;
	uint16_t ind;

	if ((direction != MP_PAD_SINK && direction != MP_PAD_SRC) || caps == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&caps->caps_structures, cs, node) {
		if (mp_structure_to_vfc(cs->structure, &vfc) < 0) {
			continue;
		}
		ind = 0;
		while (vtc->transform_cap_rpc(direction, ind++, &vfc, &other_vfc) == 0) {
			caps_item = mp_structure_new(
				MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT,
				other_vfc.pixelformat, MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT_RANGE,
				other_vfc.width_min, other_vfc.width_max, other_vfc.width_step,
				MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT_RANGE, other_vfc.height_min,
				other_vfc.height_max, other_vfc.height_step, MP_CAPS_END);
			/*
			 * TODO: Avoid duplicated caps items to save memory
			 */
			mp_caps_append(other_caps, caps_item);
		}
	}

	return other_caps;
}

void mp_zvid_transform_client_init(struct mp_element *self)
{
	struct mp_transform *transform = MP_TRANSFORM(self);
	struct mp_zvid_transform_client *vtc = MP_ZVID_TRANSFORM_CLIENT(self);

	/* Init base class */
	mp_transform_client_init(self);

	/*
	 * pools needs to be set before calling get_caps() as
	 * some pool's configs will be set during get_caps()
	 */
	transform->inpool = MP_BUFFER_POOL(&vtc->inpool);
	transform->outpool = MP_BUFFER_POOL(&vtc->outpool);

	transform->sinkpad.caps = mp_zvid_transform_client_get_caps(transform, MP_PAD_SINK);
	transform->srcpad.caps = mp_zvid_transform_client_get_caps(transform, MP_PAD_SRC);
	transform->get_caps = mp_zvid_transform_client_get_caps;
	transform->set_caps = mp_zvid_transform_client_set_caps;
	transform->transform_caps = mp_zvid_transform_client_transform_caps;
	/* Initialize buffer pools */
	mp_zvid_buffer_pool_client_init(transform->inpool);
	mp_zvid_buffer_pool_client_init(transform->outpool);
}
