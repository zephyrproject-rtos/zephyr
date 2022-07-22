/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>

#include <zephyr/net/socket.h>

#include <zephyr/drivers/video.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define MY_PORT 5000
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

void main(void)
{
	struct sockaddr_in addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	struct video_buffer *buffers[2], *vbuf;
	int i, ret, sock, client;
	struct video_format fmt;
	const struct device *video = DEVICE_DT_GET_ONE(nxp_imx_csi);

	/* Prepare Network */
	(void)memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MY_PORT);

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		LOG_ERR("Failed to create TCP socket: %d", errno);
		return;
	}

	ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Failed to bind TCP socket: %d", errno);
		close(sock);
		return;
	}

	ret = listen(sock, MAX_CLIENT_QUEUE);
	if (ret < 0) {
		LOG_ERR("Failed to listen on TCP socket: %d", errno);
		close(sock);
		return;
	}

	if (!device_is_ready(video)) {
		LOG_ERR("%s: device not ready.\n", video->name);
		return;
	}

	/* Get default/native format */
	if (video_get_format(video, VIDEO_EP_OUT, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return;
	}

	printk("Video device detected, format: %c%c%c%c %ux%u\n",
	       (char)fmt.pixelformat, (char)(fmt.pixelformat >> 8),
	       (char)(fmt.pixelformat >> 16), (char)(fmt.pixelformat >> 24),
	       fmt.width, fmt.height);

	/* Alloc Buffers */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		buffers[i] = video_buffer_alloc(fmt.pitch * fmt.height);
		if (buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return;
		}
	}

	/* Connection loop */
	do {
		printk("TCP: Waiting for client...\n");

		client = accept(sock, (struct sockaddr *)&client_addr,
				&client_addr_len);
		if (client < 0) {
			printk("Failed to accept: %d\n", errno);
			return;
		}

		printk("TCP: Accepted connection\n");

		/* Enqueue Buffers */
		for (i = 0; i < ARRAY_SIZE(buffers); i++) {
			video_enqueue(video, VIDEO_EP_OUT, buffers[i]);
		}

		/* Start video capture */
		if (video_stream_start(video)) {
			LOG_ERR("Unable to start video");
			return;
		}

		printk("Stream started\n");

		/* Capture loop */
		i = 0;
		do {
			ret = video_dequeue(video, VIDEO_EP_OUT, &vbuf,
					    K_FOREVER);
			if (ret) {
				LOG_ERR("Unable to dequeue video buf");
				return;
			}

			printk("\rSending frame %d", i++);

			/* Send video buffer to TCP client */
			ret = sendall(client, vbuf->buffer, vbuf->bytesused);
			if (ret && ret != -EAGAIN) {
				/* client disconnected */
				printk("\nTCP: Client disconnected %d\n", ret);
				close(client);
			}

			(void)video_enqueue(video, VIDEO_EP_OUT, vbuf);
		} while (!ret);

		/* stop capture */
		if (video_stream_stop(video)) {
			LOG_ERR("Unable to stop video");
			return;
		}

		/* Flush remaining buffers */
		do {
			ret = video_dequeue(video, VIDEO_EP_OUT,
					    &vbuf, K_NO_WAIT);
		} while (!ret);

	} while (1);
}
