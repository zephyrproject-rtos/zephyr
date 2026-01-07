/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>

#include "video_ctrls.h"

#define VIDEO_FRMIVAL_FPS(frmival)  DIV_ROUND_CLOSEST((frmival)->denominator, (frmival)->numerator)
#define VIDEO_FRMIVAL_MSEC(frmival) (MSEC_PER_SEC * (frmival)->numerator / (frmival)->denominator)

/* Helper to allow completion of sub-command with content that depends on previous selection */
#define VIDEO_SHELL_COMPLETE_DEFINE(n, name)                                                       \
	static void complete_##name##n(size_t idx, struct shell_static_entry *entry)               \
	{                                                                                          \
		complete_##name(idx, entry, n);                                                    \
	}                                                                                          \
	SHELL_DYNAMIC_CMD_CREATE(dsub_##name##n, complete_##name##n)

/* Current video device, used for tab completion */
static const struct device *video_shell_dev;

/* Current video control, used for tab completion */
static struct video_ctrl_query video_shell_cq;

/* Current buffer direction, used for tab completion */
static enum video_buf_type video_shell_buf_type = VIDEO_BUF_TYPE_OUTPUT;

/* A global variable holding the buffer set to entry->syntax for control identifiers */
static char video_shell_ctrl_name[CONFIG_VIDEO_SHELL_CTRL_NAME_SIZE];

/* A global variable holding the buffer set to entry->syntax for control menu values */
static char video_shell_menu_name[CONFIG_VIDEO_SHELL_CTRL_NAME_SIZE];

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

static int video_shell_parse_in_out(const struct shell *sh, char const *arg_in_out,
				    enum video_buf_type *type)
{
	if (strcmp(arg_in_out, "in") == 0) {
		*type = VIDEO_BUF_TYPE_INPUT;
	} else if (strcmp(arg_in_out, "out") == 0) {
		*type = VIDEO_BUF_TYPE_OUTPUT;
	} else {
		shell_error(sh, "Endpoint direction must be 'in' or 'out', not '%s'", arg_in_out);
		return -EINVAL;
	}

	return 0;
}

static int video_shell_parse_on_off(const struct shell *sh, char const *arg_on_off, int32_t *value)
{
	if (strcmp(arg_on_off, "on") == 0 || strcmp(arg_on_off, "1") == 0) {
		*value = true;
	} else if (strcmp(arg_on_off, "off") == 0 || strcmp(arg_on_off, "0") == 0) {
		*value = false;
	} else {
		shell_error(sh, "Endpoint direction must be 'on' or 'off', not '%s'", arg_on_off);
		return -EINVAL;
	}

	return 0;
}

static int cmd_video_start(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	char *arg_device = argv[1];
	enum video_buf_type type;
	int ret;

	dev = device_get_binding(arg_device);
	ret = video_shell_check_device(sh, dev);
	if (ret < 0) {
		return ret;
	}

	/* Only starting of output is supported for now */
	type = VIDEO_BUF_TYPE_OUTPUT;

	ret = video_stream_start(dev, type);
	if (ret < 0) {
		shell_error(sh, "Failed to start %s", dev->name);
		return ret;
	}

	shell_print(sh, "Started %s video stream", dev->name);
	return 0;
}

static int cmd_video_stop(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	char *arg_device = argv[1];
	enum video_buf_type type;
	int ret;

	dev = device_get_binding(arg_device);
	ret = video_shell_check_device(sh, dev);
	if (ret < 0) {
		return ret;
	}

	/* Only starting of output is supported for now */
	type = VIDEO_BUF_TYPE_OUTPUT;

	ret = video_stream_stop(dev, type);
	if (ret < 0) {
		shell_error(sh, "Failed to stop %s", dev->name);
		return ret;
	}

	shell_print(sh, "Stopped %s video stream", dev->name);
	return 0;
}

static void video_shell_print_buffer(const struct shell *sh, struct video_buffer *vbuf,
				     struct video_format *fmt, int i, uint32_t num_buffer,
				     uint32_t frmrate_fps, uint32_t frmival_msec)
{
	uint32_t line_offset = vbuf->line_offset;
	uint32_t byte_offset = line_offset * fmt->pitch;
	uint32_t bytes_in_buf = vbuf->bytesused;
	uint32_t lines_in_buf = vbuf->bytesused / fmt->pitch;

	shell_print(sh, "Buffer %u/%u at %u ms, Bytes %u-%u/%u, Lines %u-%u/%u, Rate %u FPS %u ms",
		    /* Buffer */ i + 1, num_buffer, vbuf->timestamp,
		    /* Bytes */ byte_offset, byte_offset + bytes_in_buf, fmt->height * fmt->pitch,
		    /* Lines */ line_offset, line_offset + lines_in_buf, fmt->height,
		    /* Rate */ frmrate_fps, frmival_msec);
}

static int cmd_video_capture(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_buffer *buffers[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX] = {NULL};
	struct video_buffer vbuf0 = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_buffer *vbuf = &vbuf0;
	char *arg_device = argv[1];
	char *arg_nbufs = argv[2];
	uint32_t first_uptime;
	uint32_t prev_uptime;
	uint32_t this_uptime;
	uint32_t frmival_msec;
	uint32_t frmrate_fps;
	size_t buf_size;
	unsigned long num_buffers;
	int ret;

	dev = device_get_binding(arg_device);
	ret = video_shell_check_device(sh, dev);
	if (ret < 0) {
		return ret;
	}

	ret = video_get_format(dev, &fmt);
	if (ret < 0) {
		shell_error(sh, "Failed to get the current format interval");
		return ret;
	}

	num_buffers = strtoull(arg_nbufs, &arg_nbufs, 10);
	if (*arg_nbufs != '\0') {
		shell_error(sh, "Invalid integer '%s' for this type", arg_nbufs);
		return -EINVAL;
	}

	buf_size = fmt.pitch * fmt.height;

	shell_print(sh, "Preparing %u buffers of %u bytes each",
		    CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, buf_size);

	for (int i = 0; i < ARRAY_SIZE(buffers); i++) {
		buffers[i] = video_buffer_alloc(buf_size, K_NO_WAIT);
		if (buffers[i] == NULL) {
			shell_error(sh, "Failed to allocate buffer %u", i);
			goto end;
		}

		/* Only queueing of output buffers is supported for now */
		buffers[i]->type = VIDEO_BUF_TYPE_OUTPUT;

		ret = video_enqueue(dev, buffers[i]);
		if (ret < 0) {
			shell_error(sh, "Failed to enqueue buffer %u: %s", i, strerror(-ret));
			goto end;
		}
	}

	shell_print(sh, "Starting the capture of %lu buffers from %s", num_buffers, dev->name);

	ret = video_stream_start(dev, VIDEO_BUF_TYPE_OUTPUT);
	if (ret < 0) {
		shell_error(sh, "Failed to start %s", dev->name);
		goto end;
	}

	first_uptime = prev_uptime = this_uptime = k_uptime_get_32();

	for (unsigned long i = 0; i < num_buffers;) {
		ret = video_dequeue(dev, &vbuf, K_FOREVER);
		if (ret < 0) {
			shell_error(sh, "Failed to dequeue this buffer: %s", strerror(-ret));
			goto end;
		}

		this_uptime = k_uptime_get_32();
		frmival_msec = this_uptime - prev_uptime;
		frmrate_fps = (frmival_msec == 0) ? (UINT32_MAX) : (MSEC_PER_SEC / frmival_msec);
		prev_uptime = this_uptime;

		video_shell_print_buffer(sh, vbuf, &fmt, i, num_buffers, frmrate_fps, frmival_msec);

		/* Only increment the frame counter on the beginning of a new frame */
		i += (vbuf->line_offset == 0);

		ret = video_enqueue(dev, vbuf);
		if (ret < 0) {
			shell_error(sh, "Failed to enqueue this buffer: %s", strerror(-ret));
			goto end;
		}
	}

	frmival_msec = this_uptime - first_uptime;
	frmrate_fps =
		(frmival_msec == 0) ? (UINT32_MAX) : (num_buffers * MSEC_PER_SEC / frmival_msec);

	shell_print(sh, "Capture of %lu buffers in %u ms in total, %u FPS on average, stopping %s",
		    num_buffers, frmival_msec, frmrate_fps, dev->name);

end:
	video_stream_stop(dev, VIDEO_BUF_TYPE_OUTPUT);

	if (vbuf != NULL) {
		video_buffer_release(vbuf);
	}

	while (video_dequeue(dev, &vbuf, K_NO_WAIT) == 0) {
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
				     uint32_t pixfmt, uint32_t width, uint32_t height)
{
	struct video_format fmt = {.pixelformat = pixfmt, .width = width, .height = height};
	struct video_frmival_enum fie = {.format = &fmt};

	for (fie.index = 0; video_enum_frmival(dev, &fie) == 0; fie.index++) {

		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			shell_print(sh, "\t\t\tInterval: "
				    "Discrete %u ms (%u FPS)",
				    VIDEO_FRMIVAL_MSEC(&fie.discrete),
				    VIDEO_FRMIVAL_FPS(&fie.discrete));
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			shell_print(sh, "\t\t\tInterval: "
				    "Stepwise: %u ms - %u ms with step %u ms (%u - %u FPS)",
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
				   size_t argc, char **argv)
{
	struct video_frmival frmival = {0};
	char *arg_rate = argv[0];
	char *end_rate;
	unsigned long val;
	int ret;

	val = strtoul(arg_rate, &end_rate, 10);
	if (strcmp(end_rate, "fps") == 0) {
		frmival.numerator = 1;
		frmival.denominator = val;
	} else if (strcmp(end_rate, "ms") == 0) {
		frmival.numerator = val;
		frmival.denominator = MSEC_PER_SEC;
	} else if (strcmp(end_rate, "us") == 0) {
		frmival.numerator = val;
		frmival.denominator = USEC_PER_SEC;
	} else {
		shell_error(sh, "Expected <n>ms, <n>us or <n>fps, not '%s'", arg_rate);
		return -EINVAL;
	}

	ret = video_set_frmival(dev, &frmival);
	if (ret < 0) {
		shell_error(sh, "Failed to set frame interval");
		return ret;
	}

	shell_print(sh, "New frame interval:     %u ms (%u FPS)",
		    VIDEO_FRMIVAL_MSEC(&frmival), VIDEO_FRMIVAL_FPS(&frmival));

	return 0;
}

static int cmd_video_frmival(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct video_format fmt = {.type = video_shell_buf_type};
	struct video_frmival frmival = {0};
	char *arg_device = argv[1];
	int ret;

	dev = device_get_binding(arg_device);
	ret = video_shell_check_device(sh, dev);
	if (ret < 0) {
		return ret;
	}

	ret = video_get_frmival(dev, &frmival);
	if (ret < 0) {
		shell_error(sh, "Failed to get the current frame interval");
		return ret;
	}

	ret = video_get_format(dev, &fmt);
	if (ret < 0) {
		shell_error(sh, "Failed to get the current format interval");
		return ret;
	}

	shell_print(sh, "Current frame interval: %u ms (%u FPS)",
		    VIDEO_FRMIVAL_MSEC(&frmival), VIDEO_FRMIVAL_FPS(&frmival));

	switch (argc) {
	case 2:
		return video_shell_print_frmival(sh, dev, fmt.pixelformat, fmt.width,
						 fmt.height);
	case 3:
		return video_shell_set_frmival(sh, dev, argc - 2, &argv[2]);
	default:
		shell_error(sh, "Wrong parameter count");
		return -EINVAL;
	}
}

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
}
SHELL_DYNAMIC_CMD_CREATE(dsub_video_frmival_dev, complete_video_frmival_dev);

/* Video format handling */

static int video_shell_print_format_cap(const struct shell *sh, const struct device *dev,
					enum video_buf_type type,
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

		ret = video_shell_print_frmival(sh, dev, cap->pixelformat, cap->width_min,
						cap->height_min);
		if (ret < 0) {
			return ret;
		}
	} else {
		shell_print(sh, "\t\tSize: "
			    "Stepwise: %ux%u - %ux%u with step %ux%u (%u - %u bytes per frame)",
			    cap->width_min, cap->height_min, cap->width_max, cap->height_max,
			    cap->width_step, cap->height_step, size_min, size_max);

		ret = video_shell_print_frmival(sh, dev, cap->pixelformat, cap->width_min,
						cap->height_min);
		if (ret < 0) {
			return ret;
		}

		ret = video_shell_print_frmival(sh, dev, cap->pixelformat, cap->width_max,
						cap->height_max);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int video_shell_print_caps(const struct shell *sh, const struct device *dev,
				  struct video_caps *caps)
{
	int ret;

	shell_print(sh, "min vbuf count: %u", caps->min_vbuf_count);

	for (size_t i = 0; caps->format_caps[i].pixelformat != 0; i++) {
		ret = video_shell_print_format_cap(sh, dev, caps->type, &caps->format_caps[i], i);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int video_shell_set_format(const struct shell *sh, const struct device *dev,
				  enum video_buf_type type, size_t argc, char **argv)
{
	struct video_format fmt = {.type = video_shell_buf_type};
	char *arg_fourcc = argv[0];
	char *arg_size = argv[1];
	char *end_size;
	int ret;

	if (strlen(arg_fourcc) != 4) {
		shell_error(sh, "Invalid four character code: '%s'", arg_fourcc);
		return -EINVAL;
	}

	fmt.pixelformat = VIDEO_FOURCC_FROM_STR(arg_fourcc);

	fmt.width = strtoul(arg_size, &end_size, 10);
	if (*end_size != 'x' || fmt.width == 0) {
		shell_error(sh, "Invalid width in <width>x<height> parameter %s", arg_size);
		return -EINVAL;
	}
	end_size++;

	fmt.height = strtoul(end_size, &end_size, 10);
	if (*end_size != '\0' || fmt.height == 0) {
		shell_error(sh, "Invalid height in <width>x<height> parameter %s (%s)",
			    arg_size, end_size);
		return -EINVAL;
	}

	fmt.pitch = fmt.width * video_bits_per_pixel(fmt.pixelformat) / BITS_PER_BYTE;

	ret = video_set_format(dev, &fmt);
	if (ret < 0) {
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
	enum video_buf_type type;
	char *arg_device = argv[1];
	char *arg_in_out = argv[2];
	int ret;

	dev = device_get_binding(arg_device);
	ret = video_shell_check_device(sh, dev);
	if (ret < 0) {
		return ret;
	}

	ret = video_shell_parse_in_out(sh, arg_in_out, &type);
	if (ret < 0) {
		return ret;
	}
	fmt.type = caps.type = type;

	ret = video_get_caps(dev, &caps);
	if (ret < 0) {
		shell_error(sh, "Failed to query %s capabilities", dev->name);
		return -ENODEV;
	}

	ret = video_get_format(dev, &fmt);
	if (ret < 0) {
		shell_error(sh, "Failed to query %s capabilities", dev->name);
		return -ENODEV;
	}

	shell_print(sh, "Current format: '%s' %ux%u (%u bytes per frame)",
		    VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height,
		    fmt.pitch * fmt.height);

	switch (argc) {
	case 3:
		return video_shell_print_caps(sh, dev, &caps);
	case 5:
		return video_shell_set_format(sh, dev, type, argc - 3, &argv[3]);
	default:
		shell_error(sh, "Wrong parameter count");
		return -EINVAL;
	}
}

/* Video control handling */

static void video_shell_convert_ctrl_name(char const *src, char *dst, size_t dst_size)
{
	size_t o = 0;
	bool add_underscore = false;

	for (size_t i = 0; src[i] != '\0'; i++) {
		if (o >= dst_size) {
			break;
		}

		if (isalnum(src[i])) {
			dst[o] = tolower(src[i]);
			o++;
			add_underscore = true;
		} else if (add_underscore) {
			dst[o] = '_';
			o++;
			add_underscore = false;
		} else {
			/* skip */
		}
	}

	do {
		dst[o] = '\0';
	} while (o-- > 0 && !isalnum(dst[o]));
}

static int video_shell_get_ctrl_by_name(struct video_ctrl_query *cq, char const *name)
{
	char ctrl_name[CONFIG_VIDEO_SHELL_CTRL_NAME_SIZE];
	int ret;

	cq->id = 0;
	for (size_t i = 0;; i++) {
		cq->id |= VIDEO_CTRL_FLAG_NEXT_CTRL;

		ret = video_query_ctrl(cq);
		if (ret < 0) {
			return -ENOENT;
		}

		video_shell_convert_ctrl_name(cq->name, ctrl_name, sizeof(ctrl_name));
		if (strcmp(ctrl_name, name) == 0) {
			break;
		}
	}

	return 0;
}

static int video_shell_get_ctrl_by_num(struct video_ctrl_query *cq, int num)
{
	int ret;

	cq->id = 0;
	for (size_t i = 0; i <= num; i++) {
		cq->id |= VIDEO_CTRL_FLAG_NEXT_CTRL;

		ret = video_query_ctrl(cq);
		if (ret < 0) {
			return -ENOENT;
		}
	}

	return 0;
}

static int video_shell_set_ctrl(const struct shell *sh, const struct device *dev, size_t argc,
				char **argv)
{
	struct video_ctrl_query cq = {.dev = dev};
	struct video_control ctrl;
	char menu_name[CONFIG_VIDEO_SHELL_CTRL_NAME_SIZE];
	char *arg_ctrl = argv[0];
	char *arg_value = argv[1];
	char *end_value;
	size_t i;
	int ret;

	ret = video_shell_get_ctrl_by_name(&cq, arg_ctrl);
	if (ret < 0) {
		shell_error(sh, "Video control '%s' not found for this device", arg_ctrl);
		return ret;
	}

	ctrl.id = cq.id;

	switch (cq.type) {
	case VIDEO_CTRL_TYPE_MENU:
		for (i = 0; cq.menu[i] != NULL; i++) {
			video_shell_convert_ctrl_name(cq.menu[i], menu_name, sizeof(menu_name));
			if (strcmp(menu_name, arg_value) == 0) {
				ctrl.val64 = i;
			}
		}
		if (cq.menu[i] != NULL) {
			shell_error(sh, "Video control value '%s' is not present in the menu",
				    arg_value);
			return -EINVAL;
		}
		break;
	case VIDEO_CTRL_TYPE_BOOLEAN:
		ret = video_shell_parse_on_off(sh, arg_value, &ctrl.val);
		if (ret < 0) {
			return ret;
		}
		break;
	case VIDEO_CTRL_TYPE_INTEGER:
		ctrl.val = strtol(arg_value, &end_value, 10);
		if (*end_value != '\0') {
			shell_error(sh, "Invalid integer '%s' for this type", arg_value);
			return -EINVAL;
		}
		break;
	case VIDEO_CTRL_TYPE_INTEGER64:
		ctrl.val64 = strtoll(arg_value, &arg_value, 10);
		if (*end_value != '\0') {
			shell_error(sh, "Invalid integer '%s' for this type", arg_value);
			return -EINVAL;
		}
		break;
	default:
		CODE_UNREACHABLE;
	}

	shell_print(sh, "Setting control '%s' (0x%08x) to value '%s' (%lld)",
		    arg_ctrl, ctrl.id, arg_value,
		    (long long)(cq.type == VIDEO_CTRL_TYPE_INTEGER64 ? ctrl.val64 : ctrl.val));

	ret = video_set_ctrl(dev, &ctrl);
	if (ret < 0) {
		shell_error(sh, "Failed to update control 0x%08x", ctrl.id);
		return ret;
	}

	return 0;
}

static int video_shell_print_ctrls(const struct shell *sh, const struct device *dev)
{
	struct video_ctrl_query cq = {.dev = dev, .id = VIDEO_CTRL_FLAG_NEXT_CTRL};
	char ctrl_name[CONFIG_VIDEO_SHELL_CTRL_NAME_SIZE];

	while (video_query_ctrl(&cq) == 0) {
		/* Convert from human-friendly form to machine-friendly */
		video_shell_convert_ctrl_name(cq.name, ctrl_name, sizeof(ctrl_name));
		cq.name = ctrl_name;

		video_print_ctrl(&cq);
		cq.id |= VIDEO_CTRL_FLAG_NEXT_CTRL;
	}

	return 0;
}

static int cmd_video_ctrl(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	char *arg_device = argv[1];
	int ret;

	dev = device_get_binding(arg_device);
	ret = video_shell_check_device(sh, dev);
	if (ret < 0) {
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
	struct video_ctrl_query cq = {.dev = dev};
	int ret;

	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
	entry->syntax = NULL;

	if (!device_is_ready(dev)) {
		return;
	}

	/* Check which control name was selected */

	ret = video_shell_get_ctrl_by_num(&cq, ctrln);
	if (ret < 0) {
		return;
	}

	/* Then complete the menu value */

	for (size_t i = 0; cq.menu[i] != NULL; i++) {
		if (i == idx) {
			video_shell_convert_ctrl_name(cq.menu[i], video_shell_menu_name,
						      sizeof(video_shell_menu_name));
			entry->syntax = video_shell_menu_name;
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
	cq->dev = dev;
	ret = video_shell_get_ctrl_by_num(cq, idx);
	if (ret < 0) {
		return;
	}

	video_shell_convert_ctrl_name(cq->name, video_shell_ctrl_name,
				      sizeof(video_shell_ctrl_name));
	entry->syntax = video_shell_ctrl_name;

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
	default:
		entry->subcmd = NULL;
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
				       enum video_buf_type type)
{
	const struct device *dev = video_shell_dev;
	static char fourcc[5] = {0};
	struct video_caps caps = {.type = video_shell_buf_type};
	uint32_t prev_pixfmt = 0;
	uint32_t pixfmt = 0;
	int ret;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	/* Fill which buffer type was given in the partial input */

	video_shell_buf_type = type;

	/* Then complete the format name */

	ret = video_get_caps(dev, &caps);
	if (ret < 0) {
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

	memcpy(fourcc, VIDEO_FOURCC_TO_STR(pixfmt), sizeof(fourcc));
	entry->syntax = fourcc;
}
static void complete_video_format_in_name(size_t idx, struct shell_static_entry *entry)
{
	complete_video_format_name(idx, entry, VIDEO_BUF_TYPE_INPUT);
}
static void complete_video_format_out_name(size_t idx, struct shell_static_entry *entry)
{
	complete_video_format_name(idx, entry, VIDEO_BUF_TYPE_OUTPUT);
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
	default:
		entry->syntax = NULL;
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

/* Video selection handling */

static void video_shell_print_selection(const struct shell *sh, struct video_selection *sel)
{
	shell_print(sh, "\tselection target: %s: (%u,%u)/%ux%u",
		    sel->target == VIDEO_SEL_TGT_CROP ? "crop" :
		    sel->target == VIDEO_SEL_TGT_CROP_BOUND ? "crop_bound" :
		    sel->target == VIDEO_SEL_TGT_NATIVE_SIZE ? "native_size" :
		    sel->target == VIDEO_SEL_TGT_COMPOSE ? "compose" : "unknown",
		    sel->rect.left, sel->rect.top, sel->rect.width, sel->rect.height);
}

static int video_shell_selection_parse_target(const struct shell *sh, char const *arg_target,
					      enum video_selection_target *sel_target)
{
	if (strcmp(arg_target, "crop") == 0) {
		*sel_target = VIDEO_SEL_TGT_CROP;
	} else if (strcmp(arg_target, "crop_bound") == 0) {
		*sel_target = VIDEO_SEL_TGT_CROP_BOUND;
	} else if (strcmp(arg_target, "native_size") == 0) {
		*sel_target = VIDEO_SEL_TGT_NATIVE_SIZE;
	} else if (strcmp(arg_target, "compose") == 0) {
		*sel_target = VIDEO_SEL_TGT_COMPOSE;
	} else if (strcmp(arg_target, "compose_bound") == 0) {
		*sel_target = VIDEO_SEL_TGT_COMPOSE_BOUND;
	} else {
		shell_error(sh,
			    "Target must be crop, crop_bound, native_size, compose or compose_bound");
		return -EINVAL;
	}

	return 0;
}

static int video_shell_selection_parse_rect(const struct shell *sh, char **args_rect,
					    struct video_rect *rect)
{
	char *arg_left = args_rect[0];
	char *arg_top = args_rect[1];
	char *arg_width_height = args_rect[2];
	char *end_size;

	rect->left = strtoul(arg_left, &end_size, 10);
	if (*end_size != '\0') {
		shell_error(sh,
			    "Invalid left value in rect <left> <top> <width>x<height> parameters");
		return -EINVAL;
	}

	rect->top = strtoul(arg_top, &end_size, 10);
	if (*end_size != '\0') {
		shell_error(sh,
			    "Invalid top value in rect <left> <top> <width>x<height> parameters");
		return -EINVAL;
	}

	rect->width = strtoul(arg_width_height, &end_size, 10);
	if (*end_size != 'x' || rect->width == 0) {
		shell_error(sh,
			    "Invalid width value in rect left> <top> <width>x<height> parameters");
		return -EINVAL;
	}
	end_size++;

	rect->height = strtoul(end_size, &end_size, 10);
	if (*end_size != '\0' || rect->height == 0) {
		shell_error(sh,
			    "Invalid height value in rect left> <top> <width>x<height> parameters");
		return -EINVAL;
	}

	return 0;
}

static int cmd_video_selection(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct video_selection sel = {0};
	char *arg_device = argv[1];
	char *arg_in_out = argv[2];
	char *arg_target = argv[3];
	int ret;

	dev = device_get_binding(arg_device);
	ret = video_shell_check_device(sh, dev);
	if (ret < 0) {
		return ret;
	}

	ret = video_shell_parse_in_out(sh, arg_in_out, &sel.type);
	if (ret < 0) {
		return ret;
	}

	ret = video_shell_selection_parse_target(sh, arg_target, &sel.target);
	if (ret < 0) {
		return ret;
	}

	switch (argc) {
	case 4:
		ret = video_get_selection(dev, &sel);
		if (ret < 0) {
			shell_error(sh, "Failed to get %s selection", dev->name);
			return -ENODEV;
		}

		video_shell_print_selection(sh, &sel);
		break;
	case 7:
		ret = video_shell_selection_parse_rect(sh, &argv[4], &sel.rect);
		if (ret < 0) {
			return ret;
		}

		ret = video_set_selection(dev, &sel);
		if (ret < 0) {
			shell_error(sh, "Failed to set %s selection", dev->name);
			return -ENODEV;
		}

		video_shell_print_selection(sh, &sel);
		break;
	default:
		shell_error(sh, "Wrong parameter count");
		return -EINVAL;
	}

	return 0;
}

/* Video shell commands declaration */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_video_cmds,
	SHELL_CMD_ARG(start, &dsub_video_dev,
		SHELL_HELP("Start a video device and its sources", "<device>"),
		cmd_video_start, 2, 0),
	SHELL_CMD_ARG(stop, &dsub_video_dev,
		SHELL_HELP("Stop a video device and its sources", "<device>"),
		cmd_video_stop, 2, 0),
	SHELL_CMD_ARG(capture, &dsub_video_dev,
		SHELL_HELP("Capture a given number of buffers from a device",
			   "<device> <num-buffers>"),
		cmd_video_capture, 3, 0),
	SHELL_CMD_ARG(format, &dsub_video_format_dev,
		SHELL_HELP("Query or set the video format of a device",
			   "<device> <dir> [<fourcc> <width>x<height>]"),
		cmd_video_format, 3, 2),
	SHELL_CMD_ARG(frmival, &dsub_video_frmival_dev,
		SHELL_HELP("Query or set the video frame rate/interval of a device",
			   "<device> [<n>fps|<n>ms|<n>us]"),
		cmd_video_frmival, 2, 1),
	SHELL_CMD_ARG(ctrl, &dsub_video_ctrl_dev,
		SHELL_HELP("Query or set video controls of a device",
			   "<device> [<ctrl> <value>]"),
		cmd_video_ctrl, 2, 2),
	SHELL_CMD_ARG(selection, &dsub_video_format_dev,
		SHELL_HELP("Query or set the video selection of a device",
			   "<device> <dir> <target> [<left> <top> <width>x<height>]"),
		cmd_video_selection, 4, 3),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(video, &sub_video_cmds, "Video driver commands", NULL);
