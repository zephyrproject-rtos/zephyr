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

#if DT_HAS_CHOSEN(zephyr_videoenc)
const struct device *encoder_dev;

int configure_encoder(void)
{
	struct video_buffer *buffer;
	struct video_format fmt;
	struct video_caps caps;
	uint32_t size;
	int i;

	encoder_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_videoenc));
	if (!device_is_ready(encoder_dev)) {
		LOG_ERR("%s: encoder video device not ready.", encoder_dev->name);
		return -1;
	}

	/* Get capabilities */
	caps.type = VIDEO_BUF_TYPE_OUTPUT;
	if (video_get_caps(encoder_dev, &caps)) {
		LOG_ERR("Unable to retrieve video capabilities");
		return -1;
	}

	LOG_INF("- Encoder output capabilities:");
	i = 0;
	while (caps.format_caps[i].pixelformat) {
		const struct video_format_cap *fcap = &caps.format_caps[i];
		/* fourcc to string */
		LOG_INF("  %s width [%u; %u; %u] height [%u; %u; %u]",
			VIDEO_FOURCC_TO_STR(fcap->pixelformat), fcap->width_min, fcap->width_max,
			fcap->width_step, fcap->height_min, fcap->height_max, fcap->height_step);
		i++;
	}

	caps.type = VIDEO_BUF_TYPE_INPUT;
	if (video_get_caps(encoder_dev, &caps)) {
		LOG_ERR("Unable to retrieve video capabilities");
		return -1;
	}

	LOG_INF("- Encoder input capabilities:");
	i = 0;
	while (caps.format_caps[i].pixelformat) {
		const struct video_format_cap *fcap = &caps.format_caps[i];
		/* fourcc to string */
		LOG_INF("  %s width [%u; %u; %u] height [%u; %u; %u]",
			VIDEO_FOURCC_TO_STR(fcap->pixelformat), fcap->width_min, fcap->width_max,
			fcap->width_step, fcap->height_min, fcap->height_max, fcap->height_step);
		i++;
	}

	/* Get default/native format */
	fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	if (video_get_format(encoder_dev, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return -1;
	}

	LOG_INF("Video encoder device detected, format: %s %ux%u",
		VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height);

#if CONFIG_VIDEO_FRAME_HEIGHT
	fmt.height = CONFIG_VIDEO_FRAME_HEIGHT;
#endif

#if CONFIG_VIDEO_FRAME_WIDTH
	fmt.width = CONFIG_VIDEO_FRAME_WIDTH;
#endif

	/* Set output format */
	if (strcmp(CONFIG_VIDEO_ENCODED_PIXEL_FORMAT, "")) {
		fmt.pixelformat = VIDEO_FOURCC_FROM_STR(CONFIG_VIDEO_ENCODED_PIXEL_FORMAT);
	}

	LOG_INF("- Video encoder output format: %s %ux%u", VIDEO_FOURCC_TO_STR(fmt.pixelformat),
		fmt.width, fmt.height);

	fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	if (video_set_format(encoder_dev, &fmt)) {
		LOG_ERR("Unable to set format");
		return -1;
	}

	/* Alloc output buffer */
	size = fmt.size;
	if (size == 0) {
		LOG_ERR("Encoder driver must set format size");
		return -1;
	}

	buffer = video_buffer_aligned_alloc(size, CONFIG_VIDEO_BUFFER_POOL_ALIGN, K_NO_WAIT);
	if (buffer == NULL) {
		LOG_ERR("Unable to alloc compressed video buffer size=%d", size);
		return -1;
	}

	buffer->type = VIDEO_BUF_TYPE_OUTPUT;
	if (video_enqueue(encoder_dev, buffer)) {
		LOG_ERR("Unable to enqueue encoder output buf");
		return -1;
	}

	/* Set input format */
	if (strcmp(CONFIG_VIDEO_PIXEL_FORMAT, "")) {
		fmt.pixelformat = VIDEO_FOURCC_FROM_STR(CONFIG_VIDEO_PIXEL_FORMAT);
	}

	LOG_INF("- Video encoder input format: %s %ux%u", VIDEO_FOURCC_TO_STR(fmt.pixelformat),
		fmt.width, fmt.height);

	fmt.type = VIDEO_BUF_TYPE_INPUT;
	if (video_set_format(encoder_dev, &fmt)) {
		LOG_ERR("Unable to set input format");
		return -1;
	}

	/* Start video encoder */
	if (video_stream_start(encoder_dev, VIDEO_BUF_TYPE_INPUT)) {
		LOG_ERR("Unable to start video encoder (input)");
		return -1;
	}
	if (video_stream_start(encoder_dev, VIDEO_BUF_TYPE_OUTPUT)) {
		LOG_ERR("Unable to start video encoder (output)");
		return -1;
	}

	return 0;
}

int encode_frame(struct video_buffer *in, struct video_buffer **out)
{
	int ret;

	in->type = VIDEO_BUF_TYPE_INPUT;
	ret = video_enqueue(encoder_dev, in);
	if (ret) {
		LOG_ERR("Unable to enqueue encoder input buf");
		return ret;
	}

	(*out)->type = VIDEO_BUF_TYPE_OUTPUT;
	ret = video_dequeue(encoder_dev, out, K_FOREVER);
	if (ret) {
		LOG_ERR("Unable to dequeue encoder output buf");
		return ret;
	}

	return 0;
}

void stop_encoder(void)
{
	if (video_stream_stop(encoder_dev, VIDEO_BUF_TYPE_OUTPUT)) {
		LOG_ERR("Unable to stop encoder (output)");
	}

	if (video_stream_stop(encoder_dev, VIDEO_BUF_TYPE_INPUT)) {
		LOG_ERR("Unable to stop encoder (input)");
	}
}
#endif

int main(void)
{
	struct sockaddr_in addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	struct video_buffer *buffers[CONFIG_VIDEO_CAPTURE_N_BUFFERING];
	struct video_buffer *vbuf = &(struct video_buffer){};
#if DT_HAS_CHOSEN(zephyr_videoenc)
	struct video_buffer *vbuf_out = &(struct video_buffer){};
#endif
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

	/* Alloc Buffers */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		/*
		 * For some hardwares, such as the PxP used on i.MX RT1170 to do image rotation,
		 * buffer alignment is needed in order to achieve the best performance
		 */
		buffers[i] = video_buffer_aligned_alloc(fmt.size, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
							K_NO_WAIT);
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

#if DT_HAS_CHOSEN(zephyr_videoenc)
		if (configure_encoder()) {
			LOG_ERR("Unable to configure video encoder");
			return 0;
		}
#endif

		/* Enqueue Buffers */
		for (i = 0; i < ARRAY_SIZE(buffers); i++) {
			ret = video_enqueue(video_dev, buffers[i]);
			if (ret) {
				LOG_ERR("Unable to enqueue video buf");
				return 0;
			}
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

#if DT_HAS_CHOSEN(zephyr_videoenc)
			encode_frame(vbuf, &vbuf_out);

			LOG_INF("Sending compressed frame %d (size=%d bytes)", i++,
				vbuf_out->bytesused);
			/* Send compressed video buffer to TCP client */
			ret = sendall(client, vbuf_out->buffer, vbuf_out->bytesused);

			vbuf_out->type = VIDEO_BUF_TYPE_OUTPUT;
			ret = video_enqueue(encoder_dev, vbuf_out);
			if (ret) {
				LOG_ERR("Unable to enqueue encoder output buf");
				return 0;
			}

#else
			LOG_INF("Sending frame %d", i++);
			/* Send video buffer to TCP client */
			ret = sendall(client, vbuf->buffer, vbuf->bytesused);
#endif
			if (ret && ret != -EAGAIN) {
				/* client disconnected */
				LOG_ERR("TCP: Client disconnected %d", ret);
				close(client);
			}

			vbuf->type = VIDEO_BUF_TYPE_INPUT;
			ret = video_enqueue(video_dev, vbuf);
			if (ret) {
				LOG_ERR("Unable to enqueue video buf");
				return 0;
			}
		} while (!ret);

		/* stop capture */
		if (video_stream_stop(video_dev, type)) {
			LOG_ERR("Unable to stop video");
			return 0;
		}

#if DT_HAS_CHOSEN(zephyr_videoenc)
		stop_encoder();
#endif

		/* Flush remaining buffers */
		do {
			vbuf->type = VIDEO_BUF_TYPE_INPUT;
			ret = video_dequeue(video_dev, &vbuf, K_NO_WAIT);
		} while (!ret);

	} while (1);
}
