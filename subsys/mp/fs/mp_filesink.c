/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/mp_buffer.h>
#include <zephyr/mp/mp_caps.h>

#include <zephyr/mp/fs/mp_filesink.h>

LOG_MODULE_REGISTER(mp_filesink, CONFIG_MP_LOG_LEVEL);

static int mp_filesink_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_filesink *fsink = (struct mp_filesink *)obj;

	switch (key) {
	case MP_PROP_FS_SINK_PATH:
		fsink->path = (const char *)val;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_filesink_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_filesink *fsink = (struct mp_filesink *)obj;

	switch (key) {
	case MP_PROP_FS_SINK_PATH:
		*(const char **)val = fsink->path;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_filesink_chainfn(struct mp_pad *pad, struct net_buf *in_buf, struct net_buf **out)
{
	struct mp_filesink *fsink =
		CONTAINER_OF(pad->object.container, struct mp_filesink, sink.element.object);
	uint32_t to_write;
	ssize_t wr;

	*out = NULL;

	if (!fsink->file_open) {
		LOG_ERR("File not opened");
		net_buf_unref(in_buf);
		return -ENOTCONN;
	}

	to_write = mp_buffer_get_meta(in_buf)->bytes_used;
	if (to_write == 0) {
		net_buf_unref(in_buf);
		return 0;
	}

	wr = fs_write(&fsink->file, in_buf->data, to_write);
	if (wr <= 0) {
		LOG_ERR("fs_write failed (%u)", (int)wr);
		net_buf_unref(in_buf);
		return -EIO;
	}

	LOG_DBG("filesink: wrote %d bytes", (int)wr);

	/* Ignore short writes for now; could loop later */
	net_buf_unref(in_buf);

	return 0;
}

static enum mp_state_change_return mp_filesink_change_state(struct mp_element *self,
							     enum mp_state_change transition)
{
	struct mp_filesink *fsink = (struct mp_filesink *)self;
	int ret;

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		if (fsink->path == NULL) {
			LOG_ERR("No file path set");
			return MP_STATE_CHANGE_FAILURE;
		}

		fs_file_t_init(&fsink->file);

		ret = fs_open(&fsink->file, fsink->path, FS_O_CREATE | FS_O_WRITE);
		if (ret != 0) {
			LOG_ERR("Failed to open file: %s (%d)", fsink->path, ret);
			return MP_STATE_CHANGE_FAILURE;
		}
		LOG_INF("Opened file for write: %s", fsink->path);
		fsink->file_open = true;
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		if (fsink->file_open) {
			(void)fs_close(&fsink->file);
			fsink->file_open = false;
		}
		break;
	default:
		break;
	}

	/*
	 * Chain to the base sink change_state, which resets the negotiated pad
	 * caps back to the template caps on PAUSED_TO_READY so a subsequent
	 * re-negotiation starts fresh.
	 */
	return mp_sink_change_state(self, transition);
}

void mp_filesink_init(struct mp_element *self)
{
	struct mp_filesink *fsink = (struct mp_filesink *)self;
	struct mp_sink *sink = &fsink->sink;
	struct mp_caps *sink_caps;

	mp_sink_init(self);

	self->object.set_property = mp_filesink_set_property;
	self->object.get_property = mp_filesink_get_property;
	self->change_state = mp_filesink_change_state;

	sink_caps = mp_caps_new_any();
	mp_sink_update_caps(sink, sink_caps);
	mp_caps_unref(sink_caps);

	sink->sinkpad.chainfn = mp_filesink_chainfn;

	fsink->path = NULL;
	fsink->file_open = false;
}
