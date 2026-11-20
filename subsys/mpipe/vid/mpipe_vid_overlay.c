/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/vid/mpipe_vid_overlay.h>

LOG_MODULE_REGISTER(mpipe_vid_overlay, CONFIG_MPIPE_LOG_LEVEL);

/* Formats the element can hand to a draw callback */
static const uint32_t vid_overlay_pixfmts[] = {
	VIDEO_PIX_FMT_RGB565,
	VIDEO_PIX_FMT_BGRX32,
};

static bool vid_overlay_supports(uint32_t pixfmt)
{
	for (size_t i = 0; i < ARRAY_SIZE(vid_overlay_pixfmts); i++) {
		if (vid_overlay_pixfmts[i] == pixfmt) {
			return true;
		}
	}

	return false;
}

static int vid_overlay_candidate(uint32_t pixfmt, struct mpipe_structure *out)
{
	struct mpipe_value v;
	int ret;

	ret = mpipe_structure_init(out, MPIPE_MEDIA_VIDEO);
	if (ret != 0) {
		return ret;
	}

	v.type = MPIPE_TYPE_UINT;
	v.v_uint = pixfmt;
	ret = mpipe_structure_append_value(out, MPIPE_CAPS_PIXEL_FORMAT, &v);
	if (ret != 0) {
		mpipe_structure_clear(out);
	}

	return ret;
}

static int vid_overlay_enum_caps(struct mpipe_pad *pad, uint32_t index,
				 const struct mpipe_structure *filter, struct mpipe_structure *out)
{
	struct mpipe_structure candidate;
	int ret;

	ARG_UNUSED(pad);

	if (index >= ARRAY_SIZE(vid_overlay_pixfmts)) {
		return -ENOENT;
	}

	ret = vid_overlay_candidate(vid_overlay_pixfmts[index], &candidate);
	if (ret != 0) {
		return ret;
	}

	return mpipe_pad_enum_filter(&candidate, filter, out);
}

static int vid_overlay_transform_caps(struct mpipe_transform *self,
				      enum mpipe_pad_direction direction,
				      const struct mpipe_structure *in, uint32_t index,
				      struct mpipe_structure *out)
{
	int ret;

	ARG_UNUSED(self);
	ARG_UNUSED(direction);

	if (in == NULL) {
		return -EINVAL;
	}

	if (mpipe_structure_is_any(in)) {
		/* Constrains nothing: every supported format is reachable */
		if (index >= ARRAY_SIZE(vid_overlay_pixfmts)) {
			return -ENOENT;
		}

		return vid_overlay_candidate(vid_overlay_pixfmts[index], out);
	}

	/* Drawing preserves the format: the identity is the only transformation */
	if (index > 0U) {
		return -ENOENT;
	}

	const struct mpipe_value *pix = mpipe_structure_get_value(in, MPIPE_CAPS_PIXEL_FORMAT);

	if (pix == NULL || pix->type != MPIPE_TYPE_UINT ||
	    !vid_overlay_supports(mpipe_value_get_uint(pix))) {
		return -ENOENT;
	}

	ret = vid_overlay_candidate(mpipe_value_get_uint(pix), out);
	if (ret != 0) {
		return ret;
	}

	/* Drawing changes nothing else; an input that does not constrain a field is fine */
	static const uint8_t passthrough[] = {MPIPE_CAPS_IMAGE_WIDTH, MPIPE_CAPS_IMAGE_HEIGHT,
					      MPIPE_CAPS_FRAME_INTERVAL};

	for (uint8_t i = 0; i < ARRAY_SIZE(passthrough); i++) {
		ret = mpipe_structure_copy_field(in, out, passthrough[i]);
		if (ret != 0 && ret != -ENOENT) {
			mpipe_structure_clear(out);
			return ret;
		}
	}

	return 0;
}

static int vid_overlay_set_caps(struct mpipe_transform *transform,
				enum mpipe_pad_direction direction,
				const struct mpipe_structure *caps)
{
	struct mpipe_vid_overlay *overlay = (struct mpipe_vid_overlay *)transform;
	const struct mpipe_value *v;
	int ret;

	if (caps == NULL) {
		return -EINVAL;
	}

	ret = mpipe_transform_set_caps(transform, direction, caps);
	if (ret < 0) {
		return ret;
	}

	v = mpipe_structure_get_value(caps, MPIPE_CAPS_PIXEL_FORMAT);
	if (v != NULL && v->type == MPIPE_TYPE_UINT) {
		overlay->pixfmt = mpipe_value_get_uint(v);
	}

	v = mpipe_structure_get_value(caps, MPIPE_CAPS_IMAGE_WIDTH);
	if (v != NULL && v->type == MPIPE_TYPE_UINT) {
		overlay->width = (uint16_t)mpipe_value_get_uint(v);
	}

	v = mpipe_structure_get_value(caps, MPIPE_CAPS_IMAGE_HEIGHT);
	if (v != NULL && v->type == MPIPE_TYPE_UINT) {
		overlay->height = (uint16_t)mpipe_value_get_uint(v);
	}

	if (overlay->width != 0U && overlay->height != 0U && overlay->pixfmt != 0U) {
		overlay->out_pool.config.size = (uint32_t)overlay->width * overlay->height *
						video_bits_per_pixel(overlay->pixfmt) /
						BITS_PER_BYTE;
	}

	return 0;
}

static int vid_overlay_process_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				  struct net_buf **out_buf)
{
	struct mpipe_transform *transform = (struct mpipe_transform *)pad->object.container;
	struct mpipe_vid_overlay *overlay = (struct mpipe_vid_overlay *)transform;
	struct mpipe_buffer_pool *out_pool = transform->out_pool;
	struct net_buf *out = NULL;
	struct mpipe_buffer_meta *meta;
	uint32_t len;

	*out_buf = NULL;

	if (overlay->width == 0U || overlay->height == 0U || overlay->pixfmt == 0U) {
		LOG_ERR("Missing negotiated caps");
		net_buf_unref(in_buf);
		return -EINVAL;
	}

	/*
	 * A frame nobody else holds is drawn in place: only behind a tee, where
	 * every branch sees the same payload, is the copy needed. A view shares
	 * its parent's bytes with other views, so it is copied as well.
	 */
	if (in_buf->ref == 1U && mpipe_buffer_get_meta(in_buf)->parent == NULL) {
		if (overlay->cb.fn != NULL) {
			overlay->cb.fn(in_buf->data, overlay->width, overlay->height,
				       overlay->pixfmt, overlay->cb.user_data);
		}
		*out_buf = in_buf;
		return 0;
	}

	if (out_pool->acquire_buffer(out_pool, &out) != 0 || out == NULL) {
		LOG_ERR("Failed to acquire output buffer");
		net_buf_unref(in_buf);
		return -EIO;
	}

	len = MIN(overlay->out_pool.config.size, mpipe_buffer_get_meta(in_buf)->bytes_used);
	memcpy(out->data, in_buf->data, len);

	if (overlay->cb.fn != NULL) {
		overlay->cb.fn(out->data, overlay->width, overlay->height, overlay->pixfmt,
			       overlay->cb.user_data);
	}

	meta = mpipe_buffer_get_meta(out);
	meta->bytes_used = len;
	meta->pts = mpipe_buffer_get_meta(in_buf)->pts;
	out->len = len;

	if (meta->driver_buf != NULL) {
		((struct video_buffer *)meta->driver_buf)->bytesused = len;
	}

	net_buf_unref(in_buf);
	*out_buf = out;

	return 0;
}

static int mpipe_vid_overlay_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_vid_overlay *overlay = (struct mpipe_vid_overlay *)obj;

	if (val == NULL) {
		return -EINVAL;
	}

	switch (key) {
	case MPIPE_PROP_VID_OVERLAY_DRAW_CB:
		overlay->cb = *(const struct mpipe_vid_overlay_cb *)val;
		return 0;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}
}

/*
 * Downstream's demand lands on the overlay's own pool, which supplies the
 * copies it draws into when a frame is shared, and travels upstream as well,
 * since a frame drawn in place is upstream's buffer held downstream. The
 * overlay itself holds nothing across process calls.
 */
static int vid_overlay_decide_buffer_pool(struct mpipe_transform *self,
					  struct mpipe_dispatch *query)
{
	struct mpipe_vid_overlay *overlay = (struct mpipe_vid_overlay *)self;
	const struct mpipe_buffer_pool_config *qpc =
		(query->pool != NULL) ? &query->pool->config : &query->pool_cfg;

	overlay->held_downstream = qpc->min_buffers;
	overlay->out_pool.config.min_buffers = (uint8_t)MIN(
		(uint32_t)overlay->out_pool.req_config.min_buffers + qpc->min_buffers, UINT8_MAX);

	return 0;
}

static int vid_overlay_propose_buffer_pool(struct mpipe_transform *self,
					   struct mpipe_dispatch *query)
{
	struct mpipe_vid_overlay *overlay = (struct mpipe_vid_overlay *)self;

	query->pool_cfg.min_buffers = overlay->held_downstream;

	return 0;
}

/* Frame-sized output buffers come from the video buffer heap */
static int vid_overlay_pool_start(struct mpipe_buffer_pool *pool)
{
	struct mpipe_vid_overlay *overlay = CONTAINER_OF(pool, struct mpipe_vid_overlay, out_pool);

	if (pool->config.min_buffers == 0U) {
		pool->config.min_buffers = 1U;
	}

	if (pool->config.min_buffers > CONFIG_VIDEO_BUFFER_POOL_NUM_MAX) {
		LOG_ERR("min_buffers=%u exceeds CONFIG_VIDEO_BUFFER_POOL_NUM_MAX=%u",
			pool->config.min_buffers, CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
		return -EINVAL;
	}

	overlay->vbuf_count = (uint8_t)pool->config.min_buffers;

	for (uint8_t i = 0; i < overlay->vbuf_count; i++) {
		struct video_buffer *vbuf = video_buffer_alloc(pool->config.size, K_NO_WAIT);

		if (vbuf == NULL) {
			LOG_ERR("Failed to allocate video buffer %u", i);
			return -ENOBUFS;
		}
		overlay->vbufs[i] = vbuf;
		k_fifo_put(&overlay->free_fifo, vbuf);
	}

	return 0;
}

static int vid_overlay_pool_stop(struct mpipe_buffer_pool *pool)
{
	struct mpipe_vid_overlay *overlay = CONTAINER_OF(pool, struct mpipe_vid_overlay, out_pool);

	for (uint8_t i = 0; i < overlay->vbuf_count; i++) {
		if (overlay->vbufs[i] != NULL) {
			(void)video_buffer_release(overlay->vbufs[i]);
			overlay->vbufs[i] = NULL;
		}
	}

	overlay->vbuf_count = 0;

	return 0;
}

static int vid_overlay_pool_acquire(struct mpipe_buffer_pool *pool, struct net_buf **out)
{
	struct mpipe_vid_overlay *overlay = CONTAINER_OF(pool, struct mpipe_vid_overlay, out_pool);
	struct video_buffer *vbuf;
	struct mpipe_buffer_meta *meta;

	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(out != NULL);

	vbuf = k_fifo_get(&overlay->free_fifo, K_FOREVER);
	if (vbuf == NULL) {
		return -ENOBUFS;
	}

	*out = net_buf_alloc_with_data(pool->nb_pool, vbuf->buffer, vbuf->size, K_NO_WAIT);
	if (*out == NULL) {
		k_fifo_put(&overlay->free_fifo, vbuf);
		return -ENOBUFS;
	}

	meta = mpipe_buffer_get_meta(*out);
	meta->pool = pool;
	meta->driver_buf = vbuf;
	meta->priv = NULL;
	meta->bytes_used = 0;
	meta->pts = 0;
	(*out)->len = 0;

	return 0;
}

static int vid_overlay_pool_release(struct mpipe_buffer_pool *pool, struct net_buf *buf)
{
	struct mpipe_vid_overlay *overlay = CONTAINER_OF(pool, struct mpipe_vid_overlay, out_pool);
	struct video_buffer *vbuf;

	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	vbuf = (struct video_buffer *)mpipe_buffer_get_meta(buf)->driver_buf;
	if (vbuf != NULL) {
		k_fifo_put(&overlay->free_fifo, vbuf);
	}

	return 0;
}

int mpipe_vid_overlay_init(struct mpipe_vid_overlay *overlay, uint8_t id)
{
	__ASSERT_NO_MSG(overlay != NULL);

	struct mpipe_element *self = &overlay->transform.element;
	struct mpipe_transform *transform = &overlay->transform;
	int ret = mpipe_transform_init(transform, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "vid_overlay");

	self->object.set_property = mpipe_vid_overlay_set_property;

	transform->sink_pad.enum_caps_fn = vid_overlay_enum_caps;
	transform->src_pad.enum_caps_fn = vid_overlay_enum_caps;

	overlay->width = 0;
	overlay->height = 0;
	overlay->pixfmt = 0;
	overlay->cb.fn = NULL;
	overlay->cb.user_data = NULL;

	k_fifo_init(&overlay->free_fifo);
	memset(overlay->vbufs, 0, sizeof(overlay->vbufs));
	overlay->vbuf_count = 0;
	overlay->held_downstream = 0;

	mpipe_buffer_pool_init(&overlay->out_pool);
	overlay->out_pool.start = vid_overlay_pool_start;
	overlay->out_pool.stop = vid_overlay_pool_stop;
	overlay->out_pool.acquire_buffer = vid_overlay_pool_acquire;
	overlay->out_pool.release_buffer = vid_overlay_pool_release;

	const struct mpipe_buffer_pool_config pool_req = {
		.size = 0,
		.align = 1,
		.min_buffers = 1,
		.max_buffers = CONFIG_VIDEO_BUFFER_POOL_NUM_MAX,
	};

	(void)mpipe_buffer_pool_set_req_config(&overlay->out_pool, &pool_req);

	transform->mode = MPIPE_TRANSFORM_MODE_NORMAL;
	transform->out_pool = &overlay->out_pool;
	transform->set_caps = vid_overlay_set_caps;
	transform->transform_caps = vid_overlay_transform_caps;
	transform->propose_buffer_pool = vid_overlay_propose_buffer_pool;
	transform->decide_buffer_pool = vid_overlay_decide_buffer_pool;
	transform->sink_pad.process_fn = vid_overlay_process_fn;

	return 0;
}
