/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>

#include <zephyr/mpipe/fs/mpipe_file_src.h>

LOG_MODULE_REGISTER(mpipe_file_src, CONFIG_MPIPE_LOG_LEVEL);

NET_BUF_POOL_FIXED_DEFINE(mpipe_fs_nb_pool, CONFIG_MPIPE_FS_NUM_BUFS, CONFIG_MPIPE_FS_BLOCK_SIZE,
			  sizeof(struct mpipe_buffer_meta), mpipe_buffer_destroy);

static int mpipe_file_src_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_file_src *fsrc = (struct mpipe_file_src *)obj;
	int ret;

	switch (key) {
	case MPIPE_PROP_FS_SRC_PATH:
		fsrc->path = (const char *)val;
		return 0;
	case MPIPE_PROP_FS_SRC_BLOCKSIZE:
		if (val == NULL) {
			return -EINVAL;
		}
		fsrc->blocksize = *(const uint32_t *)val;
		return 0;
	default:
		ret = mpipe_src_set_property(obj, key, val);
		return ret;
	}
}

static int mpipe_file_src_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_file_src *fsrc = (struct mpipe_file_src *)obj;
	int ret;

	switch (key) {
	case MPIPE_PROP_FS_SRC_PATH:
		*(const char **)val = fsrc->path;
		return 0;
	case MPIPE_PROP_FS_SRC_BLOCKSIZE:
		*(uint32_t *)val = fsrc->blocksize;
		return 0;
	default:
		ret = mpipe_src_get_property(obj, key, val);
		return ret;
	}
}

static int mpipe_file_src_decide_buffer_pool(struct mpipe_src *src, struct mpipe_dispatch *query)
{
	struct mpipe_file_src *fsrc = (struct mpipe_file_src *)src;

	fsrc->downstream_pool = query->pool;

	return 0;
}

static int mpipe_file_src_read_chunk(struct mpipe_file_src *fsrc, struct net_buf *buf)
{
	struct mpipe_buffer_meta *m = mpipe_buffer_get_meta(buf);
	uint32_t cap;
	ssize_t rd;

	if (!fsrc->file_open || fsrc->path == NULL) {
		return -EINVAL;
	}

	cap = m && m->pool ? m->pool->config.size : 0;
	if (cap == 0) {
		/* Fall back to net_buf's own size when available */
		cap = buf->size;
	}

	uint32_t to_read = MIN(fsrc->blocksize, cap);

	rd = fs_read(&fsrc->file, buf->data, to_read);
	if (rd < 0) {
		return (int)rd;
	}

	m->bytes_used = (uint32_t)rd;
	buf->len = (uint32_t)rd;

	if (rd == 0) {
		LOG_INF("End of file");
		return -ENODATA;
	}

	return 0;
}

static int mpipe_file_src_pool_acquire_buffer(struct mpipe_buffer_pool *pool, struct net_buf **buf)
{
	struct mpipe_file_src *fsrc = CONTAINER_OF(pool, struct mpipe_file_src, pool);
	struct net_buf *out = NULL;
	struct mpipe_buffer_meta *m;
	int ret;

	__ASSERT_NO_MSG(buf != NULL);

	if (fsrc->downstream_pool != NULL && fsrc->downstream_pool->acquire_buffer != NULL) {
		if (fsrc->downstream_pool->acquire_buffer(fsrc->downstream_pool, &out) != 0 ||
		    out == NULL) {
			return -ENOBUFS;
		}

		m = mpipe_buffer_get_meta(out);
		m->bytes_used = 0;
		m->timestamp = 0;
	} else {
		out = net_buf_alloc_len(&mpipe_fs_nb_pool, CONFIG_MPIPE_FS_BLOCK_SIZE, K_NO_WAIT);
		if (out == NULL) {
			return -ENOBUFS;
		}

		m = mpipe_buffer_get_meta(out);
		m->pool = &fsrc->pool;
		m->bytes_used = 0;
		m->timestamp = 0;
		m->driver_buf = NULL;
		m->priv = NULL;
	}

	ret = mpipe_file_src_read_chunk(fsrc, out);
	if (ret != 0) {
		net_buf_unref(out);
		return ret;
	}

	*buf = out;

	return 0;
}

static int mpipe_file_src_pool_release_buffer(struct mpipe_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	/* Only used for buffers allocated from mpipe_fs_nb_pool (meta->pool = &fsrc->pool). */
	if (buf != NULL) {
		struct mpipe_buffer_meta *m = mpipe_buffer_get_meta(buf);

		if (m != NULL) {
			m->bytes_used = 0;
			m->timestamp = 0;
			m->driver_buf = NULL;
			m->priv = NULL;
		}

		buf->len = 0;
	}

	return 0;
}

static enum mpipe_state_change_return
mpipe_file_src_change_state(struct mpipe_element *self, enum mpipe_state_change transition)
{
	struct mpipe_file_src *fsrc = (struct mpipe_file_src *)self;
	enum mpipe_state_change_return ret;

	/* Reuse base mpipe_src negotiation/pool start behavior */
	ret = mpipe_src_change_state(self, transition);
	if (ret != MPIPE_STATE_CHANGE_SUCCESS) {
		return ret;
	}

	switch (transition) {
	case MPIPE_STATE_CHANGE_READY_TO_PAUSED:
		if (fsrc->path == NULL) {
			LOG_ERR("No file path set");
			return MPIPE_STATE_CHANGE_FAILURE;
		}

		fs_file_t_init(&fsrc->file);
		if (fs_open(&fsrc->file, fsrc->path, FS_O_READ) != 0) {
			LOG_ERR("Failed to open file: %s", fsrc->path);
			return MPIPE_STATE_CHANGE_FAILURE;
		}
		fsrc->file_open = true;
		LOG_DBG("File is open");
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		if (fsrc->file_open) {
			(void)fs_close(&fsrc->file);
			fsrc->file_open = false;
			LOG_DBG("File is closed");
		}
		break;
	default:
		break;
	}

	return MPIPE_STATE_CHANGE_SUCCESS;
}

int mpipe_file_src_init(struct mpipe_file_src *fsrc, uint8_t id)
{
	__ASSERT_NO_MSG(fsrc != NULL);

	struct mpipe_element *self = &fsrc->src.element;
	struct mpipe_src *src = &fsrc->src;
	int ret = mpipe_src_init(src, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "file_src");

	self->object.set_property = mpipe_file_src_set_property;
	self->object.get_property = mpipe_file_src_get_property;
	self->change_state = mpipe_file_src_change_state;

	src->decide_buffer_pool = mpipe_file_src_decide_buffer_pool;
	src->pool = &fsrc->pool;

	mpipe_buffer_pool_init(&fsrc->pool);

	fsrc->downstream_pool = NULL;
	fsrc->path = NULL;
	fsrc->file_open = false;
	fsrc->blocksize = CONFIG_MPIPE_FS_BLOCK_SIZE;
	const struct mpipe_buffer_pool_config pool_req = {.size = CONFIG_MPIPE_FS_BLOCK_SIZE};

	(void)mpipe_buffer_pool_set_req_config(&fsrc->pool, &pool_req);
	fsrc->pool.acquire_buffer = mpipe_file_src_pool_acquire_buffer;
	fsrc->pool.release_buffer = mpipe_file_src_pool_release_buffer;

	return 0;
}
