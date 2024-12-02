/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include "video_async.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_ctrls, CONFIG_VIDEO_LOG_LEVEL);

void video_ctrl_handler_init(struct video_ctrl_handler *hdl, const struct device *dev)
{
	if (!hdl) {
		return;
	}

	hdl->dev = dev;

	sys_slist_init(&hdl->list);
}

void video_ctrl_handler_free(struct video_ctrl_handler *hdl)
{
	if (!hdl) {
		return;
	}

	hdl->dev = NULL;

	while (sys_slist_get(&hdl->list)) {
	}
}

void video_ctrl_handler_add(struct video_ctrl_handler *hdl, struct video_ctrl_handler *added)
{
	if (!hdl || !added || hdl == added || sys_slist_is_empty(&added->list)) {
		return;
	}

	sys_slist_append_list(&hdl->list, sys_slist_peek_head(&added->list),
			      sys_slist_peek_tail(&added->list));
}

void video_ctrl_handler_remove(struct video_ctrl_handler *hdl, struct video_ctrl_handler *removed)
{
	sys_snode_t *node;

	if (!hdl || !removed || sys_slist_is_empty(&hdl->list) ||
	    sys_slist_is_empty(&removed->list)) {
		return;
	}

	SYS_SLIST_FOR_EACH_NODE(&removed->list, node) {
		sys_slist_find_and_remove(&hdl->list, node);
	}
}

static inline int check_range(enum video_ctrl_type type, int32_t min, int32_t max, uint32_t step,
			      uint32_t def)
{
	switch (type) {
	case VIDEO_CTRL_TYPE_BOOLEAN:
		if (step != 1 || max > 1 || min < 0) {
			return -ERANGE;
		}
		return 0;
	case VIDEO_CTRL_TYPE_INTEGER:
	case VIDEO_CTRL_TYPE_INTEGER64:
		if (step == 0 || min > max || def < min || def > max) {
			return -ERANGE;
		}
		return 0;
	case VIDEO_CTRL_TYPE_MENU:
	case VIDEO_CTRL_TYPE_INTEGER_MENU:
		if (min > max || def < min || def > max || min < 0) {
			return -ERANGE;
		}
		return 0;
	default:
		return 0;
	}
}

static inline void set_type_flag(uint32_t id, enum video_ctrl_type *type, uint32_t *flags)
{
	*flags = 0;

	switch (id) {
	case VIDEO_CID_HFLIP:
	case VIDEO_CID_VFLIP:
		*type = VIDEO_CTRL_TYPE_BOOLEAN;
		break;
	case VIDEO_CID_POWER_LINE_FREQUENCY:
	case VIDEO_CID_TEST_PATTERN:
		*type = VIDEO_CTRL_TYPE_MENU;
		break;
	case VIDEO_CID_PIXEL_RATE:
		*type = VIDEO_CTRL_TYPE_INTEGER64;
		*flags |= VIDEO_CTRL_FLAG_READ_ONLY;
		break;
	default:
		*type = VIDEO_CTRL_TYPE_INTEGER;
		break;
	}
}

int video_ctrl_init(struct video_ctrl *ctrl, struct video_ctrl_handler *hdl, uint32_t id,
		    int32_t min, int32_t max, uint32_t step, int32_t def)
{
	int ret;
	enum video_ctrl_type type;
	uint32_t flags;

	set_type_flag(id, &type, &flags);

	/* Sanity checks */
	if (id == 0 || id >= VIDEO_CID_PRIVATE_BASE) {
		return -EINVAL;
	}

	ret = check_range(type, min, max, step, def);
	if (ret) {
		return ret;
	}

	ctrl->handler = hdl;

	ctrl->id = id;
	ctrl->type = type;
	ctrl->flags = flags;
	ctrl->min = min;
	ctrl->max = max;
	ctrl->step = step;
	ctrl->def = def;
	ctrl->val = def;

	sys_slist_append(&hdl->list, &ctrl->node);

	return 0;
}

struct video_ctrl *video_ctrl_find(struct video_ctrl_handler *hdl, uint32_t id)
{
	struct video_ctrl *ctrl;

	SYS_SLIST_FOR_EACH_CONTAINER(&hdl->list, ctrl, node) {
		if (ctrl->id == id) {
			return ctrl;
		}
	}

	return NULL;
}

static int video_ctrl_find_by_root_dev(const struct device *dev, uint32_t id,
				       struct video_ctrl **ctrl)
{
	struct video_device *vdev = video_find_vdev(dev);

	if (!vdev) {
		return -EINVAL;
	}

	struct video_ctrl_handler *hdl = &vdev->ctrl_handler;

	if (!hdl) {
		LOG_ERR("Control handler not found\n");
		return -ENOTSUP;
	}

	*ctrl = video_ctrl_find(hdl, id);
	if (!*ctrl) {
		LOG_ERR("Control id 0x%x not found\n", id);
		return -ENOTSUP;
	}

	return 0;
}

int video_get_ctrl(const struct device *dev, struct video_control *control)
{
	struct video_ctrl *ctrl;

	int ret = video_ctrl_find_by_root_dev(dev, control->id, &ctrl);

	if (ret) {
		return ret;
	}

	if (ctrl->flags & VIDEO_CTRL_FLAG_WRITE_ONLY) {
		LOG_ERR("Control id 0x%x is write-only\n", control->id);
		return -EACCES;
	}

	control->val = ctrl->val;

	return 0;
}

int video_set_ctrl(const struct device *dev, struct video_control *control)
{
	struct video_ctrl *ctrl;

	int ret = video_ctrl_find_by_root_dev(dev, control->id, &ctrl);

	if (ret) {
		return ret;
	}

	if (ctrl->flags & VIDEO_CTRL_FLAG_READ_ONLY) {
		LOG_ERR("Control id 0x%x is read-only\n", control->id);
		return -EACCES;
	}

	if (control->val < ctrl->min || control->val > ctrl->max) {
		LOG_ERR("Control value is invalid\n");
		return -EINVAL;
	}

	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_ctrl == NULL) {
		return -ENOSYS;
	}

	ret = api->set_ctrl(dev, control);
	if (ret) {
		return ret;
	}

	/* Only update the ctrl in memory once everything is OK */
	ctrl->val = control->val;

	return 0;
}
