/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_ctrls, CONFIG_VIDEO_LOG_LEVEL);

static inline const char *const *video_get_std_menu_ctrl(uint32_t id)
{
	static char const *const power_line_frequency[] = {
		"Disabled", "50 Hz", "60 Hz", "Auto", NULL,
	};
	static char const *const exposure_auto[] = {
		"Auto Mode", "Manual Mode", "Shutter Priority Mode", "Aperture Priority Mode", NULL,
	};
	static char const *const colorfx[] = {
		"None", "Black & White", "Sepia", "Negative", "Emboss", "Sketch", "Sky Blue",
		"Grass Green", "Skin Whiten", "Vivid", "Aqua", "Art Freeze", "Silhouette",
		"Solarization", "Antique", "Set Cb/Cr", NULL,
	};
	static char const *const camera_orientation[] = {
		"Front", "Back", "External", NULL,
	};

	switch (id) {
	/* User control menus */
	case VIDEO_CID_POWER_LINE_FREQUENCY:
		return power_line_frequency;

	/* Camera control menus */
	case VIDEO_CID_EXPOSURE_AUTO:
		return exposure_auto;
	case VIDEO_CID_COLORFX:
		return colorfx;
	case VIDEO_CID_CAMERA_ORIENTATION:
		return camera_orientation;
	default:
		return NULL;
	}
}

static inline int check_range(enum video_ctrl_type type, struct video_ctrl_range range)
{
	switch (type) {
	case VIDEO_CTRL_TYPE_BOOLEAN:
		if (range.step != 1 || range.max > 1 || range.min < 0) {
			return -ERANGE;
		}
		return 0;
	case VIDEO_CTRL_TYPE_INTEGER:
		if (range.step == 0 || range.min > range.max ||
		    !IN_RANGE(range.def, range.min, range.max)) {
			return -ERANGE;
		}
		return 0;
	case VIDEO_CTRL_TYPE_INTEGER64:
		if (range.step64 == 0 || range.min64 > range.max64 ||
		    !IN_RANGE(range.def64, range.min64, range.max64)) {
			return -ERANGE;
		}
		return 0;
	case VIDEO_CTRL_TYPE_MENU:
	case VIDEO_CTRL_TYPE_INTEGER_MENU:
		if (!IN_RANGE(range.min, 0, range.max) ||
		    !IN_RANGE(range.def, range.min, range.max)) {
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
	case VIDEO_CID_AUTO_WHITE_BALANCE:
	case VIDEO_CID_AUTOGAIN:
	case VIDEO_CID_HFLIP:
	case VIDEO_CID_VFLIP:
	case VIDEO_CID_HUE_AUTO:
	case VIDEO_CID_AUTOBRIGHTNESS:
	case VIDEO_CID_EXPOSURE_AUTO_PRIORITY:
	case VIDEO_CID_FOCUS_AUTO:
	case VIDEO_CID_WIDE_DYNAMIC_RANGE:
		*type = VIDEO_CTRL_TYPE_BOOLEAN;
		break;
	case VIDEO_CID_POWER_LINE_FREQUENCY:
	case VIDEO_CID_EXPOSURE_AUTO:
	case VIDEO_CID_COLORFX:
	case VIDEO_CID_TEST_PATTERN:
	case VIDEO_CID_CAMERA_ORIENTATION:
		*type = VIDEO_CTRL_TYPE_MENU;
		break;
	case VIDEO_CID_PIXEL_RATE:
		*type = VIDEO_CTRL_TYPE_INTEGER64;
		*flags |= VIDEO_CTRL_FLAG_READ_ONLY;
		break;
	case VIDEO_CID_LINK_FREQ:
		*type = VIDEO_CTRL_TYPE_INTEGER_MENU;
		break;
	default:
		*type = VIDEO_CTRL_TYPE_INTEGER;
		break;
	}
}

int video_init_ctrl(struct video_ctrl *ctrl, const struct device *dev, uint32_t id,
		    struct video_ctrl_range range)
{
	int ret;
	uint32_t flags;
	enum video_ctrl_type type;
	struct video_ctrl *vc;
	struct video_device *vdev;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(ctrl != NULL);

	vdev = video_find_vdev(dev);
	if (!vdev) {
		return -EINVAL;
	}

	/* Sanity checks */
	if (id < VIDEO_CID_BASE) {
		return -EINVAL;
	}

	set_type_flag(id, &type, &flags);

	ret = check_range(type, range);
	if (ret) {
		return ret;
	}

	ctrl->cluster_sz = 0;
	ctrl->cluster = NULL;
	ctrl->is_auto = false;
	ctrl->has_volatiles = false;
	ctrl->menu = NULL;
	ctrl->vdev = vdev;
	ctrl->id = id;
	ctrl->type = type;
	ctrl->flags = flags;
	ctrl->range = range;

	if (type == VIDEO_CTRL_TYPE_INTEGER64) {
		ctrl->val64 = range.def64;
	} else {
		ctrl->val = range.def;
	}

	/* Insert in an ascending order of ctrl's id */
	SYS_DLIST_FOR_EACH_CONTAINER(&vdev->ctrls, vc, node) {
		if (vc->id > ctrl->id) {
			sys_dlist_insert(&vc->node, &ctrl->node);
			return 0;
		}
	}

	sys_dlist_append(&vdev->ctrls, &ctrl->node);

	return 0;
}

int video_init_menu_ctrl(struct video_ctrl *ctrl, const struct device *dev, uint32_t id,
			 uint8_t def, const char *const menu[])
{
	int ret;
	uint8_t sz = 0;
	const char *const *_menu = menu ? menu : video_get_std_menu_ctrl(id);

	if (!_menu) {
		return -EINVAL;
	}

	while (_menu[sz]) {
		sz++;
	}

	ret = video_init_ctrl(
		ctrl, dev, id,
		(struct video_ctrl_range){.min = 0, .max = sz - 1, .step = 1, .def = def});

	if (ret) {
		return ret;
	}

	ctrl->menu = _menu;

	return 0;
}

int video_init_int_menu_ctrl(struct video_ctrl *ctrl, const struct device *dev, uint32_t id,
			     uint8_t def, const int64_t menu[], size_t menu_len)
{
	int ret;

	if (!menu) {
		return -EINVAL;
	}

	ret = video_init_ctrl(
		ctrl, dev, id,
		(struct video_ctrl_range){.min = 0, .max = menu_len - 1, .step = 1, .def = def});

	if (ret) {
		return ret;
	}

	ctrl->int_menu = menu;

	return 0;
}

/* By definition, the cluster is in manual mode if the primary control value is 0 */
static inline bool is_cluster_manual(const struct video_ctrl *primary)
{
	return primary->type == VIDEO_CTRL_TYPE_INTEGER64 ? primary->val64 == 0 : primary->val == 0;
}

void video_cluster_ctrl(struct video_ctrl *ctrls, uint8_t sz)
{
	bool has_volatiles = false;

	__ASSERT(sz && ctrls, "The 1st control, i.e. the primary control, must not be NULL");

	for (uint8_t i = 0; i < sz; i++) {
		ctrls[i].cluster_sz = sz;
		ctrls[i].cluster = ctrls;
		if (ctrls[i].flags & VIDEO_CTRL_FLAG_VOLATILE) {
			has_volatiles = true;
		}
	}

	ctrls->has_volatiles = has_volatiles;
}

void video_auto_cluster_ctrl(struct video_ctrl *ctrls, uint8_t sz, bool set_volatile)
{
	video_cluster_ctrl(ctrls, sz);

	__ASSERT(sz > 1, "Control auto cluster size must be > 1");
	__ASSERT(!(set_volatile && !DEVICE_API_GET(video, ctrls->vdev->dev)->get_volatile_ctrl),
		 "Volatile is set but no ops");

	ctrls->is_auto = true;
	ctrls->has_volatiles = set_volatile;
	ctrls->flags |= VIDEO_CTRL_FLAG_UPDATE;

	/* If the cluster is in automatic mode, mark all manual controls inactive and volatile */
	for (uint8_t i = 1; i < sz; i++) {
		if (!is_cluster_manual(ctrls)) {
			ctrls[i].flags |= VIDEO_CTRL_FLAG_INACTIVE |
					  (set_volatile ? VIDEO_CTRL_FLAG_VOLATILE : 0);
		}
	}
}

static int video_find_ctrl(const struct device *dev, uint32_t id, struct video_ctrl **ctrl)
{
	struct video_device *vdev = video_find_vdev(dev);

	while (vdev) {
		SYS_DLIST_FOR_EACH_CONTAINER(&vdev->ctrls, *ctrl, node) {
			if ((*ctrl)->id == id) {
				return 0;
			}
		}

		vdev = video_find_vdev(vdev->src_dev);
	}

	return -ENOTSUP;
}

int video_get_ctrl(const struct device *dev, struct video_control *control)
{
	struct video_ctrl *ctrl = NULL;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(control != NULL);

	int ret = video_find_ctrl(dev, control->id, &ctrl);

	if (ret) {
		return ret;
	}

	if (ctrl->flags & VIDEO_CTRL_FLAG_WRITE_ONLY) {
		LOG_ERR("Control id 0x%x is write-only\n", control->id);
		return -EACCES;
	}

	if (ctrl->flags & VIDEO_CTRL_FLAG_VOLATILE) {
		if (DEVICE_API_GET(video, ctrl->vdev->dev)->get_volatile_ctrl == NULL) {
			return -ENOSYS;
		}

		/* Call driver's get_volatile_ctrl */
		ret = DEVICE_API_GET(video, ctrl->vdev->dev)
			      ->get_volatile_ctrl(ctrl->vdev->dev,
						  ctrl->cluster ? ctrl->cluster->id : ctrl->id);
		if (ret) {
			return ret;
		}
	}

	/* Give the control's current value to user */
	if (ctrl->type == VIDEO_CTRL_TYPE_INTEGER64) {
		control->val64 = ctrl->val64;
	} else {
		control->val = ctrl->val;
	}

	return 0;
}

int video_set_ctrl(const struct device *dev, struct video_control *control)
{
	struct video_ctrl *ctrl = NULL;
	int ret;
	uint8_t i = 0;
	int32_t val = 0;
	int64_t val64 = 0;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(control != NULL);

	ret = video_find_ctrl(dev, control->id, &ctrl);
	if (ret) {
		return ret;
	}

	if (ctrl->flags & VIDEO_CTRL_FLAG_READ_ONLY) {
		LOG_ERR("Control id 0x%x is read-only\n", control->id);
		return -EACCES;
	}

	if (ctrl->flags & VIDEO_CTRL_FLAG_INACTIVE) {
		LOG_ERR("Control id 0x%x is inactive\n", control->id);
		return -EACCES;
	}

	if (ctrl->type == VIDEO_CTRL_TYPE_INTEGER64
		    ? !IN_RANGE(control->val64, ctrl->range.min64, ctrl->range.max64)
		    : !IN_RANGE(control->val, ctrl->range.min, ctrl->range.max)) {
		LOG_ERR("Control value is invalid\n");
		return -EINVAL;
	}

	/* No new value */
	if (ctrl->type == VIDEO_CTRL_TYPE_INTEGER64 ? ctrl->val64 == control->val64
						    : ctrl->val == control->val) {
		return 0;
	}

	/* Backup the control's value then set it to the new value */
	if (ctrl->type == VIDEO_CTRL_TYPE_INTEGER64) {
		val64 = ctrl->val64;
		ctrl->val64 = control->val64;
	} else {
		val = ctrl->val;
		ctrl->val = control->val;
	}

	/*
	 * For auto-clusters having volatiles, before switching to manual mode, get the current
	 * volatile values since those will become the initial manual values after this switch.
	 */
	if (ctrl == ctrl->cluster && ctrl->is_auto && ctrl->has_volatiles &&
	    is_cluster_manual(ctrl)) {

		if (DEVICE_API_GET(video, ctrl->vdev->dev)->get_volatile_ctrl == NULL) {
			ret = -ENOSYS;
			goto restore;
		}

		ret = DEVICE_API_GET(video, ctrl->vdev->dev)
			      ->get_volatile_ctrl(ctrl->vdev->dev, ctrl->id);
		if (ret) {
			goto restore;
		}
	}

	/* Call driver's set_ctrl */
	if (DEVICE_API_GET(video, ctrl->vdev->dev)->set_ctrl) {
		ret = DEVICE_API_GET(video, ctrl->vdev->dev)
				     ->set_ctrl(ctrl->vdev->dev,
						ctrl->cluster ? ctrl->cluster->id : ctrl->id);
		if (ret) {
			goto restore;
		}
	}

	/* Update the manual controls' flags of the cluster */
	if (ctrl->cluster && ctrl->cluster->is_auto) {
		for (i = 1; i < ctrl->cluster_sz; i++) {
			if (!is_cluster_manual(ctrl->cluster)) {
				/* Automatic mode: set the inactive and volatile flags of the manual
				 * controls
				 */
				ctrl->cluster[i].flags |=
					VIDEO_CTRL_FLAG_INACTIVE |
					(ctrl->cluster->has_volatiles ? VIDEO_CTRL_FLAG_VOLATILE
								      : 0);
			} else {
				/* Manual mode: clear the inactive and volatile flags of the manual
				 * controls
				 */
				ctrl->cluster[i].flags &=
					~(VIDEO_CTRL_FLAG_INACTIVE | VIDEO_CTRL_FLAG_VOLATILE);
			}
		}
	}

	return 0;

restore:
	/* Restore the old control's value */
	if (ctrl->type == VIDEO_CTRL_TYPE_INTEGER64) {
		ctrl->val64 = val64;
	} else {
		ctrl->val = val;
	}

	return ret;
}

static inline const char *video_get_ctrl_name(uint32_t id)
{
	switch (id) {
	/* User controls */
	case VIDEO_CID_BRIGHTNESS:
		return "Brightness";
	case VIDEO_CID_CONTRAST:
		return "Contrast";
	case VIDEO_CID_SATURATION:
		return "Saturation";
	case VIDEO_CID_HUE:
		return "Hue";
	case VIDEO_CID_AUTO_WHITE_BALANCE:
		return "White Balance, Automatic";
	case VIDEO_CID_RED_BALANCE:
		return "Red Balance";
	case VIDEO_CID_BLUE_BALANCE:
		return "Blue Balance";
	case VIDEO_CID_GAMMA:
		return "Gamma";
	case VIDEO_CID_EXPOSURE:
		return "Exposure";
	case VIDEO_CID_AUTOGAIN:
		return "Gain, Automatic";
	case VIDEO_CID_GAIN:
		return "Gain";
	case VIDEO_CID_ANALOGUE_GAIN:
		return "Analogue Gain";
	case VIDEO_CID_HFLIP:
		return "Horizontal Flip";
	case VIDEO_CID_VFLIP:
		return "Vertical Flip";
	case VIDEO_CID_POWER_LINE_FREQUENCY:
		return "Power Line Frequency";
	case VIDEO_CID_HUE_AUTO:
		return "Hue, Automatic";
	case VIDEO_CID_WHITE_BALANCE_TEMPERATURE:
		return "White Balance Temperature";
	case VIDEO_CID_SHARPNESS:
		return "Sharpness";
	case VIDEO_CID_BACKLIGHT_COMPENSATION:
		return "Backlight Compensation";
	case VIDEO_CID_COLORFX:
		return "Color Effects";
	case VIDEO_CID_AUTOBRIGHTNESS:
		return "Brightness, Automatic";
	case VIDEO_CID_BAND_STOP_FILTER:
		return "Band-Stop Filter";
	case VIDEO_CID_ROTATE:
		return "Rotate";
	case VIDEO_CID_ALPHA_COMPONENT:
		return "Alpha Component";

	/* Camera controls */
	case VIDEO_CID_EXPOSURE_AUTO:
		return "Auto Exposure";
	case VIDEO_CID_EXPOSURE_ABSOLUTE:
		return "Exposure Time, Absolute";
	case VIDEO_CID_EXPOSURE_AUTO_PRIORITY:
		return "Exposure, Dynamic Framerate";
	case VIDEO_CID_PAN_RELATIVE:
		return "Pan, Relative";
	case VIDEO_CID_TILT_RELATIVE:
		return "Tilt, Reset";
	case VIDEO_CID_PAN_ABSOLUTE:
		return "Pan, Absolute";
	case VIDEO_CID_TILT_ABSOLUTE:
		return "Tilt, Absolute";
	case VIDEO_CID_FOCUS_ABSOLUTE:
		return "Focus, Absolute";
	case VIDEO_CID_FOCUS_RELATIVE:
		return "Focus, Relative";
	case VIDEO_CID_FOCUS_AUTO:
		return "Focus, Automatic Continuous";
	case VIDEO_CID_ZOOM_ABSOLUTE:
		return "Zoom, Absolute";
	case VIDEO_CID_ZOOM_RELATIVE:
		return "Zoom, Relative";
	case VIDEO_CID_ZOOM_CONTINUOUS:
		return "Zoom, Continuous";
	case VIDEO_CID_IRIS_ABSOLUTE:
		return "Iris, Absolute";
	case VIDEO_CID_IRIS_RELATIVE:
		return "Iris, Relative";
	case VIDEO_CID_WIDE_DYNAMIC_RANGE:
		return "Wide Dynamic Range";
	case VIDEO_CID_PAN_SPEED:
		return "Pan, Speed";
	case VIDEO_CID_TILT_SPEED:
		return "Tilt, Speed";
	case VIDEO_CID_CAMERA_ORIENTATION:
		return "Camera Orientation";
	case VIDEO_CID_CAMERA_SENSOR_ROTATION:
		return "Camera Sensor Rotation";

	/* JPEG encoder controls */
	case VIDEO_CID_JPEG_COMPRESSION_QUALITY:
		return "Compression Quality";

	/* Image processing controls */
	case VIDEO_CID_PIXEL_RATE:
		return "Pixel Rate";
	case VIDEO_CID_TEST_PATTERN:
		return "Test Pattern";
	case VIDEO_CID_LINK_FREQ:
		return "Link Frequency";
	default:
		return NULL;
	}
}

int video_query_ctrl(struct video_ctrl_query *cq)
{
	int ret;
	struct video_device *vdev;
	struct video_ctrl *ctrl = NULL;

	__ASSERT_NO_MSG(cq != NULL);
	__ASSERT_NO_MSG(cq->dev != NULL);

	if (cq->id & VIDEO_CTRL_FLAG_NEXT_CTRL) {
		cq->id &= ~VIDEO_CTRL_FLAG_NEXT_CTRL;
		vdev = video_find_vdev(cq->dev);
		while (vdev != NULL) {
			SYS_DLIST_FOR_EACH_CONTAINER(&vdev->ctrls, ctrl, node) {
				if (ctrl->id > cq->id) {
					goto fill_query;
				}
			}
			cq->id = 0;
			cq->dev = vdev->src_dev;
			vdev = video_find_vdev(cq->dev);
		}
		return -ENOTSUP;
	}

	ret = video_find_ctrl(cq->dev, cq->id, &ctrl);
	if (ret) {
		return ret;
	}

fill_query:
	cq->id = ctrl->id;
	cq->type = ctrl->type;
	cq->flags = ctrl->flags;
	cq->range = ctrl->range;
	if (cq->type == VIDEO_CTRL_TYPE_MENU) {
		cq->menu = ctrl->menu;
	} else if (cq->type == VIDEO_CTRL_TYPE_INTEGER_MENU) {
		cq->int_menu = ctrl->int_menu;
	}
	cq->name = video_get_ctrl_name(cq->id);

	return 0;
}

void video_print_ctrl(const struct video_ctrl_query *const cq)
{
	uint8_t i = 0;
	const char *type = NULL;
	char typebuf[8];

	__ASSERT_NO_MSG(cq != NULL);
	__ASSERT_NO_MSG(cq->dev != NULL);

	/* Get type of the control */
	switch (cq->type) {
	case VIDEO_CTRL_TYPE_BOOLEAN:
		type = "bool";
		break;
	case VIDEO_CTRL_TYPE_INTEGER:
		type = "int";
		break;
	case VIDEO_CTRL_TYPE_INTEGER64:
		type = "int64";
		break;
	case VIDEO_CTRL_TYPE_MENU:
		type = "menu";
		break;
	case VIDEO_CTRL_TYPE_INTEGER_MENU:
		type = "integer menu";
		break;
	case VIDEO_CTRL_TYPE_STRING:
		type = "string";
		break;
	default:
		break;
	}
	snprintf(typebuf, sizeof(typebuf), "(%s)", type);

	/* Get current value of the control */
	struct video_control vc = {.id = cq->id};

	video_get_ctrl(cq->dev, &vc);

	/* Print the control information */
	if (cq->type == VIDEO_CTRL_TYPE_INTEGER64) {
		LOG_INF("%32s 0x%08x %-8s (flags=0x%02x) : min=%lld max=%lld step=%lld "
			"default=%lld value=%lld ",
			cq->name, cq->id, typebuf, cq->flags, cq->range.min64, cq->range.max64,
			cq->range.step64, cq->range.def64, vc.val64);
	} else {
		LOG_INF("%32s 0x%08x %-8s (flags=0x%02x) : min=%d max=%d step=%d default=%d "
			"value=%d ",
			cq->name, cq->id, typebuf, cq->flags, cq->range.min, cq->range.max,
			cq->range.step, cq->range.def, vc.val);
	}

	if (cq->type == VIDEO_CTRL_TYPE_MENU && cq->menu) {
		while (cq->menu[i]) {
			LOG_INF("%*s %u: %s", 32, "", i, cq->menu[i]);
			i++;
		}
	} else if (cq->type == VIDEO_CTRL_TYPE_INTEGER_MENU && cq->int_menu) {
		while (cq->int_menu[i]) {
			LOG_INF("%*s %u: %lld", 12, "", i, cq->int_menu[i]);
			i++;
		}
	}
}
