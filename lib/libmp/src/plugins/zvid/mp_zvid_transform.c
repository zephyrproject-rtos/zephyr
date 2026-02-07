/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <src/core/mp_caps.h>

#include "mp_zvid_property.h"
#include "mp_zvid_transform.h"

LOG_MODULE_REGISTER(mp_zvid_transform, CONFIG_LIBMP_LOG_LEVEL);

#define DEFAULT_PROP_DEVICE DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_videotrans))

static bool mp_zvid_transform_chainfn(struct mp_pad *pad, struct mp_buffer *buffer)
{
	int ret;
	struct mp_transform *transform = MP_TRANSFORM(pad->object.container);
	struct mp_zvid_transform *zvid_transform = MP_ZVID_TRANSFORM(transform);
	struct mp_buffer_pool *outpool = MP_BUFFER_POOL(&zvid_transform->zvid_obj_out.pool);
	struct mp_buffer *out_buf = NULL;
	struct video_buffer in_vbuf = {.type = VIDEO_BUF_TYPE_INPUT, .index = buffer->index};

	/* Enqueue input buffer */
	if (video_enqueue(zvid_transform->zvid_obj_in.vdev, &in_vbuf)) {
		LOG_ERR("Unable to enqueue input buffer");
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

static struct mp_caps *mp_zvid_transform_get_caps(struct mp_transform *transform,
						  enum mp_pad_direction direction)
{
	struct mp_zvid_transform *zvid_transform = MP_ZVID_TRANSFORM(transform);

	if (direction == MP_PAD_SINK) {
		return mp_zvid_object_get_caps(&zvid_transform->zvid_obj_in);
	} else if (direction == MP_PAD_SRC) {
		return mp_zvid_object_get_caps(&zvid_transform->zvid_obj_out);
	}

	return NULL;
}

static bool mp_zvid_transform_set_caps(struct mp_transform *transform,
				       enum mp_pad_direction direction, struct mp_caps *caps)
{
	struct mp_zvid_transform *zvid_transform = MP_ZVID_TRANSFORM(transform);
	struct mp_zvid_object *zvid_obj = NULL;

	if (direction == MP_PAD_SINK) {
		zvid_obj = &zvid_transform->zvid_obj_in;
	}

	if (direction == MP_PAD_SRC) {
		zvid_obj = &zvid_transform->zvid_obj_out;
	}

	if (zvid_obj == NULL || !mp_zvid_object_set_caps(zvid_obj, caps)) {
		return false;
	}

	/* Set pad's caps only when everything is OK */
	mp_caps_replace(&transform->sinkpad.caps, caps);

	return true;
}

static struct mp_caps *mp_zvid_transform_transform_caps(struct mp_transform *self,
							enum mp_pad_direction direction,
							struct mp_caps *caps)
{
	struct mp_zvid_transform *zvid_transform = MP_ZVID_TRANSFORM(self);
	const struct device *dev = zvid_transform->zvid_obj_in.vdev;
	struct mp_caps *other_caps = mp_caps_new(MP_MEDIA_END);
	struct mp_structure *caps_item = NULL;
	struct mp_cap_structure *cs;
	struct video_format_cap vfc, other_vfc;
	uint16_t ind;

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
	int ret = 0;
	struct mp_transform *transform = MP_TRANSFORM(obj);
	struct mp_zvid_transform *zvid_transform = MP_ZVID_TRANSFORM(obj);

	switch (key) {
	case PROP_ZVID_DEVICE:
		mp_zvid_object_set_property(&zvid_transform->zvid_obj_in, key, val,
					    &transform->sinkpad.caps);
		mp_zvid_object_set_property(&zvid_transform->zvid_obj_out, key, val,
					    &transform->srcpad.caps);
		break;
	default:
		ret = mp_zvid_object_set_property(&zvid_transform->zvid_obj_in, key, val, NULL);
		if (ret == -ENOTSUP) {
			return mp_transform_set_property(obj, key, val);
		}
	}

	return ret;
}

static int mp_zvid_transform_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	int ret = 0;
	struct mp_zvid_transform *self = MP_ZVID_TRANSFORM(obj);

	ret = mp_zvid_object_get_property(&self->zvid_obj_in, key, val);
	if (ret == -ENOTSUP) {
		return mp_transform_get_property(obj, key, val);
	}

	return ret;
}

static bool mp_zvid_transform_decide_allocation(struct mp_transform *self, struct mp_query *query)
{
	return mp_zvid_object_decide_allocation(&MP_ZVID_TRANSFORM(self)->zvid_obj_out, query);
}

static bool mp_zvid_transform_propose_allocation(struct mp_transform *self, struct mp_query *query)
{
	return mp_query_set_pool(query, self->inpool);
}

void mp_zvid_transform_init(struct mp_element *self)
{
	struct mp_transform *transform = MP_TRANSFORM(self);
	struct mp_zvid_transform *zvid_transform = MP_ZVID_TRANSFORM(self);

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
	 * pools needs to be set before calling get_caps() as
	 * some pool's configs will be set during get_caps()
	 */
	transform->inpool = MP_BUFFER_POOL(&zvid_transform->zvid_obj_in.pool);
	transform->outpool = MP_BUFFER_POOL(&zvid_transform->zvid_obj_out.pool);

	transform->sinkpad.caps = mp_zvid_transform_get_caps(transform, MP_PAD_SINK);
	transform->srcpad.caps = mp_zvid_transform_get_caps(transform, MP_PAD_SRC);
	transform->transform_caps = mp_zvid_transform_transform_caps;
	transform->get_caps = mp_zvid_transform_get_caps;
	transform->set_caps = mp_zvid_transform_set_caps;
	transform->sinkpad.chainfn = mp_zvid_transform_chainfn;
	transform->decide_allocation = mp_zvid_transform_decide_allocation;
	transform->propose_allocation = mp_zvid_transform_propose_allocation;

	/* Initialize buffer pools */
	mp_zvid_buffer_pool_init(transform->inpool, &(zvid_transform->zvid_obj_in));
	mp_zvid_buffer_pool_init(transform->outpool, &(zvid_transform->zvid_obj_out));
}
