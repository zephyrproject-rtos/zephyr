/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_buffer.h>

#include <zephyr/mpipe/fs/mpipe_file_sink.h>

LOG_MODULE_REGISTER(mpipe_file_sink, CONFIG_MPIPE_LOG_LEVEL);

static int mpipe_file_sink_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_file_sink *fsink = (struct mpipe_file_sink *)obj;

	switch (key) {
	case MPIPE_PROP_FS_SINK_PATH:
		fsink->path = (const char *)val;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mpipe_file_sink_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_file_sink *fsink = (struct mpipe_file_sink *)obj;

	switch (key) {
	case MPIPE_PROP_FS_SINK_PATH:
		*(const char **)val = fsink->path;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mpipe_file_sink_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				    struct net_buf **out)
{
	struct mpipe_file_sink *fsink =
		CONTAINER_OF(pad->object.container, struct mpipe_file_sink, sink.element.object);
	uint32_t to_write;
	ssize_t wr;

	*out = NULL;

	if (!fsink->file_open) {
		LOG_ERR("File not opened");
		net_buf_unref(in_buf);
		return -ENOTCONN;
	}

	to_write = mpipe_buffer_get_meta(in_buf)->bytes_used;
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

	LOG_DBG("file_sink: wrote %d bytes", (int)wr);

	/* Ignore short writes for now; could loop later */
	net_buf_unref(in_buf);

	return 0;
}

static enum mpipe_state_change_return
mpipe_file_sink_change_state(struct mpipe_element *self, enum mpipe_state_change transition)
{
	struct mpipe_file_sink *fsink = (struct mpipe_file_sink *)self;
	int ret;

	switch (transition) {
	case MPIPE_STATE_CHANGE_READY_TO_PAUSED:
		if (fsink->path == NULL) {
			LOG_ERR("No file path set");
			return MPIPE_STATE_CHANGE_FAILURE;
		}

		fs_file_t_init(&fsink->file);

		ret = fs_open(&fsink->file, fsink->path, FS_O_CREATE | FS_O_WRITE);
		if (ret != 0) {
			LOG_ERR("Failed to open file: %s (%d)", fsink->path, ret);
			return MPIPE_STATE_CHANGE_FAILURE;
		}
		LOG_INF("Opened file for write: %s", fsink->path);
		fsink->file_open = true;
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
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
	return mpipe_sink_change_state(self, transition);
}

int mpipe_file_sink_init(struct mpipe_file_sink *fsink, uint8_t id)
{
	__ASSERT_NO_MSG(fsink != NULL);

	struct mpipe_element *self = &fsink->sink.element;
	struct mpipe_sink *sink = &fsink->sink;
	int ret = mpipe_sink_init(sink, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "file_sink");

	self->object.set_property = mpipe_file_sink_set_property;
	self->object.get_property = mpipe_file_sink_get_property;
	self->change_state = mpipe_file_sink_change_state;

	sink->sink_pad.chain_fn = mpipe_file_sink_chain_fn;

	fsink->path = NULL;
	fsink->file_open = false;

	return 0;
}
