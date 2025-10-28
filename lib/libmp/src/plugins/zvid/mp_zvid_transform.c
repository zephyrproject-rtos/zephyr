/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <src/core/mp_pixel_format.h>

#include "mp_zvid.h"
#include "mp_zvid_property.h"
#include "mp_zvid_transform.h"

LOG_MODULE_REGISTER(mp_zvid_transform, CONFIG_LIBMP_LOG_LEVEL);

static bool mp_zvid_transform_chainfn(MpPad *pad, MpBuffer *buffer)
{
	int ret;
	MpTransform *transform = MP_TRANSFORM(pad->object.container);
	MpZvidTransform *zvid_transform = MP_ZVID_TRANSFORM(transform);
	MpBufferPool *outpool = MP_BUFFERPOOL(&zvid_transform->zvid_obj_out.pool);
	MpBuffer *out_buf = NULL;

	struct video_buffer in_vbuf = {.type = VIDEO_BUF_TYPE_INPUT, .index = buffer->index};

	/* Enqueue input buffer */
	if (video_enqueue(zvid_transform->zvid_obj_in.vdev, &in_vbuf)) {
		LOG_ERR("Unable to enqueue input buffer");
		return false;
	}

	/* Start input stream */
	if (video_stream_start(zvid_transform->zvid_obj_in.vdev, VIDEO_BUF_TYPE_INPUT)) {
		LOG_ERR("Unable to start input stream");
		return false;
	}

	/* Dequeue an input buffer, blocking */
	struct video_buffer *vbuf =
		&(struct video_buffer){.type = zvid_transform->zvid_obj_in.type};
	ret = video_dequeue(zvid_transform->zvid_obj_in.vdev, &vbuf, K_FOREVER);
	if (ret) {
		LOG_ERR("Unable to dequeue input buffer");
		return false;
	}

	/* Done with the input buffer, the pool will re-enqueue it to the device they belongs to */
	mp_buffer_unref(buffer);

	/* Dequeue an output buffer, blocking */
	outpool->acquire_buffer(outpool, &out_buf);

	/* Push processed buffer to src pad */
	mp_pad_push(&transform->srcpad, out_buf);

	return true;
}

static MpCaps *mp_zvid_transform_get_caps(MpTransform *transform, MpPadDirection direction)
{
	MpZvidTransform *zvid_transform = MP_ZVID_TRANSFORM(transform);

	if (direction == MP_PAD_SINK) {
		return mp_zvid_object_get_caps(&zvid_transform->zvid_obj_in);
	} else if (direction == MP_PAD_SRC) {
		return mp_zvid_object_get_caps(&zvid_transform->zvid_obj_out);
	}

	return NULL;
}

static bool mp_zvid_transform_set_caps(MpTransform *transform, MpPadDirection direction,
				       MpCaps *caps)
{
	MpZvidTransform *zvid_transform = MP_ZVID_TRANSFORM(transform);

	if (direction == MP_PAD_SINK) {
		if (!mp_zvid_object_set_caps(&zvid_transform->zvid_obj_in, caps)) {
			return false;
		}

		/* Set pad's caps only when everything is OK */
		mp_caps_replace(&transform->sinkpad.caps, caps);
	} else if (direction == MP_PAD_SRC) {
		if (!mp_zvid_object_set_caps(&zvid_transform->zvid_obj_out, caps)) {
			return false;
		}

		/* Set pad's caps only when everything is OK */
		mp_caps_replace(&transform->srcpad.caps, caps);
	}

	return true;
}

static MpCaps *mp_zvid_transform_transform_caps(MpTransform *self, MpPadDirection direction,
						MpCaps *caps)
{
	MpZvidTransform *zvid_transform = MP_ZVID_TRANSFORM(self);
	const struct device *dev = zvid_transform->zvid_obj_in.vdev;
	MpCaps *other_caps = mp_caps_new(NULL);
	MpStructure *caps_item = NULL;
	MpCapStructure *cs;
	struct video_format_cap vfc, other_vfc;
	uint16_t ind;

	SYS_SLIST_FOR_EACH_CONTAINER(&caps->caps_structures, cs, node) {
		get_zvid_fmt_from_structure(cs->structure, &vfc);
		ind = 0;
		while (video_transform_cap(dev, &vfc, &other_vfc, direction, ind) == 0) {
			MpPixelFormat mp_fmt = zvid2mp_pixfmt(other_vfc.pixelformat);
			if (mp_fmt != MP_PIXEL_FORMAT_UNKNOWN) {
				/* TODO: Only supports video/x-raw for now */
				caps_item = mp_structure_new(
					"video/x-raw", "format", MP_TYPE_UINT, mp_fmt, "width",
					MP_TYPE_INT_RANGE, other_vfc.width_min,
					other_vfc.width_max, other_vfc.width_step, "height",
					MP_TYPE_INT_RANGE, other_vfc.height_min,
					other_vfc.height_max, other_vfc.height_step, NULL);
				/*
				 * TODO: Avoid duplicated caps items to save memory
				 */
				mp_caps_append(other_caps, caps_item);
			}
			ind++;
		}
	}

	return other_caps;
}

static int mp_zvid_transform_set_property(MpObject *obj, uint32_t key, const void *val)
{
	int ret = 0;
	MpTransform *transform = MP_TRANSFORM(obj);
	MpZvidTransform *zvid_transform = MP_ZVID_TRANSFORM(obj);

	switch (key) {
	case PROP_DEVICE:
		zvid_transform->zvid_obj_in.vdev = val;
		zvid_transform->zvid_obj_out.vdev = val;
		/* Device has been set or changed. Get caps from HW */
		MpCaps *sink_caps = mp_zvid_transform_get_caps(transform, MP_PAD_SINK);
		MpCaps *src_caps = mp_zvid_transform_get_caps(transform, MP_PAD_SRC);

		mp_caps_replace(&transform->sinkpad.caps, sink_caps);
		mp_caps_unref(sink_caps);
		mp_caps_replace(&transform->srcpad.caps, src_caps);
		mp_caps_unref(src_caps);
		break;
	default:
		ret = mp_zvid_object_set_property(&zvid_transform->zvid_obj_in, key, val);
		if (ret != 0) {
			return mp_transform_set_property(obj, key, val);
		}
	}

	return ret;
}

static int mp_zvid_transform_get_property(MpObject *obj, uint32_t key, void *val)
{
	int ret = 0;
	MpZvidTransform *self = MP_ZVID_TRANSFORM(obj);

	switch (key) {
	case PROP_DEVICE:
		*(const struct device **)val = self->zvid_obj_in.vdev;
		break;
	default:
		ret = mp_zvid_object_get_property(&self->zvid_obj_in, key, val);
		if (ret != 0) {
			return mp_transform_get_property(obj, key, val);
		}
	}

	return ret;
}

static bool mp_zvid_transform_decide_allocation(MpTransform *self, MpQuery *query)
{
	return mp_zvid_object_decide_allocation(&MP_ZVID_TRANSFORM(self)->zvid_obj_out, query);
}

static bool mp_zvid_transform_propose_allocation(MpTransform *self, MpQuery *query)
{
	return mp_query_set_pool(query, self->inpool);
}

void mp_zvid_transform_init(MpElement *self)
{
	MpTransform *transform = MP_TRANSFORM(self);
	MpZvidTransform *zvid_transform = MP_ZVID_TRANSFORM(self);

	/* Init base class */
	mp_transform_init(self);

	self->object.set_property = mp_zvid_transform_set_property;
	self->object.get_property = mp_zvid_transform_get_property;

	/*
	 * m2m devices have both input and output buffer queues,
	 * so it should be in normal mode by default
	 */
	transform->mode = MP_MODE_NORMAL;

	/*
	 * pools needs to be set before calling get_caps() as
	 * some pool's configs will be set during get_caps()
	 */
	transform->inpool = MP_BUFFERPOOL(&zvid_transform->zvid_obj_in.pool);
	transform->outpool = MP_BUFFERPOOL(&zvid_transform->zvid_obj_out.pool);

	transform->sinkpad.caps = mp_zvid_transform_get_caps(transform, MP_PAD_SINK);
	transform->srcpad.caps = mp_zvid_transform_get_caps(transform, MP_PAD_SRC);
	transform->transform_caps = mp_zvid_transform_transform_caps;
	transform->get_caps = mp_zvid_transform_get_caps;
	transform->set_caps = mp_zvid_transform_set_caps;
	transform->sinkpad.chainfn = mp_zvid_transform_chainfn;
	transform->decide_allocation = mp_zvid_transform_decide_allocation;
	transform->propose_allocation = mp_zvid_transform_propose_allocation;

	/* Initialize zvid objects */
	zvid_transform->zvid_obj_in.type = VIDEO_BUF_TYPE_INPUT;
	zvid_transform->zvid_obj_out.type = VIDEO_BUF_TYPE_OUTPUT;

	/* Initialize buffer pools */
	mp_zvid_buffer_pool_init(transform->inpool, &(zvid_transform->zvid_obj_in));
	mp_zvid_buffer_pool_init(transform->outpool, &(zvid_transform->zvid_obj_out));
}
