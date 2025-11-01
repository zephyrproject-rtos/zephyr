/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "mp_zvid_buffer_pool.h"
#include "mp_zvid_src.h"

LOG_MODULE_REGISTER(mp_zvid_src, CONFIG_LIBMP_LOG_LEVEL);

#define DEFAULT_PROP_DEVICE DEVICE_DT_GET(DT_CHOSEN(zephyr_camera))

static int mp_zvid_src_set_property(MpObject *obj, uint32_t key, const void *val)
{
	int ret;
	MpZvidSrc *zvid_src = MP_ZVID_SRC(obj);

	ret = mp_zvid_object_set_property(&(zvid_src->zvid_obj), key, val);
	if (ret != 0) {
		return mp_src_set_property(obj, key, val);
	}

	return 0;
}

static int mp_zvid_src_get_property(MpObject *obj, uint32_t key, void *val)
{
	int ret;
	MpZvidSrc *zvid_src = MP_ZVID_SRC(obj);

	ret = mp_zvid_object_get_property(&(zvid_src->zvid_obj), key, val);
	if (ret != 0) {
		return mp_src_get_property(obj, key, val);
	}

	return 0;
}

static MpCaps *mp_zvid_src_get_caps(MpSrc *src)
{
	MpZvidSrc *zvid_src = MP_ZVID_SRC(src);

	return mp_zvid_object_get_caps(&zvid_src->zvid_obj);
}

static bool mp_zvid_src_set_caps(MpSrc *src, MpCaps *caps)
{
	MpZvidSrc *zvid_src = MP_ZVID_SRC(src);

	if (!mp_zvid_object_set_caps(&zvid_src->zvid_obj, caps)) {
		return false;
	}

	/* Set pad's caps only when everything is OK */
	mp_caps_replace(&src->srcpad.caps, caps);

	return true;
}

static bool mp_zvid_src_decide_allocation(MpSrc *self, MpQuery *query)
{
	return mp_zvid_object_decide_allocation(&MP_ZVID_SRC(self)->zvid_obj, query);
}

void mp_zvid_src_init(MpElement *self)
{
	MpSrc *src = MP_SRC(self);
	MpZvidSrc *zvid_src = MP_ZVID_SRC(self);

	/* Init base class */
	mp_src_init(self);

	/* Initialize zvid object */
	zvid_src->zvid_obj.vdev = DEFAULT_PROP_DEVICE;
	zvid_src->zvid_obj.type = VIDEO_BUF_TYPE_OUTPUT;

	self->object.get_property = mp_zvid_src_get_property;
	self->object.set_property = mp_zvid_src_set_property;

	/*
	 * pool needs to be set before calling get_caps() as
	 * some pool's configs will be set during get_caps()
	 */
	src->pool = MP_BUFFERPOOL(&zvid_src->zvid_obj.pool);

	src->srcpad.caps = mp_zvid_src_get_caps(src);
	src->get_caps = mp_zvid_src_get_caps;
	src->set_caps = mp_zvid_src_set_caps;
	src->decide_allocation = mp_zvid_src_decide_allocation;

	mp_zvid_buffer_pool_init(src->pool, &zvid_src->zvid_obj);
}
