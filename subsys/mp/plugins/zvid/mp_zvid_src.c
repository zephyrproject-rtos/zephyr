/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/mp/zvid/mp_zvid_buffer_pool.h>
#include <zephyr/mp/zvid/mp_zvid_property.h>
#include <zephyr/mp/zvid/mp_zvid_src.h>

LOG_MODULE_REGISTER(mp_zvid_src, CONFIG_MP_LOG_LEVEL);

#define DEFAULT_PROP_DEVICE DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_camera))

static struct mp_caps *mp_zvid_src_supported_caps(struct mp_src *src)
{
	struct mp_zvid_src *zvid_src = (struct mp_zvid_src *)src;

	return mp_zvid_object_get_caps(&zvid_src->zvid_obj);
}

static void mp_zvid_src_update_caps(struct mp_src *src)
{
	struct mp_caps *caps = mp_zvid_src_supported_caps(src);

	mp_src_update_caps(src, caps);
	mp_caps_unref(caps);
}

static int mp_zvid_src_set_caps(struct mp_src *src, struct mp_caps *caps)
{
	struct mp_zvid_src *zvid_src = (struct mp_zvid_src *)src;

	if (mp_zvid_object_set_caps(&zvid_src->zvid_obj, caps) < 0) {
		return -EINVAL;
	}

	/* Set pad's caps only when everything is OK */
	mp_caps_replace(&src->srcpad.caps, caps);

	return 0;
}

static int mp_zvid_src_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_zvid_src *zvid_src = (struct mp_zvid_src *)obj;
	struct mp_src *src = &zvid_src->src;
	int ret = mp_zvid_object_set_property(&zvid_src->zvid_obj, key, val);

	if (ret == 0 && (key == PROP_ZVID_DEVICE || key == PROP_ZVID_CROP)) {
		/* Device set, update caps */
		mp_zvid_src_update_caps(src);
	}

	if (ret == -ENOTSUP) {
		return mp_src_set_property(obj, key, val);
	}

	return ret;
}

static int mp_zvid_src_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_zvid_src *zvid_src = (struct mp_zvid_src *)obj;
	int ret = mp_zvid_object_get_property(&zvid_src->zvid_obj, key, val);

	if (ret == -ENOTSUP) {
		return mp_src_get_property(obj, key, val);
	}

	return ret;
}

static int mp_zvid_src_decide_allocation(struct mp_src *self, struct mp_dispatch *query)
{
	struct mp_zvid_src *zvid_src = (struct mp_zvid_src *)self;

	return mp_zvid_object_decide_allocation(&zvid_src->zvid_obj, query);
}

void mp_zvid_src_init(struct mp_element *self)
{
	struct mp_src *src = (struct mp_src *)self;
	struct mp_zvid_src *zvid_src = (struct mp_zvid_src *)src;

	/* Init base class */
	mp_src_init(self);

	/* Initialize zvid object */
	zvid_src->zvid_obj.vdev = DEFAULT_PROP_DEVICE;
	zvid_src->zvid_obj.type = VIDEO_BUF_TYPE_OUTPUT;

	self->object.get_property = mp_zvid_src_get_property;
	self->object.set_property = mp_zvid_src_set_property;

	/*
	 * pool needs to be set before retrieving supported caps as
	 * some pool's configs will be set during caps probing.
	 */
	src->pool = &zvid_src->zvid_obj.pool.pool;
	mp_zvid_buffer_pool_init(src->pool, &zvid_src->zvid_obj);

	/* Retrieve supported caps */
	mp_zvid_src_update_caps(src);

	src->set_caps = mp_zvid_src_set_caps;
	src->decide_allocation = mp_zvid_src_decide_allocation;
}
