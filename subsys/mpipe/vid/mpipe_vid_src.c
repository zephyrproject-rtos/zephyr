/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/video/controls.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/mpipe/vid/mpipe_vid_buffer_pool.h>
#include <zephyr/mpipe/vid/mpipe_vid_property.h>
#include <zephyr/mpipe/vid/mpipe_vid_src.h>

LOG_MODULE_REGISTER(mpipe_vid_src, CONFIG_MPIPE_LOG_LEVEL);

#define DEFAULT_PROP_DEVICE DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_camera))

static int mpipe_vid_src_enum_caps(struct mpipe_pad *pad, uint32_t index,
				   const struct mpipe_structure *filter,
				   struct mpipe_structure *out)
{
	struct mpipe_vid_src *vid_src = (struct mpipe_vid_src *)pad->object.container;

	return mpipe_vid_object_enum_caps(&vid_src->vid_obj, index, filter, out);
}

static int mpipe_vid_src_set_caps(struct mpipe_src *src, const struct mpipe_structure *caps)
{
	struct mpipe_vid_src *vid_src = (struct mpipe_vid_src *)src;

	if (mpipe_vid_object_set_caps(&vid_src->vid_obj, caps) < 0) {
		return -EINVAL;
	}

	/* Set pad's caps only when everything is OK */
	return mpipe_pad_set_caps(&src->src_pad, caps);
}

static int mpipe_vid_src_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_vid_src *vid_src = (struct mpipe_vid_src *)obj;
	int ret = mpipe_vid_object_set_property(&vid_src->vid_obj, key, val);

	if (ret == -ENOTSUP) {
		return mpipe_src_set_property(obj, key, val);
	}

	return ret;
}

static int mpipe_vid_src_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_vid_src *vid_src = (struct mpipe_vid_src *)obj;
	int ret = mpipe_vid_object_get_property(&vid_src->vid_obj, key, val);

	if (ret == -ENOTSUP) {
		return mpipe_src_get_property(obj, key, val);
	}

	return ret;
}

static int mpipe_vid_src_decide_buffer_pool(struct mpipe_src *self, struct mpipe_dispatch *query)
{
	struct mpipe_vid_src *vid_src = (struct mpipe_vid_src *)self;

	return mpipe_vid_object_decide_buffer_pool(&vid_src->vid_obj, query);
}

int mpipe_vid_src_init(struct mpipe_vid_src *vid_src, uint8_t id)
{
	__ASSERT_NO_MSG(vid_src != NULL);

	struct mpipe_element *self = &vid_src->src.element;
	struct mpipe_src *src = &vid_src->src;
	int ret = mpipe_src_init(src, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "vid_src");

	/* Initialize vid object */
	vid_src->vid_obj.vdev = DEFAULT_PROP_DEVICE;
	vid_src->vid_obj.type = VIDEO_BUF_TYPE_OUTPUT;

	self->object.get_property = mpipe_vid_src_get_property;
	self->object.set_property = mpipe_vid_src_set_property;

	/*
	 * The pool has to exist before the first capability is enumerated, as
	 * probing the device fills in the pool's buffer count and alignment.
	 */
	src->pool = &vid_src->vid_obj.pool.pool;
	mpipe_vid_buffer_pool_init(src->pool, &vid_src->vid_obj);

	src->src_pad.enum_caps_fn = mpipe_vid_src_enum_caps;
	src->set_caps = mpipe_vid_src_set_caps;
	src->decide_buffer_pool = mpipe_vid_src_decide_buffer_pool;

	return 0;
}
