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

#include <zephyr/mp/mp_buffer.h>
#include <zephyr/mp/mp_caps.h>
#include <zephyr/mp/mp_dispatch.h>

#include <zephyr/mp/fs/mp_filesrc.h>

LOG_MODULE_REGISTER(mp_filesrc, CONFIG_MP_LOG_LEVEL);

NET_BUF_POOL_FIXED_DEFINE(mp_fs_nb_pool, CONFIG_MP_FS_NUM_BUFS,
			  CONFIG_MP_FS_BLOCK_SIZE, sizeof(struct mp_buffer_meta),
			  mp_buffer_destroy);

static int mp_filesrc_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_filesrc *fsrc = (struct mp_filesrc *)obj;
	int ret;

	switch (key) {
	case MP_PROP_FS_SRC_PATH:
		fsrc->path = (const char *)val;
		return 0;
	case MP_PROP_FS_SRC_BLOCKSIZE:
		if (val == NULL) {
			return -EINVAL;
		}
		fsrc->blocksize = *(const uint32_t *)val;
		return 0;
	default:
		ret = mp_src_set_property(obj, key, val);
		return ret;
	}
}

static int mp_filesrc_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_filesrc *fsrc = (struct mp_filesrc *)obj;
	int ret;

	switch (key) {
	case MP_PROP_FS_SRC_PATH:
		*(const char **)val = fsrc->path;
		return 0;
	case MP_PROP_FS_SRC_BLOCKSIZE:
		*(uint32_t *)val = fsrc->blocksize;
		return 0;
	default:
		ret = mp_src_get_property(obj, key, val);
		return ret;
	}
}

static int mp_filesrc_decide_allocation(struct mp_src *src, struct mp_dispatch *query)
{
	struct mp_filesrc *fsrc = (struct mp_filesrc *)src;

	fsrc->downstream_pool = mp_dispatch_get_pool(query);

	return 0;
}

static int mp_filesrc_read_chunk(struct mp_filesrc *fsrc, struct net_buf *buf)
{
	struct mp_buffer_meta *m = mp_buffer_get_meta(buf);
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

static int mp_filesrc_pool_acquire_buffer(struct mp_buffer_pool *pool, struct net_buf **buf)
{
	struct mp_filesrc *fsrc = CONTAINER_OF(pool, struct mp_filesrc, pool);
	struct net_buf *out = NULL;
	struct mp_buffer_meta *m;
	int ret;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (fsrc->downstream_pool != NULL && fsrc->downstream_pool->acquire_buffer != NULL) {
		if (fsrc->downstream_pool->acquire_buffer(fsrc->downstream_pool, &out) != 0 ||
		    out == NULL) {
			return -ENOBUFS;
		}

		m = mp_buffer_get_meta(out);
		m->bytes_used = 0;
		m->timestamp = 0;
	} else {
		out = net_buf_alloc_len(&mp_fs_nb_pool, CONFIG_MP_FS_BLOCK_SIZE,
					K_NO_WAIT);
		if (out == NULL) {
			return -ENOBUFS;
		}

		m = mp_buffer_get_meta(out);
		m->pool = &fsrc->pool;
		m->bytes_used = 0;
		m->timestamp = 0;
		m->driver_buf = NULL;
		m->priv = NULL;
	}

	ret = mp_filesrc_read_chunk(fsrc, out);
	if (ret != 0) {
		net_buf_unref(out);
		return ret;
	}

	*buf = out;

	return 0;
}

static int mp_filesrc_pool_release_buffer(struct mp_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	/* Only used for buffers allocated from mp_fs_nb_pool (meta->pool = &fsrc->pool). */
	if (buf != NULL) {
		struct mp_buffer_meta *m = mp_buffer_get_meta(buf);

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

static enum mp_state_change_return mp_filesrc_change_state(struct mp_element *self,
							    enum mp_state_change transition)
{
	struct mp_filesrc *fsrc = (struct mp_filesrc *)self;
	enum mp_state_change_return ret;

	/* Reuse base mp_src negotiation/pool start behavior */
	ret = mp_src_change_state(self, transition);
	if (ret != MP_STATE_CHANGE_SUCCESS) {
		return ret;
	}

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		if (fsrc->path == NULL) {
			LOG_ERR("No file path set");
			return MP_STATE_CHANGE_FAILURE;
		}

		fs_file_t_init(&fsrc->file);
		if (fs_open(&fsrc->file, fsrc->path, FS_O_READ) != 0) {
			LOG_ERR("Failed to open file: %s", fsrc->path);
			return MP_STATE_CHANGE_FAILURE;
		}
		fsrc->file_open = true;
		LOG_DBG("File is open");
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		if (fsrc->file_open) {
			(void)fs_close(&fsrc->file);
			fsrc->file_open = false;
			LOG_DBG("File is closed");
		}
		break;
	default:
		break;
	}

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_filesrc_init(struct mp_element *self)
{
	struct mp_filesrc *fsrc = (struct mp_filesrc *)self;
	struct mp_src *src = &fsrc->src;
	struct mp_caps *src_caps;

	mp_src_init(self);

	self->object.set_property = mp_filesrc_set_property;
	self->object.get_property = mp_filesrc_get_property;
	self->change_state = mp_filesrc_change_state;

	src_caps = mp_caps_new_any();
	mp_src_update_caps(src, src_caps);
	mp_caps_unref(src_caps);

	src->decide_allocation = mp_filesrc_decide_allocation;
	src->pool = &fsrc->pool;

	mp_buffer_pool_init(&fsrc->pool);

	fsrc->downstream_pool = NULL;
	fsrc->path = NULL;
	fsrc->file_open = false;
	fsrc->blocksize = CONFIG_MP_FS_BLOCK_SIZE;
	fsrc->pool.config.size = CONFIG_MP_FS_BLOCK_SIZE;
	fsrc->pool.acquire_buffer = mp_filesrc_pool_acquire_buffer;
	fsrc->pool.release_buffer = mp_filesrc_pool_release_buffer;
}
