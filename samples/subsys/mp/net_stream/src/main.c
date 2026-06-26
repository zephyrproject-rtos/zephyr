/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MediaPipe MJPEG-over-TCP network streaming sample.
 *
 * Reads an MJPEG file from the filesystem and streams the raw bytes over TCP.
 * The client side is responsible for parsing and decoding frames.
 *
 * Pipeline:
 *   zfilesrc --> ztcpsink
 *
 * External client (TAP networking):
 *   ffplay -f mjpeg tcp://192.0.2.1:5000
 *
 * Self-test (loopback, CONFIG_SAMPLE_NET_STREAM_SELFTEST=y):
 *   A built-in client thread connects, receives all bytes, and validates
 *   JPEG SOI/EOI markers.  Exits with pass/fail log.
 */

#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <zephyr/mp/core/mp.h>
#include <zephyr/mp/zfs/mp_zfilesrc.h>
#include <zephyr/mp/znet/mp_ztcpsink.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define PIPE_ID     0
#define FILE_SRC_ID 1
#define TCP_SINK_ID 2

#define MNT_POINT  "/SD:"
#define INPUT_FILE "sample.mjp"

static FATFS fat_fs;
static struct fs_mount_t mnt = {
	.type      = FS_FATFS,
	.fs_data   = &fat_fs,
	.mnt_point = MNT_POINT,
};

static struct mp_pipeline pipe;
static struct mp_zfilesrc  filesrc;
static struct mp_ztcpsink  tcpsink;

/* Self-test client (only built when CONFIG_SAMPLE_NET_STREAM_SELFTEST=y) */
#if defined(CONFIG_SAMPLE_NET_STREAM_SELFTEST)

#define CLIENT_STACK_SIZE 4096
#define CLIENT_PRIORITY   5
#define CLIENT_RECV_BUF   4096

static K_THREAD_STACK_DEFINE(client_stack, CLIENT_STACK_SIZE);
static struct k_thread client_thread;

static uint8_t client_recv_buf[CLIENT_RECV_BUF];

static void selftest_client(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

	/* Give the server time to bind and listen */
	k_sleep(K_MSEC(500));

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port   = htons(CONFIG_SAMPLE_NET_STREAM_PORT),
	};
	zsock_inet_pton(AF_INET, CONFIG_SAMPLE_NET_STREAM_SELFTEST_ADDR,
			&addr.sin_addr);

	int fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (fd < 0) {
		LOG_ERR("[selftest] socket() failed: %d", errno);
		return;
	}

	if (zsock_connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOG_ERR("[selftest] connect() failed: %d", errno);
		zsock_close(fd);
		return;
	}
	LOG_INF("[selftest] Connected to server");

	/* Scan raw byte stream for JPEG SOI (0xFF 0xD8) and EOI (0xFF 0xD9) */
	int frames = 0;
	ssize_t rd;
	bool in_frame = false;
	uint8_t prev = 0;

	while ((rd = zsock_recv(fd, client_recv_buf, sizeof(client_recv_buf), 0)) > 0) {
		for (ssize_t i = 0; i < rd; i++) {
			uint8_t b = client_recv_buf[i];

			if (!in_frame && prev == 0xFF && b == 0xD8) {
				in_frame = true;
			} else if (in_frame && prev == 0xFF && b == 0xD9) {
				frames++;
				LOG_DBG("[selftest] Frame %d complete", frames);
				in_frame = false;
			}
			prev = b;
		}
	}

	zsock_close(fd);

	if (frames > 0) {
		LOG_INF("[selftest] PASS - received %d valid JPEG frames", frames);
	} else {
		LOG_ERR("[selftest] FAIL - no valid JPEG frames received");
	}
}

static void start_selftest_client(void)
{
	k_thread_create(&client_thread, client_stack,
			K_THREAD_STACK_SIZEOF(client_stack),
			selftest_client, NULL, NULL, NULL,
			CLIENT_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&client_thread, "selftest_client");
}

#endif /* CONFIG_SAMPLE_NET_STREAM_SELFTEST */

int main(void)
{
	int ret;
	uint16_t port = CONFIG_SAMPLE_NET_STREAM_PORT;

	ret = fs_mount(&mnt);
	if (ret != 0) {
		LOG_ERR("Failed to mount disk (%d)", ret);
		return ret;
	}
	LOG_INF("Filesystem mounted at %s", MNT_POINT);

	MP_ELEMENT_INIT(&pipe,    mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&filesrc, mp_zfilesrc_init, FILE_SRC_ID);
	MP_ELEMENT_INIT(&tcpsink, mp_ztcpsink_init, TCP_SINK_ID);

	ret = mp_object_set_properties((struct mp_object *)&filesrc,
				       PROP_ZFILESRC_PATH, MNT_POINT "/" INPUT_FILE,
				       PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set filesrc path (%d)", ret);
		goto err;
	}

	ret = mp_object_set_properties((struct mp_object *)&tcpsink,
				       PROP_ZTCPSINK_PORT, &port,
				       PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set tcpsink port (%d)", ret);
		goto err;
	}

	if (mp_bin_add((struct mp_bin *)&pipe,
		       (struct mp_element *)&filesrc,
		       (struct mp_element *)&tcpsink,
		       NULL) != 0) {
		LOG_ERR("Failed to add elements to pipeline");
		goto err;
	}

	if (mp_element_link((struct mp_element *)&filesrc,
			    (struct mp_element *)&tcpsink,
			    NULL) != 0) {
		LOG_ERR("Failed to link pipeline elements");
		goto err;
	}

	LOG_INF("Pipeline: filesrc -> ztcpsink (port %u)", port);

#if defined(CONFIG_SAMPLE_NET_STREAM_SELFTEST)
	start_selftest_client();
	LOG_INF("[selftest] Self-test client started");
#else
	LOG_INF("Connect with: ffplay -f mjpeg tcp://192.0.2.1:%u", port);
#endif

	if (mp_element_set_state((struct mp_element *)&pipe, MP_STATE_PLAYING)
	    != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start pipeline");
		goto err;
	}

	struct mp_bus *bus = mp_element_get_bus((struct mp_element *)&pipe);
	struct mp_message msg;

	if (mp_bus_pop_msg(bus, MP_MESSAGE_ERROR | MP_MESSAGE_EOS, &msg) == 0) {
		switch (msg.type) {
		case MP_MESSAGE_EOS:
			LOG_INF("End of stream");
			break;
		case MP_MESSAGE_ERROR:
			LOG_ERR("Pipeline error from element %d", msg.origin->object.id);
			break;
		default:
			LOG_ERR("Unexpected message type from element %d", msg.origin->object.id);
			break;
		}
	}

	mp_element_set_state((struct mp_element *)&pipe, MP_STATE_READY);

err:
	fs_unmount(&mnt);
	return 0;
}
