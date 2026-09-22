/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/base/mpipe_app_src.h>

LOG_MODULE_REGISTER(mpipe_app_src, CONFIG_MPIPE_LOG_LEVEL);

NET_BUF_POOL_FIXED_DEFINE(mpipe_app_src_pool, CONFIG_MPIPE_BASE_APP_SRC_POOL_NUM,
			  CONFIG_MPIPE_BASE_APP_SRC_BUF_SZ, sizeof(struct mpipe_buffer_meta),
			  mpipe_buffer_destroy);

/*
 * Sentinels enqueued among the buffers, told apart by address. EOS makes the
 * pipeline thread's acquire report end of stream; flush makes it report a
 * forced stop so that teardown never leaves it blocked on an empty queue.
 */
static uint8_t eos_sentinel;
static uint8_t flush_sentinel;

int mpipe_app_src_alloc(struct mpipe_app_src *app_src, uint32_t size, k_timeout_t timeout,
			struct net_buf **buf)
{
	struct net_buf *nb;
	struct mpipe_buffer_meta *meta;

	if (app_src == NULL || buf == NULL || size == 0U ||
	    size > CONFIG_MPIPE_BASE_APP_SRC_BUF_SZ) {
		return -EINVAL;
	}

	nb = net_buf_alloc_len(app_src->pool.nb_pool, size, timeout);
	if (nb == NULL) {
		return -ENOBUFS;
	}

	meta = mpipe_buffer_get_meta(nb);
	meta->pool = &app_src->pool;
	meta->bytes_used = 0;
	meta->timestamp = 0;
	meta->driver_buf = NULL;
	meta->priv = NULL;
	nb->len = 0;

	*buf = nb;

	return 0;
}

int mpipe_app_src_push_buf(struct mpipe_app_src *app_src, struct net_buf *buf, uint32_t size,
			   k_timeout_t timeout)
{
	struct mpipe_buffer_meta *meta;

	if (app_src == NULL || buf == NULL || size == 0U || size > buf->size) {
		return -EINVAL;
	}

	buf->len = size;
	meta = mpipe_buffer_get_meta(buf);
	meta->bytes_used = size;
	meta->timestamp = 0;

	if (k_msgq_put(&app_src->msgq, &buf, timeout) != 0) {
		return -EAGAIN;
	}

	return 0;
}

int mpipe_app_src_push(struct mpipe_app_src *app_src, const void *data, uint32_t size,
		       k_timeout_t timeout)
{
	struct net_buf *nb;
	int ret;

	if (data == NULL) {
		return -EINVAL;
	}

	ret = mpipe_app_src_alloc(app_src, size, timeout, &nb);
	if (ret != 0) {
		return ret;
	}

	memcpy(nb->data, data, size);

	ret = mpipe_app_src_push_buf(app_src, nb, size, timeout);
	if (ret != 0) {
		net_buf_unref(nb);
	}

	return ret;
}

int mpipe_app_src_eos(struct mpipe_app_src *app_src, k_timeout_t timeout)
{
	void *eos_ptr = &eos_sentinel;

	if (app_src == NULL) {
		return -EINVAL;
	}

	if (k_msgq_put(&app_src->msgq, &eos_ptr, timeout) != 0) {
		return -EAGAIN;
	}

	return 0;
}

static int mpipe_app_src_pool_acquire(struct mpipe_buffer_pool *pool, struct net_buf **buf)
{
	struct mpipe_app_src *app_src = CONTAINER_OF(pool, struct mpipe_app_src, pool);
	void *entry;
	int ret;

	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	ret = k_msgq_get(&app_src->msgq, &entry, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	if (entry == (void *)&eos_sentinel) {
		return -ENODATA;
	}

	if (entry == (void *)&flush_sentinel) {
		return -EPIPE;
	}

	*buf = entry;

	return 0;
}

static int mpipe_app_src_pool_release(struct mpipe_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	__ASSERT_NO_MSG(buf != NULL);

	struct mpipe_buffer_meta *meta = mpipe_buffer_get_meta(buf);

	if (meta != NULL) {
		meta->bytes_used = 0;
		meta->timestamp = 0;
		meta->driver_buf = NULL;
		meta->priv = NULL;
	}

	buf->len = 0;

	return 0;
}

/* The one capability the element offers is the one the application declared */
static int mpipe_app_src_enum_caps(struct mpipe_pad *pad, uint32_t index,
				   const struct mpipe_structure *filter,
				   struct mpipe_structure *out)
{
	struct mpipe_app_src *app_src = (struct mpipe_app_src *)pad->object.container;

	if (index > 0U) {
		return -ENOENT;
	}

	return mpipe_pad_enum_filter(&app_src->caps, filter, out);
}

static int mpipe_app_src_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_app_src *app_src = (struct mpipe_app_src *)obj;

	switch (key) {
	case MPIPE_PROP_BASE_APP_SRC_CAPS:
		if (val == NULL) {
			return -EINVAL;
		}

		app_src->caps = *(const struct mpipe_structure *)val;
		return 0;
	default:
		return mpipe_src_set_property(obj, key, val);
	}
}

static int mpipe_app_src_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_app_src *app_src = (struct mpipe_app_src *)obj;

	switch (key) {
	case MPIPE_PROP_BASE_APP_SRC_CAPS:
		if (val == NULL) {
			return -EINVAL;
		}

		*(const struct mpipe_structure **)val = &app_src->caps;
		return 0;
	default:
		return mpipe_src_get_property(obj, key, val);
	}
}

/* Drain queued buffers; sentinels are simply discarded */
static void mpipe_app_src_drain(struct mpipe_app_src *app_src)
{
	void *entry;

	while (k_msgq_get(&app_src->msgq, &entry, K_NO_WAIT) == 0) {
		if (entry != (void *)&eos_sentinel && entry != (void *)&flush_sentinel) {
			net_buf_unref((struct net_buf *)entry);
		}
	}
}

static int mpipe_app_src_change_state(struct mpipe_element *element,
				      enum mpipe_state_change transition)
{
	struct mpipe_app_src *app_src = (struct mpipe_app_src *)element;
	void *flush_ptr = &flush_sentinel;

	switch (transition) {
	case MPIPE_STATE_CHANGE_READY_TO_PAUSED:
		/* Start from a clean queue; no stale buffer or sentinel */
		mpipe_app_src_drain(app_src);
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		/*
		 * Unblock the pipeline thread if it waits on an empty queue,
		 * so the join that follows this transition can complete. The
		 * drain first frees a slot for the sentinel; if the thread
		 * consumes a buffer concurrently, the raised flushing gates
		 * make it drop the push.
		 */
		mpipe_app_src_drain(app_src);
		(void)k_msgq_put(&app_src->msgq, &flush_ptr, K_NO_WAIT);
		break;
	default:
		break;
	}

	return mpipe_src_change_state(element, transition);
}

int mpipe_app_src_init(struct mpipe_app_src *app_src, uint8_t id)
{
	__ASSERT_NO_MSG(app_src != NULL);

	struct mpipe_element *self = &app_src->src.element;
	int ret = mpipe_src_init(&app_src->src, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "app_src");

	self->object.set_property = mpipe_app_src_set_property;
	self->object.get_property = mpipe_app_src_get_property;
	self->change_state = mpipe_app_src_change_state;

	ret = mpipe_structure_init_any(&app_src->caps);
	if (ret != 0) {
		return ret;
	}

	app_src->src.src_pad.enum_caps_fn = mpipe_app_src_enum_caps;

	const struct mpipe_buffer_pool_config pool_req = {
		.size = CONFIG_MPIPE_BASE_APP_SRC_BUF_SZ,
		.min_buffers = 1,
		.max_buffers = CONFIG_MPIPE_BASE_APP_SRC_POOL_NUM,
	};

	mpipe_buffer_pool_init(&app_src->pool);
	app_src->pool.nb_pool = &mpipe_app_src_pool;
	(void)mpipe_buffer_pool_set_req_config(&app_src->pool, &pool_req);
	app_src->pool.acquire_buffer = mpipe_app_src_pool_acquire;
	app_src->pool.release_buffer = mpipe_app_src_pool_release;

	app_src->src.pool = &app_src->pool;

	k_msgq_init(&app_src->msgq, app_src->msgq_buffer, sizeof(void *),
		    CONFIG_MPIPE_BASE_APP_SRC_QUEUE_DEPTH + 1);

	return 0;
}
