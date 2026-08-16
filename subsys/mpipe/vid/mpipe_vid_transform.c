/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/vid/mpipe_vid_property.h>
#include <zephyr/mpipe/vid/mpipe_vid_transform.h>

LOG_MODULE_REGISTER(mpipe_vid_transform, CONFIG_MPIPE_LOG_LEVEL);

#define DEFAULT_PROP_DEVICE DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_videotrans))

static int mpipe_vid_transform_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
					struct net_buf **out_buf)
{
	int ret;
	struct mpipe_transform *transform =
		CONTAINER_OF(pad->object.container, struct mpipe_transform, element.object);
	struct mpipe_vid_transform *vid_transform = (struct mpipe_vid_transform *)transform;
	struct mpipe_buffer_pool *out_pool = &vid_transform->vid_obj_out.pool.pool;
	struct video_buffer *in_vbuf;

	/* TODO: Ensure net_buf meta's driver_buf is always a video buffer */
	if (mpipe_buffer_get_meta(in_buf)->driver_buf == NULL) {
		in_vbuf = video_import_buffer(in_buf->data, in_buf->size);
	} else {
		in_vbuf = mpipe_buffer_get_meta(in_buf)->driver_buf;
	}
	in_vbuf->bytesused = mpipe_buffer_get_meta(in_buf)->bytes_used;

	/* Enqueue input buffer */
	in_vbuf->type = VIDEO_BUF_TYPE_INPUT;
	if (video_enqueue(vid_transform->vid_obj_in.vdev, in_vbuf) != 0) {
		LOG_ERR("Failed to enqueue input buffer");
		net_buf_unref(in_buf);
		return -EIO;
	}

	/* Dequeue an input buffer, blocking */
	struct video_buffer *vbuf = &(struct video_buffer){.type = vid_transform->vid_obj_in.type};

	ret = video_dequeue(vid_transform->vid_obj_in.vdev, &vbuf, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to dequeue input buffer");
		net_buf_unref(in_buf);
		return -EIO;
	}

	/* Done with the input buffer */
	net_buf_unref(in_buf);

	/* Dequeue an output buffer, blocking */
	ret = out_pool->acquire_buffer(out_pool, out_buf);
	if (ret != 0) {
		LOG_ERR("Failed to acquire output buffer");
		return -ENOMEM;
	}

	return 0;
}

static int mpipe_vid_transform_enum_caps(struct mpipe_pad *pad, uint32_t index,
					 const struct mpipe_structure *filter,
					 struct mpipe_structure *out)
{
	struct mpipe_vid_transform *vid_transform =
		(struct mpipe_vid_transform *)pad->object.container;
	struct mpipe_vid_object *vid_obj;

	if (pad->direction == MPIPE_PAD_SINK) {
		vid_obj = &vid_transform->vid_obj_in;
	} else if (pad->direction == MPIPE_PAD_SRC) {
		vid_obj = &vid_transform->vid_obj_out;
	} else {
		return -EINVAL;
	}

	return mpipe_vid_object_enum_caps(vid_obj, index, filter, out);
}

static int mpipe_vid_transform_set_caps(struct mpipe_transform *transform,
					enum mpipe_pad_direction direction,
					const struct mpipe_structure *caps)
{
	struct mpipe_vid_transform *vid_transform = (struct mpipe_vid_transform *)transform;
	struct mpipe_vid_object *vid_obj = NULL;

	if (direction == MPIPE_PAD_SINK) {
		vid_obj = &vid_transform->vid_obj_in;
	}

	if (direction == MPIPE_PAD_SRC) {
		vid_obj = &vid_transform->vid_obj_out;
	}

	if (vid_obj == NULL || mpipe_vid_object_set_caps(vid_obj, caps) < 0) {
		return -EINVAL;
	}

	/* Set pad's caps only when everything is OK */
	return mpipe_pad_set_caps(
		direction == MPIPE_PAD_SRC ? &transform->src_pad : &transform->sink_pad, caps);
}

static int mpipe_vid_transform_transform_caps(struct mpipe_transform *self,
					      enum mpipe_pad_direction direction,
					      const struct mpipe_structure *in, uint32_t index,
					      struct mpipe_structure *out)
{
	struct mpipe_vid_transform *vid_transform = (struct mpipe_vid_transform *)self;
	const struct device *dev = vid_transform->vid_obj_in.vdev;
	struct video_format_cap vfc, other_vfc;

	if (direction != MPIPE_PAD_SINK && direction != MPIPE_PAD_SRC) {
		return -EINVAL;
	}

	if (in == NULL || mpipe_vid_caps_to_vfc(in, &vfc) < 0) {
		return -ENOENT;
	}

	if (video_transform_cap(dev, &vfc, &other_vfc, direction, (uint16_t)index) != 0) {
		return -ENOENT;
	}

	return mpipe_vid_vfc_to_caps(&other_vfc, out);
}

static int mpipe_vid_transform_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_vid_transform *vid_transform = (struct mpipe_vid_transform *)obj;

	switch (key) {
	case MPIPE_PROP_VID_DEVICE: {
		const struct device *prev_vdev = vid_transform->vid_obj_in.vdev;

		mpipe_vid_object_set_property(&vid_transform->vid_obj_in, key, val);
		mpipe_vid_object_set_property(&vid_transform->vid_obj_out, key, val);

		/*
		 * Re-read the pool parameters from the new device. Probing disturbs
		 * the compose selection, so skip it when nothing changed.
		 */
		if (vid_transform->vid_obj_in.vdev != prev_vdev) {
			(void)mpipe_vid_object_probe_bounds(&vid_transform->vid_obj_in);
			(void)mpipe_vid_object_probe_bounds(&vid_transform->vid_obj_out);
		}

		return 0;
	}
	default:
		return mpipe_vid_object_set_property(&vid_transform->vid_obj_in, key, val);
	}
}

static int mpipe_vid_transform_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_vid_transform *vid_transform = (struct mpipe_vid_transform *)obj;

	return mpipe_vid_object_get_property(&vid_transform->vid_obj_in, key, val);
}

static int mpipe_vid_transform_decide_buffer_pool(struct mpipe_transform *self,
						  struct mpipe_dispatch *query)
{
	struct mpipe_vid_transform *vid_transform = (struct mpipe_vid_transform *)self;

	return mpipe_vid_object_decide_buffer_pool(&vid_transform->vid_obj_out, query);
}

static int mpipe_vid_transform_propose_buffer_pool(struct mpipe_transform *self,
						   struct mpipe_dispatch *query)
{
	struct mpipe_vid_transform *vid_transform = (struct mpipe_vid_transform *)self;
	struct mpipe_vid_object *vid_obj = &vid_transform->vid_obj_in;

	query->pool = &vid_obj->pool.pool;

	return 0;
}

int mpipe_vid_transform_init(struct mpipe_vid_transform *vid_transform, uint8_t id)
{
	__ASSERT_NO_MSG(vid_transform != NULL);

	struct mpipe_element *self = &vid_transform->transform.element;
	struct mpipe_transform *transform = &vid_transform->transform;
	int ret = mpipe_transform_init(transform, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "vid_transform");

	/* Initialize vid objects */
	vid_transform->vid_obj_in.vdev = DEFAULT_PROP_DEVICE;
	vid_transform->vid_obj_out.vdev = DEFAULT_PROP_DEVICE;
	vid_transform->vid_obj_in.type = VIDEO_BUF_TYPE_INPUT;
	vid_transform->vid_obj_out.type = VIDEO_BUF_TYPE_OUTPUT;

	self->object.set_property = mpipe_vid_transform_set_property;
	self->object.get_property = mpipe_vid_transform_get_property;

	/*
	 * m2m devices have both input and output buffer queues,
	 * so it should be in normal mode by default
	 */
	transform->mode = MPIPE_MODE_NORMAL;

	transform->in_pool = &vid_transform->vid_obj_in.pool.pool;
	transform->out_pool = &vid_transform->vid_obj_out.pool.pool;
	/* Initialize buffer pools */
	mpipe_vid_buffer_pool_init(transform->in_pool, &(vid_transform->vid_obj_in));
	mpipe_vid_buffer_pool_init(transform->out_pool, &(vid_transform->vid_obj_out));

	/*
	 * Probe the pool parameters for both directions here. The formats are
	 * enumerated from the device on demand instead, so the pad caps stay ANY
	 * until one is negotiated. The pools cannot wait for that enumeration:
	 * the src pad's capability comes out of transform_caps(), so vid_obj_out
	 * is never enumerated.
	 */
	(void)mpipe_vid_object_probe_bounds(&vid_transform->vid_obj_in);
	(void)mpipe_vid_object_probe_bounds(&vid_transform->vid_obj_out);

	transform->sink_pad.enum_caps_fn = mpipe_vid_transform_enum_caps;
	transform->src_pad.enum_caps_fn = mpipe_vid_transform_enum_caps;

	transform->set_caps = mpipe_vid_transform_set_caps;
	transform->transform_caps = mpipe_vid_transform_transform_caps;
	transform->sink_pad.chain_fn = mpipe_vid_transform_chain_fn;
	transform->decide_buffer_pool = mpipe_vid_transform_decide_buffer_pool;
	transform->propose_buffer_pool = mpipe_vid_transform_propose_buffer_pool;

	return 0;
}
