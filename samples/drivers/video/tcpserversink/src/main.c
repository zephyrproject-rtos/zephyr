/*
 * Copyright (c) 2019 Linaro Limited
 * Copyright 2025 NXP
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define MY_PORT          5000
#define MAX_CLIENT_QUEUE 1

static ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = send(sock, buf, len, 0);

		if (out_len < 0) {
			return out_len;
		}
		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

int main(void)
{
	struct sockaddr_in addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	struct video_buffer *buffers[CONFIG_VIDEO_CAPTURE_N_BUFFERING];
	struct video_buffer *vbuf = &(struct video_buffer){};
	int ret, sock, client;
	struct video_format fmt;
	struct video_caps caps;
	struct video_frmival frmival;
	struct video_frmival_enum fie;
	enum video_buf_type type = VIDEO_BUF_TYPE_OUTPUT;
	const struct device *video_dev;
#if (CONFIG_VIDEO_SOURCE_CROP_WIDTH && CONFIG_VIDEO_SOURCE_CROP_HEIGHT) || \
	CONFIG_VIDEO_FRAME_HEIGHT || CONFIG_VIDEO_FRAME_WIDTH
	struct video_selection sel = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};
#endif
	size_t bsize;
	int i = 0;
#if CONFIG_VIDEO_FRAME_HEIGHT || CONFIG_VIDEO_FRAME_WIDTH
	int err;
#endif
	const struct device *last_dev = NULL;
	struct video_ctrl_query cq = {.id = VIDEO_CTRL_FLAG_NEXT_CTRL};
	struct video_control ctrl = {.id = VIDEO_CID_HFLIP, .val = 1};
	int tp_set_ret = -ENOTSUP;

	video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));
	if (!device_is_ready(video_dev)) {
		LOG_ERR("%s: video device not ready.", video_dev->name);
		return 0;
	}

	/* Prepare Network */
	(void)memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MY_PORT);

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		LOG_ERR("Failed to create TCP socket: %d", errno);
		return 0;
	}

	ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Failed to bind TCP socket: %d", errno);
		close(sock);
		return 0;
	}

	ret = listen(sock, MAX_CLIENT_QUEUE);
	if (ret < 0) {
		LOG_ERR("Failed to listen on TCP socket: %d", errno);
		close(sock);
		return 0;
	}

	/* Get capabilities */
	caps.type = type;
	if (video_get_caps(video_dev, &caps)) {
		LOG_ERR("Unable to retrieve video capabilities");
		return 0;
	}

	LOG_INF("- Capabilities:");
	while (caps.format_caps[i].pixelformat) {
		const struct video_format_cap *fcap = &caps.format_caps[i];
		/* fourcc to string */
		LOG_INF("  %s width [%u; %u; %u] height [%u; %u; %u]",
			VIDEO_FOURCC_TO_STR(fcap->pixelformat), fcap->width_min, fcap->width_max,
			fcap->width_step, fcap->height_min, fcap->height_max, fcap->height_step);
		i++;
	}

	/* Get default/native format */
	fmt.type = type;
	if (video_get_format(video_dev, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return 0;
	}

	LOG_INF("Video device detected, format: %s %ux%u",
		VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height);

	if (caps.min_line_count != LINE_COUNT_HEIGHT) {
		LOG_ERR("Partial framebuffers not supported by this sample");
		return 0;
	}

	/* Set the crop setting if necessary */
#if CONFIG_VIDEO_SOURCE_CROP_WIDTH && CONFIG_VIDEO_SOURCE_CROP_HEIGHT
	sel.target = VIDEO_SEL_TGT_CROP;
	sel.rect.left = CONFIG_VIDEO_SOURCE_CROP_LEFT;
	sel.rect.top = CONFIG_VIDEO_SOURCE_CROP_TOP;
	sel.rect.width = CONFIG_VIDEO_SOURCE_CROP_WIDTH;
	sel.rect.height = CONFIG_VIDEO_SOURCE_CROP_HEIGHT;
	if (video_set_selection(video_dev, &sel)) {
		LOG_ERR("Unable to set selection crop");
		return 0;
	}
	LOG_INF("Selection crop set to (%u,%u)/%ux%u", sel.rect.left, sel.rect.top, sel.rect.width,
		sel.rect.height);
#endif

#if CONFIG_VIDEO_FRAME_HEIGHT || CONFIG_VIDEO_FRAME_WIDTH
#if CONFIG_VIDEO_FRAME_HEIGHT
	fmt.height = CONFIG_VIDEO_FRAME_HEIGHT;
#endif

#if CONFIG_VIDEO_FRAME_WIDTH
	fmt.width = CONFIG_VIDEO_FRAME_WIDTH;
#endif

	/*
	 * Check (if possible) if targeted size is same as crop
	 * and if compose is necessary
	 */
	sel.target = VIDEO_SEL_TGT_CROP;
	err = video_get_selection(video_dev, &sel);
	if (err < 0 && err != -ENOSYS) {
		LOG_ERR("Unable to get selection crop");
		return 0;
	}

	if (err == 0 && (sel.rect.width != fmt.width || sel.rect.height != fmt.height)) {
		sel.target = VIDEO_SEL_TGT_COMPOSE;
		sel.rect.left = 0;
		sel.rect.top = 0;
		sel.rect.width = fmt.width;
		sel.rect.height = fmt.height;
		err = video_set_selection(video_dev, &sel);
		if (err < 0 && err != -ENOSYS) {
			LOG_ERR("Unable to set selection compose");
			return 0;
		}
	}
#endif

	if (strcmp(CONFIG_VIDEO_PIXEL_FORMAT, "")) {
		fmt.pixelformat = VIDEO_FOURCC_FROM_STR(CONFIG_VIDEO_PIXEL_FORMAT);
	}

	LOG_INF("- Video format: %s %ux%u", VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width,
		fmt.height);

	if (video_set_format(video_dev, &fmt)) {
		LOG_ERR("Unable to set format");
		return 0;
	}

	if (!video_get_frmival(video_dev, &frmival)) {
		LOG_INF("- Default frame rate : %f fps",
			1.0 * frmival.denominator / frmival.numerator);
	}

	LOG_INF("- Supported frame intervals for the default format:");
	memset(&fie, 0, sizeof(fie));
	fie.format = &fmt;
	while (video_enum_frmival(video_dev, &fie) == 0) {
		if (fie.type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
			LOG_INF("   %u/%u", fie.discrete.numerator, fie.discrete.denominator);
		} else {
			LOG_INF("   [min = %u/%u; max = %u/%u; step = %u/%u]",
				fie.stepwise.min.numerator, fie.stepwise.min.denominator,
				fie.stepwise.max.numerator, fie.stepwise.max.denominator,
				fie.stepwise.step.numerator, fie.stepwise.step.denominator);
		}
		fie.index++;
	}

	/* Get supported controls */
	LOG_INF("- Supported controls:");
	cq.dev = video_dev;
	while (!video_query_ctrl(&cq)) {
		if (cq.dev != last_dev) {
			last_dev = cq.dev;
			LOG_INF("\t\tdevice: %s", cq.dev->name);
		}
		video_print_ctrl(&cq);
		cq.id |= VIDEO_CTRL_FLAG_NEXT_CTRL;
	}

	/* Set controls */
	if (IS_ENABLED(CONFIG_VIDEO_CTRL_HFLIP)) {
		video_set_ctrl(video_dev, &ctrl);
	}

	if (IS_ENABLED(CONFIG_VIDEO_CTRL_VFLIP)) {
		ctrl.id = VIDEO_CID_VFLIP;
		video_set_ctrl(video_dev, &ctrl);
	}

	if (IS_ENABLED(CONFIG_TEST)) {
		ctrl.id = VIDEO_CID_TEST_PATTERN;
		tp_set_ret = video_set_ctrl(video_dev, &ctrl);
	}

	/* Size to allocate for each buffer */
	if (caps.min_line_count == LINE_COUNT_HEIGHT) {
		bsize = fmt.pitch * fmt.height;
	} else {
		bsize = fmt.pitch * caps.min_line_count;
	}

	/* Alloc Buffers */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		/*
		 * For some hardwares, such as the PxP used on i.MX RT1170 to do image rotation,
		 * buffer alignment is needed in order to achieve the best performance
		 */
		buffers[i] = video_buffer_aligned_alloc(bsize, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
							K_FOREVER);
		if (buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return 0;
		}
		buffers[i]->type = type;
	}

	/* Connection loop */
	do {
		LOG_INF("TCP: Waiting for client...");

		client = accept(sock, (struct sockaddr *)&client_addr, &client_addr_len);
		if (client < 0) {
			LOG_ERR("Failed to accept: %d", errno);
			return 0;
		}

		LOG_INF("TCP: Accepted connection");

		/* Enqueue Buffers */
		for (i = 0; i < ARRAY_SIZE(buffers); i++) {
			video_enqueue(video_dev, buffers[i]);
		}

		/* Start video capture */
		if (video_stream_start(video_dev, type)) {
			LOG_ERR("Unable to start video");
			return 0;
		}

		LOG_INF("Stream started");

		/* Capture loop */
		i = 0;
		vbuf->type = type;
		do {
			ret = video_dequeue(video_dev, &vbuf, K_FOREVER);
			if (ret) {
				LOG_ERR("Unable to dequeue video buf");
				return 0;
			}

			LOG_INF("Sending frame %d", i++);

			/* Send video buffer to TCP client */
			ret = sendall(client, vbuf->buffer, vbuf->bytesused);
			if (ret && ret != -EAGAIN) {
				/* client disconnected */
				LOG_ERR("TCP: Client disconnected %d", ret);
				close(client);
			}

			(void)video_enqueue(video_dev, vbuf);
		} while (!ret);

		/* stop capture */
		if (video_stream_stop(video_dev, type)) {
			LOG_ERR("Unable to stop video");
			return 0;
		}

		/* Flush remaining buffers */
		do {
			ret = video_dequeue(video_dev, &vbuf, K_NO_WAIT);
		} while (!ret);

	} while (1);
}
