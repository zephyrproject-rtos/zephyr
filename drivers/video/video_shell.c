/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/shell/shell.h>

#include "video_ctrls.h"

#define VIDEO_FRMIVAL_FPS(frmival)  DIV_ROUND_CLOSEST((frmival)->denominator, (frmival)->numerator)
#define VIDEO_FRMIVAL_MSEC(frmival) (MSEC_PER_SEC * (frmival)->numerator / (frmival)->denominator)

/* Helper to allow completion of sub-command that depend on previous selection */
#define VIDEO_SHELL_COMPLETE_DEFINE(n, name)                                                       \
	static void complete_##name##n(size_t idx, struct shell_static_entry *entry)               \
	{                                                                                          \
		complete_##name(idx, entry, n);                                                    \
	}                                                                                          \
	SHELL_DYNAMIC_CMD_CREATE(dsub_##name##n, complete_##name##n)

/* Current video device, used for tab completion */
const struct device *video_shell_dev;

/* Current video control, used for tab completion */
struct video_ctrl_query video_shell_cq;

/* Current video endpoint, used for tab completion */
enum video_endpoint_id video_shell_ep = VIDEO_EP_NONE;

static bool device_is_video_and_ready(const struct device *dev)
{
	return device_is_ready(dev) && DEVICE_API_IS(video, dev);
}

static int video_shell_check_device(const struct shell *sh, const struct device *dev)
{
	if (dev == NULL) {
		shell_error(sh, "Could not find a video device with that name");
		return -ENODEV;
	}

	if (!DEVICE_API_IS(video, dev)) {
		shell_error(sh, "%s is not a video device", dev->name);
		return -EINVAL;
	}

	if (!device_is_ready(dev)) {
		shell_error(sh, "Device %s not ready", dev->name);
		return -ENODEV;
	}

	return 0;
}

static const struct device *video_shell_get_dev_by_num(int num)
{
	const struct device *dev_list;
	size_t n = z_device_get_all_static(&dev_list);

	for (size_t i = 0; i < n; i++) {
		if (!DEVICE_API_IS(video, (&dev_list[i]))) {
			continue;
		}
		if (num == 0) {
			return &dev_list[i];
		}
		num--;
	}

	return NULL;
}

static int cmd_video_start(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	ret = video_shell_check_device(sh, dev);
	if (ret != 0) {
		return ret;
	}

	ret = video_stream_start(dev);
	if (ret != 0) {
		shell_error(sh, "Failed to start %s", dev->name);
		return ret;
	}

	shell_print(sh, "Started %s video stream", dev->name);
	return 0;
}

static int cmd_video_stop(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	ret = video_shell_check_device(sh, dev);
	if (ret != 0) {
		return ret;
	}

	ret = video_stream_stop(dev);
	if (ret != 0) {
		shell_error(sh, "Failed to stop %s", dev->name);
		return ret;
	}

	shell_print(sh, "Stopped %s video stream", dev->name);
	return 0;
}

static int cmd_video_capture(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct video_format fmt = {0};
	struct video_buffer *buffers[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX] = {NULL};
	struct video_buffer *vbuf = NULL;
	char *arg2 = argv[2];
	uint32_t first_uptime;
	uint32_t prev_uptime;
	uint32_t this_uptime;
	uint32_t frmival_msec;
	uint32_t frmrate_fps;
	size_t buf_size;
	unsigned long num_frames;
	int ret;

	dev = device_get_binding(argv[1]);
	ret = video_shell_check_device(sh, dev);
	if (ret != 0) {
		return ret;
	}

	ret = video_get_format(dev, VIDEO_EP_OUT, &fmt);
	if (ret != 0) {
		shell_error(sh, "Failed to get the current format interval");
		return ret;
	}

	num_frames = strtoull(arg2, &arg2, 10);
	if (*arg2 != '\0') {
		shell_error(sh, "Invalid integer '%s' for this type", argv[1]);
		return -EINVAL;
	}

	buf_size = MIN(fmt.pitch * fmt.height, CONFIG_VIDEO_BUFFER_POOL_SZ_MAX);

	shell_print(sh, "Preparing %u buffers of %u bytes each",
		    CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, buf_size);

	for (int i = 0; i < ARRAY_SIZE(buffers); i++) {
		buffers[i] = video_buffer_alloc(buf_size, K_NO_WAIT);
		if (buffers[i] == NULL) {
			shell_error(sh, "Failed to allocate buffer %u", i);
			goto end;
		}

		ret = video_enqueue(dev, VIDEO_EP_OUT, buffers[i]);
		if (ret != 0) {
			shell_error(sh, "Failed to enqueue buffer %u: %s", i, strerror(-ret));
			goto end;
		}
	}

	shell_print(sh, "Starting the capture of %u frames from %s", num_frames, dev->name);

	ret = video_stream_start(dev);
	if (ret != 0) {
		shell_error(sh, "Failed to start %s", dev->name);
		goto end;
	}

	first_uptime = prev_uptime = k_uptime_get_32();

	for (unsigned long i = 0; i < num_frames;) {
		ret = video_dequeue(dev, VIDEO_EP_OUT, &vbuf, K_FOREVER);
		if (ret != 0) {
			shell_error(sh, "Failed to dequeue this buffer: %s", strerror(-ret));
			goto end;
		}

		this_uptime = k_uptime_get_32();
		frmival_msec = MSEC_PER_SEC *
			       (this_uptime - prev_uptime) / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
		frmrate_fps = frmival_msec == 0 ? UINT32_MAX : MSEC_PER_SEC / frmival_msec;
		prev_uptime = this_uptime;

		shell_print(sh, "Frame %3u/%u, Bytes %u-%u/%u, Lines %u-%u/%u, %ums %u FPS",
			    i, num_frames, vbuf->line_offset * fmt.pitch,
			    vbuf->line_offset * fmt.pitch + vbuf->bytesused, vbuf->size,
			    vbuf->line_offset,
			    vbuf->line_offset + vbuf->bytesused / fmt.pitch, fmt.height,
			    frmival_msec, frmrate_fps);

		/* Only increment the frame counter on the beginning of a new frame */
		i += (vbuf->line_offset == 0);

		ret = video_enqueue(dev, VIDEO_EP_OUT, vbuf);
		if (ret != 0) {
			shell_error(sh, "Failed to enqueue this buffer: %s", strerror(-ret));
			goto end;
		}
	}

	frmival_msec = MSEC_PER_SEC * (first_uptime - prev_uptime) /
		       CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	frmrate_fps = MSEC_PER_SEC / frmival_msec / num_frames;

	shell_print(sh, "Capture of %u frames in %ums in total, %u FPS on average, stopping %s",
		    num_frames, frmival_msec, frmrate_fps, dev->name);

end:
	video_stream_stop(dev);

	if (vbuf != NULL) {
		video_buffer_release(vbuf);
	}

	while (video_dequeue(dev, VIDEO_EP_OUT, &vbuf, K_NO_WAIT) == 0) {
		video_buffer_release(vbuf);
	}

	return ret;
}

static void complete_video_dev(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev;

	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
	entry->syntax = NULL;

	dev = shell_device_filter(idx, device_is_video_and_ready);
	if (dev == NULL) {
		return;
	}

	entry->syntax = dev->name;
}
SHELL_DYNAMIC_CMD_CREATE(dsub_video_dev, complete_video_dev);

/* Video frame interval handling */

static int video_shell_print_frmival(const struct shell *sh, const struct device *dev,
				     enum video_endpoint_id ep, uint32_t pixfmt, uint32_t width,
				     uint32_t height)
{
	struct video_format fmt = {.pixelformat = pixfmt, .width = width, .height = height};
	struct video_frmival_enum fie = {.format = &fmt};

	for (fie.index = 0; video_enum_frmival(dev, ep, &fie) == 0; fie.index++) {

		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			shell_print(sh, "\t\t\tInterval: "
				    "Discrete %ums (%u FPS)",
				    VIDEO_FRMIVAL_MSEC(&fie.discrete),
				    VIDEO_FRMIVAL_FPS(&fie.discrete));
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			shell_print(sh, "\t\t\tInterval: "
				    "Stepwise: %ums - %ums with step %ums (%u - %u FPS)",
				    VIDEO_FRMIVAL_MSEC(&fie.stepwise.min),
				    VIDEO_FRMIVAL_MSEC(&fie.stepwise.max),
				    VIDEO_FRMIVAL_MSEC(&fie.stepwise.step),
				    VIDEO_FRMIVAL_FPS(&fie.stepwise.max),
				    VIDEO_FRMIVAL_FPS(&fie.stepwise.min));
			break;
		default:
			shell_error(sh, "Invalid type 0x%x", fie.type);
			break;
		}
	}

	if (fie.index == 0) {
		shell_warn(sh, "No frame interval supported for this format for output endpoint");
	}

	return 0;
}

static int video_shell_set_frmival(const struct shell *sh, const struct device *dev,
				   enum video_endpoint_id ep, size_t argc, char **argv)
{
	struct video_frmival frmival = {0};
	char *arg0 = argv[0];
	unsigned long val;
	int ret;

	val = strtoul(arg0, &arg0, 10);
	if (strcmp(arg0, "fps") == 0) {
		frmival.numerator = 1;
		frmival.denominator = val;
	} else if (strcmp(arg0, "ms") == 0) {
		frmival.numerator = val;
		frmival.denominator = MSEC_PER_SEC;
	} else if (strcmp(arg0, "us") == 0) {
		frmival.numerator = val;
		frmival.denominator = USEC_PER_SEC;
	} else {
		shell_error(sh, "Expected <n>ms, <n>us or <n>fps, not '%s'", argv[0]);
		return -EINVAL;
	}

	ret = video_set_frmival(dev, ep, &frmival);
	if (ret != 0) {
		shell_error(sh, "Failed to set frame interval");
		return ret;
	}

	shell_print(sh, "New frame interval:     %ums (%u FPS)",
		    VIDEO_FRMIVAL_MSEC(&frmival), VIDEO_FRMIVAL_FPS(&frmival));

	return 0;
}

static int cmd_video_frmival(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct video_format fmt = {0};
	struct video_frmival frmival = {0};
	enum video_endpoint_id ep;
	int ret;

	dev = device_get_binding(argv[1]);
	ret = video_shell_check_device(sh, dev);
	if (ret != 0) {
		return ret;
	}

	if (strcmp(argv[2], "in") == 0) {
		ep = VIDEO_EP_IN;
	} else if (strcmp(argv[2], "out") == 0) {
		ep = VIDEO_EP_OUT;
	} else {
		shell_error(sh, "Endpoint direction must be 'in' or 'out', not '%s'", argv[2]);
		return -EINVAL;
	}

	ret = video_get_frmival(dev, ep, &frmival);
	if (ret != 0) {
		shell_error(sh, "Failed to get the current frame interval");
		return ret;
	}

	ret = video_get_format(dev, ep, &fmt);
	if (ret != 0) {
		shell_error(sh, "Failed to get the current format interval");
		return ret;
	}

	shell_print(sh, "Current frame interval: %ums (%u FPS)",
		    VIDEO_FRMIVAL_MSEC(&frmival), VIDEO_FRMIVAL_FPS(&frmival));

	switch (argc) {
	case 3:
		return video_shell_print_frmival(sh, dev, ep, fmt.pixelformat, fmt.width,
						 fmt.height);
	case 4:
		return video_shell_set_frmival(sh, dev, ep, argc - 3, &argv[3]);
	default:
		shell_error(sh, "Wrong parameter count");
		return -EINVAL;
	}
}

static void complete_video_frmival_dir(size_t idx, struct shell_static_entry *entry)
{
	static const char *const syntax[] = {"in", "out"};

	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
	entry->syntax = NULL;

	if (idx >= ARRAY_SIZE(syntax)) {
		return;
	}

	entry->syntax = syntax[idx];
}
SHELL_DYNAMIC_CMD_CREATE(dsub_video_frmival_dir, complete_video_frmival_dir);

static void complete_video_frmival_dev(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev;

	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
	entry->syntax = NULL;

	dev = shell_device_filter(idx, device_is_video_and_ready);
	if (dev == NULL) {
		return;
	}

	entry->syntax = dev->name;
	entry->subcmd = &dsub_video_frmival_dir;
}
SHELL_DYNAMIC_CMD_CREATE(dsub_video_frmival_dev, complete_video_frmival_dev);

/* Video format handling */

static int video_shell_print_format_cap(const struct shell *sh, const struct device *dev,
					enum video_endpoint_id ep,
					const struct video_format_cap *cap, size_t i)
{
	size_t bpp = video_bits_per_pixel(cap->pixelformat);
	size_t size_max = cap->width_max * cap->height_max * bpp / BITS_PER_BYTE;
	size_t size_min = cap->width_min * cap->height_min * bpp / BITS_PER_BYTE;
	int ret;

	shell_print(sh, "\t[%u]: '%s' (%u bits per pixel)",
		    i, VIDEO_FOURCC_TO_STR(cap->pixelformat), bpp);

	if (cap->width_min == cap->width_max && cap->height_min == cap->height_max) {
		shell_print(sh, "\t\tSize: "
			    "Discrete: %ux%u (%u bytes per frame)",
			    cap->width_max, cap->height_max, size_min);

		ret = video_shell_print_frmival(sh, dev, ep, cap->pixelformat, cap->width_min,
						cap->height_min);
		if (ret != 0) {
			return ret;
		}
	} else {
		shell_print(sh, "\t\tSize: "
			    "Stepwise: %ux%u - %ux%u with step %ux%u (%u - %u bytes per frame)",
			    cap->width_min, cap->height_min, cap->width_max, cap->height_max,
			    cap->width_step, cap->height_step, size_min, size_max);

		ret = video_shell_print_frmival(sh, dev, ep, cap->pixelformat, cap->width_min,
						cap->height_min);
		if (ret != 0) {
			return ret;
		}

		ret = video_shell_print_frmival(sh, dev, ep, cap->pixelformat, cap->width_max,
						cap->height_max);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int video_shell_print_caps(const struct shell *sh, const struct device *dev,
				  enum video_endpoint_id ep, struct video_caps *caps)
{
	int ret;

	shell_print(sh, "min vbuf count: %u", caps->min_vbuf_count);
	shell_print(sh, "min line count: %u", caps->min_line_count);
	shell_print(sh, "max line count: %u", caps->max_line_count);

	for (size_t i = 0; caps->format_caps[i].pixelformat != 0; i++) {
		ret = video_shell_print_format_cap(sh, dev, ep, &caps->format_caps[i], i);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int video_shell_set_format(const struct shell *sh, const struct device *dev,
				  enum video_endpoint_id ep, size_t argc, char **argv)
{
	struct video_format fmt = {0};
	char *arg1 = argv[1];
	int ret;

	if (strlen(argv[0]) != 4) {
		shell_error(sh, "Invalid four character code: '%s'", argv[0]);
		return -EINVAL;
	}

	fmt.pixelformat = VIDEO_FOURCC_FROM_STR(argv[0]);

	fmt.width = strtoul(arg1, &arg1, 10);
	if (*arg1 != 'x' || fmt.width == 0) {
		shell_error(sh, "Invalid width in <width>x<height> parameter %s", argv[1]);
		return -EINVAL;
	}
	arg1++;

	fmt.height = strtoul(arg1, &arg1, 10);
	if (*arg1 != '\0' || fmt.height == 0) {
		shell_error(sh, "Invalid height in <width>x<height> parameter %s (%s)",
			    argv[1], arg1);
		return -EINVAL;
	}

	fmt.pitch = fmt.width * video_bits_per_pixel(fmt.pixelformat) / BITS_PER_BYTE;

	ret = video_set_format(dev, ep, &fmt);
	if (ret != 0) {
		shell_error(sh, "Failed to set the format to '%s' %ux%u on output endpoint",
			    VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height);
		return ret;
	}

	shell_print(sh, "New format:     '%s' %ux%u (%u bytes per frame)",
		    VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height,
		    fmt.pitch * fmt.height);

	return 0;
}

static int cmd_video_format(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct video_caps caps = {0};
	struct video_format fmt = {0};
	enum video_endpoint_id ep;
	int ret;

	dev = device_get_binding(argv[1]);
	ret = video_shell_check_device(sh, dev);
	if (ret != 0) {
		return ret;
	}

	if (strcmp(argv[2], "in") == 0) {
		ep = VIDEO_EP_IN;
	} else if (strcmp(argv[2], "out") == 0) {
		ep = VIDEO_EP_OUT;
	} else {
		shell_error(sh, "Endpoint direction must be 'in' or 'out', not '%s'", argv[2]);
		return -EINVAL;
	}

	ret = video_get_caps(dev, ep, &caps);
	if (ret != 0) {
		shell_error(sh, "Failed to query %s capabilities", dev->name);
		return -ENODEV;
	}

	ret = video_get_format(dev, ep, &fmt);
	if (ret != 0) {
		shell_error(sh, "Failed to query %s capabilities", dev->name);
		return -ENODEV;
	}

	shell_print(sh, "Current format: '%s' %ux%u (%u bytes per frame)",
		    VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height,
		    fmt.pitch * fmt.height);

	switch (argc) {
	case 3:
		return video_shell_print_caps(sh, dev, ep, &caps);
	case 5:
		return video_shell_set_format(sh, dev, ep, argc - 3, &argv[3]);
	default:
		shell_error(sh, "Wrong parameter count");
		return -EINVAL;
	}
}

/* Video control handling */

static const char *video_shell_get_ctrl_name(uint32_t id)
{
	switch (id) {
	/* User controls */
	case VIDEO_CID_BRIGHTNESS:
		return "brightness";
	case VIDEO_CID_CONTRAST:
		return "contrast";
	case VIDEO_CID_SATURATION:
		return "saturation";
	case VIDEO_CID_HUE:
		return "hue";
	case VIDEO_CID_EXPOSURE:
		return "exposure";
	case VIDEO_CID_AUTOGAIN:
		return "gain_automatic";
	case VIDEO_CID_GAIN:
		return "gain";
	case VIDEO_CID_ANALOGUE_GAIN:
		return "analogue_gain";
	case VIDEO_CID_HFLIP:
		return "horizontal_flip";
	case VIDEO_CID_VFLIP:
		return "vertical_flip";
	case VIDEO_CID_POWER_LINE_FREQUENCY:
		return "power_line_frequency";

	/* Camera controls */
	case VIDEO_CID_ZOOM_ABSOLUTE:
		return "zoom_absolute";

	/* JPEG encoder controls */
	case VIDEO_CID_JPEG_COMPRESSION_QUALITY:
		return "compression_quality";

	/* Image processing controls */
	case VIDEO_CID_PIXEL_RATE:
		return "pixel_rate";
	case VIDEO_CID_TEST_PATTERN:
		return "test_pattern";
	default:
		return NULL;
	}
}

static int video_shell_get_ctrl_by_name(const struct device *dev, struct video_ctrl_query *cq,
					char const *name)
{
	int ret;

	cq->id = 0;
	for (size_t i = 0;; i++) {
		cq->id |= VIDEO_CTRL_FLAG_NEXT_CTRL;

		ret = video_query_ctrl(dev, cq);
		if (ret != 0) {
			return -ENOENT;
		}

		if (strcmp(video_shell_get_ctrl_name(cq->id), name) == 0) {
			break;
		}
	}

	return 0;
}

static int video_shell_get_ctrl_by_num(const struct device *dev, struct video_ctrl_query *cq,
				       int num)
{
	int ret;

	cq->id = 0;
	for (size_t i = 0; i <= num; i++) {
		cq->id |= VIDEO_CTRL_FLAG_NEXT_CTRL;

		ret = video_query_ctrl(dev, cq);
		if (ret != 0) {
			return -ENOENT;
		}
	}

	return 0;
}

static int video_shell_set_ctrl(const struct shell *sh, const struct device *dev, size_t argc,
				char **argv)
{
	struct video_ctrl_query cq = {0};
	struct video_control ctrl;
	char *arg0 = argv[0];
	size_t i;
	int ret;

	ret = video_shell_get_ctrl_by_name(dev, &cq, argv[0]);
	if (ret != 0) {
		shell_error(sh, "Video control '%s' not found for this device", argv[0]);
		return ret;
	}

	ctrl.id = cq.id;

	switch (cq.type) {
	case VIDEO_CTRL_TYPE_BOOLEAN:
		if (strcmp(argv[1], "on") == 0) {
			ctrl.val64 = true;
		} else if (strcmp(argv[1], "off") == 0) {
			ctrl.val64 = false;
		} else {
			shell_error(sh, "Video control value must be 'on' or 'off', not '%s'",
				    argv[1]);
			return -EINVAL;
		}
		break;
	case VIDEO_CTRL_TYPE_MENU:
		for (i = 0; cq.menu[i] != NULL; i++) {
			if (strcmp(cq.menu[i], argv[1]) == 0) {
				ctrl.val64 = i;
			}
		}
		if (cq.menu[i] != NULL) {
			shell_error(sh, "Video control value '%s' is not present in the menu",
				    argv[1]);
			return -EINVAL;
		}
		break;
	default:
		ctrl.val64 = strtoull(arg0, &arg0, 10);
		if (*arg0 != '\0') {
			shell_error(sh, "Invalid integer '%s' for this type", argv[1]);
			return -EINVAL;
		}
	}

	shell_print(sh, "Setting control '%s' (0x%08x) to value '%s' (%llu)",
		    argv[0], ctrl.id, argv[1], ctrl.val64);

	ret = video_set_ctrl(dev, &ctrl);
	if (ret != 0) {
		shell_error(sh, "Failed to set control 0x%08x to value %ull", ctrl.id, ctrl.val64);
		return ret;
	}

	return 0;
}

static int video_shell_print_ctrls(const struct shell *sh, const struct device *dev)
{
	struct video_ctrl_query cq = {.id = VIDEO_CTRL_FLAG_NEXT_CTRL};

	while (video_query_ctrl(dev, &cq) == 0) {
		video_print_ctrl(dev, &cq);
		cq.id |= VIDEO_CTRL_FLAG_NEXT_CTRL;
	}

	return 0;
}

static int cmd_video_ctrl(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	ret = video_shell_check_device(sh, dev);
	if (ret != 0) {
		return ret;
	}

	switch (argc) {
	case 2:
		return video_shell_print_ctrls(sh, dev);
	case 4:
		return video_shell_set_ctrl(sh, dev, argc - 2, &argv[2]);
	default:
		shell_error(sh, "Wrong parameter count");
		return -EINVAL;
	}

	return 0;
}

static void complete_video_ctrl_boolean(size_t idx, struct shell_static_entry *entry)
{
	static const char *const syntax[] = {"on", "off"};

	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
	entry->syntax = NULL;

	if (idx >= ARRAY_SIZE(syntax)) {
		return;
	}

	entry->syntax = syntax[idx];
}
SHELL_DYNAMIC_CMD_CREATE(dsub_video_ctrl_boolean, complete_video_ctrl_boolean);

static void complete_video_ctrl_menu_name(size_t idx, struct shell_static_entry *entry, int ctrln)
{
	const struct device *dev = video_shell_dev;
	struct video_ctrl_query cq = {0};
	int ret;

	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
	entry->syntax = NULL;

	dev = video_shell_dev;
	if (!device_is_ready(dev)) {
		return;
	}

	/* Check which control name was selected */

	ret = video_shell_get_ctrl_by_num(dev, &cq, ctrln);
	if (ret != 0) {
		return;
	}

	/* Then complete the menu value */

	for (size_t i = 0; cq.menu[i] != NULL; i++) {
		if (i == idx) {
			entry->syntax = cq.menu[i];
			break;
		}
	}

}
LISTIFY(10, VIDEO_SHELL_COMPLETE_DEFINE, (;), video_ctrl_menu_name);

static const union shell_cmd_entry *dsub_video_ctrl_menu_name[] = {
	&dsub_video_ctrl_menu_name0, &dsub_video_ctrl_menu_name1,
	&dsub_video_ctrl_menu_name2, &dsub_video_ctrl_menu_name3,
	&dsub_video_ctrl_menu_name4, &dsub_video_ctrl_menu_name5,
	&dsub_video_ctrl_menu_name6, &dsub_video_ctrl_menu_name7,
	&dsub_video_ctrl_menu_name8, &dsub_video_ctrl_menu_name9,
};

static void complete_video_ctrl_name_dev(size_t idx, struct shell_static_entry *entry, int devn)
{
	const struct device *dev;
	struct video_ctrl_query *cq = &video_shell_cq;
	int ret;

	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
	entry->syntax = NULL;

	/* Check which device was selected */

	dev = video_shell_dev = video_shell_get_dev_by_num(devn);
	if (!device_is_ready(dev)) {
		return;
	}

	/* Then complete the control name */

	ret = video_shell_get_ctrl_by_num(dev, &video_shell_cq, idx);
	if (ret != 0) {
		return;
	}

	entry->syntax = video_shell_get_ctrl_name(cq->id);

	switch (cq->type) {
	case VIDEO_CTRL_TYPE_BOOLEAN:
		entry->subcmd = &dsub_video_ctrl_boolean;
		break;
	case VIDEO_CTRL_TYPE_MENU:
		if (idx >= ARRAY_SIZE(dsub_video_ctrl_menu_name)) {
			return;
		}
		entry->subcmd = dsub_video_ctrl_menu_name[idx];
		break;
	}
}
LISTIFY(10, VIDEO_SHELL_COMPLETE_DEFINE, (;), video_ctrl_name_dev);

static const union shell_cmd_entry *dsub_video_ctrl_name_dev[] = {
	&dsub_video_ctrl_name_dev0, &dsub_video_ctrl_name_dev1,
	&dsub_video_ctrl_name_dev2, &dsub_video_ctrl_name_dev3,
	&dsub_video_ctrl_name_dev4, &dsub_video_ctrl_name_dev5,
	&dsub_video_ctrl_name_dev6, &dsub_video_ctrl_name_dev7,
	&dsub_video_ctrl_name_dev8, &dsub_video_ctrl_name_dev9,
};

static void complete_video_ctrl_dev(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	dev = video_shell_get_dev_by_num(idx);
	if (!device_is_ready(dev) || idx >= ARRAY_SIZE(dsub_video_ctrl_name_dev)) {
		return;
	}

	entry->syntax = dev->name;
	entry->subcmd = dsub_video_ctrl_name_dev[idx];
}
SHELL_DYNAMIC_CMD_CREATE(dsub_video_ctrl_dev, complete_video_ctrl_dev);

static void complete_video_format_name(size_t idx, struct shell_static_entry *entry,
				       enum video_endpoint_id ep)
{
	const struct device *dev = video_shell_dev;
	static char fourcc[5] = {0};
	struct video_caps caps = {0};
	uint32_t prev_pixfmt = 0;
	uint32_t pixfmt = 0;
	int ret;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	/* Check which endpoint was selected */

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_OUT) {
		return;
	}

	video_shell_ep = ep;

	/* Then complete the format name */

	ret = video_get_caps(dev, video_shell_ep, &caps);
	if (ret != 0) {
		return;
	}

	for (size_t i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		if (idx == 0) {
			pixfmt = caps.format_caps[i].pixelformat;
			break;
		}

		if (prev_pixfmt != caps.format_caps[i].pixelformat) {
			idx--;
		}
		prev_pixfmt = caps.format_caps[i].pixelformat;
	}
	if (pixfmt == 0) {
		return;
	}

	memcpy(fourcc, VIDEO_FOURCC_TO_STR(pixfmt), 4);
	entry->syntax = fourcc;
}
static void complete_video_format_in_name(size_t idx, struct shell_static_entry *entry)
{
	complete_video_format_name(idx, entry, VIDEO_EP_IN);
}
static void complete_video_format_out_name(size_t idx, struct shell_static_entry *entry)
{
	complete_video_format_name(idx, entry, VIDEO_EP_OUT);
}
SHELL_DYNAMIC_CMD_CREATE(dsub_video_format_in_name, complete_video_format_in_name);
SHELL_DYNAMIC_CMD_CREATE(dsub_video_format_out_name, complete_video_format_out_name);

static void complete_video_format_dir_dev(size_t idx, struct shell_static_entry *entry, int devn)
{
	const struct device *dev;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
	video_shell_dev = NULL;

	/* Check which device was selected */

	dev = video_shell_dev = video_shell_get_dev_by_num(devn);
	if (!device_is_ready(dev)) {
		return;
	}

	/* Then complete the endpoint direction */

	switch (idx) {
	case 0:
		entry->subcmd = &dsub_video_format_in_name;
		entry->syntax = "in";
		break;
	case 1:
		entry->subcmd = &dsub_video_format_out_name;
		entry->syntax = "out";
		break;
	}
}
LISTIFY(10, VIDEO_SHELL_COMPLETE_DEFINE, (;), video_format_dir_dev);

static const union shell_cmd_entry *dsub_video_format_dir_dev[] = {
	&dsub_video_format_dir_dev0, &dsub_video_format_dir_dev1,
	&dsub_video_format_dir_dev2, &dsub_video_format_dir_dev3,
	&dsub_video_format_dir_dev4, &dsub_video_format_dir_dev5,
	&dsub_video_format_dir_dev6, &dsub_video_format_dir_dev7,
	&dsub_video_format_dir_dev8, &dsub_video_format_dir_dev9,
};

static void complete_video_format_dev(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	dev = video_shell_get_dev_by_num(idx);
	if (!device_is_ready(dev) || idx >= ARRAY_SIZE(dsub_video_format_dir_dev)) {
		return;
	}

	entry->syntax = dev->name;
	entry->subcmd = dsub_video_format_dir_dev[idx];
}
SHELL_DYNAMIC_CMD_CREATE(dsub_video_format_dev, complete_video_format_dev);

/* Video shell commands declaration */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_video_cmds,
	SHELL_CMD_ARG(start, &dsub_video_dev,
		"Start a video device and its sources\n"
		"Usage: video start <device>",
		cmd_video_start, 2, 0),
	SHELL_CMD_ARG(stop, &dsub_video_dev,
		"Stop a video device and its sources\n"
		"Usage: video stop <device>",
		cmd_video_stop, 2, 0),
	SHELL_CMD_ARG(capture, &dsub_video_dev,
		"Capture a given number of frames from a device\n"
		"Usage: video capture <device> <num-frames>",
		cmd_video_capture, 3, 0),
	SHELL_CMD_ARG(format, &dsub_video_format_dev,
		"Query or set the video format of a device\n"
		"Usage: video format <device> <ep> [<fourcc> <width>x<height>]",
		cmd_video_format, 3, 2),
	SHELL_CMD_ARG(frmival, &dsub_video_frmival_dev,
		"Query or set the video frame rate/interval of a device\n"
		"Usage: video frmival <device> <ep> [<n>fps|<n>ms|<n>us]",
		cmd_video_frmival, 3, 1),
	SHELL_CMD_ARG(ctrl, &dsub_video_ctrl_dev,
		"Query or set video controls of a device\n"
		"Usage: video ctrl <device> [<ctrl> <value>]",
		cmd_video_ctrl, 2, 2),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(video, &sub_video_cmds, "Video driver commands", NULL);
