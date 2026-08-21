/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>

#include <zephyr/mp/zvid/mp_zvid_property.h>
#include <zephyr/mp/zvid/mp_zvid_transform.h>

LOG_MODULE_REGISTER(mp_zvid_transform, CONFIG_MP_LOG_LEVEL);

#define DEFAULT_PROP_DEVICE DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_videotrans))

static int mp_zvid_transform_chainfn(struct mp_pad *pad, struct net_buf *in_buf,
				     struct net_buf **out_buf)
{
	int ret;
	struct mp_transform *transform =
		CONTAINER_OF(pad->object.container, struct mp_transform, element.object);
	struct mp_zvid_transform *zvid_transform = (struct mp_zvid_transform *)transform;
	struct mp_buffer_pool *outpool = &zvid_transform->zvid_obj_out.pool.pool;
	struct video_buffer *in_vbuf;

	/* TODO: Ensure net_buf meta's priv is always a video buffer */
	if (mp_buffer_get_meta(in_buf)->priv == NULL) {
		in_vbuf = video_import_buffer(in_buf->data, in_buf->size);
	} else {
		in_vbuf = mp_buffer_get_meta(in_buf)->priv;
	}
	in_vbuf->bytesused = mp_buffer_get_meta(in_buf)->bytes_used;

	/* Enqueue input buffer */
	in_vbuf->type = VIDEO_BUF_TYPE_INPUT;
	if (video_enqueue(zvid_transform->zvid_obj_in.vdev, in_vbuf) != 0) {
		LOG_ERR("Failed to enqueue input buffer");
		return -EIO;
	}

	/* Dequeue an input buffer, blocking */
	struct video_buffer *vbuf =
		&(struct video_buffer){.type = zvid_transform->zvid_obj_in.type};
	ret = video_dequeue(zvid_transform->zvid_obj_in.vdev, &vbuf, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to dequeue input buffer");
		return -EIO;
	}

	/* Done with the input buffer */
	net_buf_unref(in_buf);

	/* Dequeue an output buffer, blocking */
	ret = outpool->acquire_buffer(outpool, out_buf);
	if (ret != 0) {
		LOG_ERR("Failed to acquire output buffer");
		return -ENOMEM;
	}

	return 0;
}

static struct mp_caps *mp_zvid_transform_supported_caps(struct mp_transform *transform,
							enum mp_pad_direction direction)
{
	struct mp_zvid_transform *zvid_transform = (struct mp_zvid_transform *)transform;

	if (direction == MP_PAD_SINK) {
		return mp_zvid_object_get_caps(&zvid_transform->zvid_obj_in);
	}

	if (direction == MP_PAD_SRC) {
		return mp_zvid_object_get_caps(&zvid_transform->zvid_obj_out);
	}

	return NULL;
}

static void mp_zvid_transform_update_caps(struct mp_transform *transform)
{
	struct mp_caps *sink_caps = mp_zvid_transform_supported_caps(transform, MP_PAD_SINK);
	struct mp_caps *src_caps = mp_zvid_transform_supported_caps(transform, MP_PAD_SRC);

	mp_transform_update_caps(transform, sink_caps, src_caps);
	mp_caps_unref(sink_caps);
	mp_caps_unref(src_caps);
}

static int mp_zvid_transform_set_caps(struct mp_transform *transform,
				      enum mp_pad_direction direction, struct mp_caps *caps)
{
	struct mp_zvid_transform *zvid_transform = (struct mp_zvid_transform *)transform;
	struct mp_zvid_object *zvid_obj = NULL;

	if (direction == MP_PAD_SINK) {
		zvid_obj = &zvid_transform->zvid_obj_in;
	}

	if (direction == MP_PAD_SRC) {
		zvid_obj = &zvid_transform->zvid_obj_out;
	}

	if (zvid_obj == NULL || mp_zvid_object_set_caps(zvid_obj, caps) < 0) {
		return -EINVAL;
	}

	/* Set pad's caps only when everything is OK */
	mp_caps_replace(
		direction == MP_PAD_SRC ? &transform->srcpad.caps : &transform->sinkpad.caps, caps);

	return 0;
}

static struct mp_caps *mp_zvid_transform_transform_caps(struct mp_transform *self,
							enum mp_pad_direction direction,
							struct mp_caps *caps)
{
	struct mp_zvid_transform *zvid_transform = (struct mp_zvid_transform *)self;
	const struct device *dev = zvid_transform->zvid_obj_in.vdev;
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
		while (video_transform_cap(dev, &vfc, &other_vfc, direction, ind) == 0) {
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

			ind++;
		}
	}

	return other_caps;
}

static int mp_zvid_transform_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_transform *transform = (struct mp_transform *)obj;
	struct mp_zvid_transform *zvid_transform = (struct mp_zvid_transform *)obj;

	switch (key) {
	case PROP_ZVID_DEVICE:
		mp_zvid_object_set_property(&zvid_transform->zvid_obj_in, key, val);
		mp_zvid_object_set_property(&zvid_transform->zvid_obj_out, key, val);
		/* Device set, update caps */
		mp_zvid_transform_update_caps(transform);

		return 0;
	default:
		return mp_zvid_object_set_property(&zvid_transform->zvid_obj_in, key, val);
	}
}

static int mp_zvid_transform_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_zvid_transform *zvid_transform = (struct mp_zvid_transform *)obj;

	return mp_zvid_object_get_property(&zvid_transform->zvid_obj_in, key, val);
}

static int mp_zvid_transform_decide_allocation(struct mp_transform *self, struct mp_dispatch *query)
{
	struct mp_zvid_transform *zvid_transform = (struct mp_zvid_transform *)self;

	return mp_zvid_object_decide_allocation(&zvid_transform->zvid_obj_out, query);
}

static int mp_zvid_transform_propose_allocation(struct mp_transform *self, struct mp_dispatch *query)
{
	return mp_dispatch_set_pool(query, self->inpool);
}

void mp_zvid_transform_init(struct mp_element *self)
{
	struct mp_transform *transform = (struct mp_transform *)self;
	struct mp_zvid_transform *zvid_transform = (struct mp_zvid_transform *)transform;

	/* Init base class */
	mp_transform_init(self);

	/* Initialize zvid objects */
	zvid_transform->zvid_obj_in.vdev = DEFAULT_PROP_DEVICE;
	zvid_transform->zvid_obj_out.vdev = DEFAULT_PROP_DEVICE;
	zvid_transform->zvid_obj_in.type = VIDEO_BUF_TYPE_INPUT;
	zvid_transform->zvid_obj_out.type = VIDEO_BUF_TYPE_OUTPUT;

	self->object.set_property = mp_zvid_transform_set_property;
	self->object.get_property = mp_zvid_transform_get_property;

	/*
	 * m2m devices have both input and output buffer queues,
	 * so it should be in normal mode by default
	 */
	transform->mode = MP_MODE_NORMAL;

	/*
	 * pools needs to be set before retrieving supported caps as
	 * some pool's configs will be set during caps probing.
	 */
	transform->inpool = &zvid_transform->zvid_obj_in.pool.pool;
	transform->outpool = &zvid_transform->zvid_obj_out.pool.pool;
	/* Initialize buffer pools */
	mp_zvid_buffer_pool_init(transform->inpool, &(zvid_transform->zvid_obj_in));
	mp_zvid_buffer_pool_init(transform->outpool, &(zvid_transform->zvid_obj_out));

	mp_zvid_transform_update_caps(transform);

	transform->set_caps = mp_zvid_transform_set_caps;
	transform->transform_caps = mp_zvid_transform_transform_caps;
	transform->sinkpad.chainfn = mp_zvid_transform_chainfn;
	transform->decide_allocation = mp_zvid_transform_decide_allocation;
	transform->propose_allocation = mp_zvid_transform_propose_allocation;
}
