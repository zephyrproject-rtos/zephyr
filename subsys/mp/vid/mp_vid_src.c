/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/mp/vid/mp_vid_buffer_pool.h>
#include <zephyr/mp/vid/mp_vid_property.h>
#include <zephyr/mp/vid/mp_vid_src.h>

LOG_MODULE_REGISTER(mp_vid_src, CONFIG_MP_LOG_LEVEL);

#define DEFAULT_PROP_DEVICE DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_camera))

static struct mp_caps *mp_vid_src_supported_caps(struct mp_src *src)
{
	struct mp_vid_src *vid_src = (struct mp_vid_src *)src;

	return mp_vid_object_get_caps(&vid_src->vid_obj);
}

static void mp_vid_src_update_caps(struct mp_src *src)
{
	struct mp_caps *caps = mp_vid_src_supported_caps(src);

	mp_src_update_caps(src, caps);
	mp_caps_unref(caps);
}

static int mp_vid_src_set_caps(struct mp_src *src, struct mp_caps *caps)
{
	struct mp_vid_src *vid_src = (struct mp_vid_src *)src;

	if (mp_vid_object_set_caps(&vid_src->vid_obj, caps) < 0) {
		return -EINVAL;
	}

	/* Set pad's caps only when everything is OK */
	mp_caps_replace(&src->srcpad.caps, caps);

	return 0;
}

static int mp_vid_src_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_vid_src *vid_src = (struct mp_vid_src *)obj;
	struct mp_src *src = &vid_src->src;
	int ret = mp_vid_object_set_property(&vid_src->vid_obj, key, val);

	if (ret == 0 && (key == MP_PROP_VID_DEVICE || key == MP_PROP_VID_CROP)) {
		/* Device set, update caps */
		mp_vid_src_update_caps(src);
	}

	if (ret == -ENOTSUP) {
		return mp_src_set_property(obj, key, val);
	}

	return ret;
}

static int mp_vid_src_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_vid_src *vid_src = (struct mp_vid_src *)obj;
	int ret = mp_vid_object_get_property(&vid_src->vid_obj, key, val);

	if (ret == -ENOTSUP) {
		return mp_src_get_property(obj, key, val);
	}

	return ret;
}

static int mp_vid_src_decide_allocation(struct mp_src *self, struct mp_dispatch *query)
{
	struct mp_vid_src *vid_src = (struct mp_vid_src *)self;

	return mp_vid_object_decide_allocation(&vid_src->vid_obj, query);
}

void mp_vid_src_init(struct mp_element *self)
{
	struct mp_src *src = (struct mp_src *)self;
	struct mp_vid_src *vid_src = (struct mp_vid_src *)src;

	/* Init base class */
	mp_src_init(self);

	/* Initialize vid object */
	vid_src->vid_obj.vdev = DEFAULT_PROP_DEVICE;
	vid_src->vid_obj.type = VIDEO_BUF_TYPE_OUTPUT;

	self->object.get_property = mp_vid_src_get_property;
	self->object.set_property = mp_vid_src_set_property;

	/*
	 * pool needs to be set before retrieving supported caps as
	 * some pool's configs will be set during caps probing.
	 */
	src->pool = &vid_src->vid_obj.pool.pool;
	mp_vid_buffer_pool_init(src->pool, &vid_src->vid_obj);

	/* Retrieve supported caps */
	mp_vid_src_update_caps(src);

	src->set_caps = mp_vid_src_set_caps;
	src->decide_allocation = mp_vid_src_decide_allocation;
}
