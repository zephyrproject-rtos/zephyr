/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ff.h>

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/mpipe_message.h>
#include <zephyr/mpipe/fs/mpipe_file_src.h>
#include <zephyr/mpipe/net/mpipe_tcp_server_sink.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

enum {
	PIPE_ID,
	FILE_SRC_ID,
	TCP_SINK_ID,
};

#define MNT_POINT  "/SD:"
#define INPUT_FILE MNT_POINT "/sample.mjp"

static FATFS fat_fs;
static struct fs_mount_t mnt = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = MNT_POINT,
};

ZBUS_MSG_SUBSCRIBER_DEFINE(main_sub);

static struct mpipe pipeline;
static struct mpipe_file_src file_src;
static struct mpipe_tcp_server_sink tcp_sink;

#ifdef CONFIG_SAMPLE_NET_STREAM_SELFTEST

#define SELFTEST_FRAMES 3

/* A JPEG frame holding a comment segment instead of image data */
static const uint8_t jpeg_frame[] = {
	0xFF, 0xD8,                                           /* SOI */
	0xFF, 0xFE, 0x00, 0x08, 'm', 'p', 't', 'e', 's', 't', /* COM */
	0xFF, 0xD9,                                           /* EOI */
};

static int write_input_file(void)
{
	struct fs_file_t fh;
	int ret;

	fs_file_t_init(&fh);
	ret = fs_open(&fh, INPUT_FILE, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (ret != 0) {
		return ret;
	}

	for (int i = 0; i < SELFTEST_FRAMES && ret == 0; i++) {
		if (fs_write(&fh, jpeg_frame, sizeof(jpeg_frame)) != sizeof(jpeg_frame)) {
			ret = -EIO;
		}
	}

	(void)fs_close(&fh);

	return ret;
}

static void selftest_client(void *p1, void *p2, void *p3)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(CONFIG_MPIPE_NET_SINK_PORT),
	};
	uint8_t buf[256];
	uint8_t prev = 0;
	int frames = 0;
	ssize_t rd;
	int fd;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Let main reach accept() first */
	k_sleep(K_MSEC(500));

	(void)zsock_inet_pton(NET_AF_INET, "127.0.0.1", &addr.sin_addr);

	fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (fd < 0) {
		LOG_ERR("[selftest] FAIL: socket() failed (%d)", errno);
		return;
	}

	if (zsock_connect(fd, (struct net_sockaddr *)&addr, sizeof(addr)) < 0) {
		LOG_ERR("[selftest] FAIL: connect() failed (%d)", errno);
		(void)zsock_close(fd);
		return;
	}

	/* Count the EOI markers (FF D9) in the byte stream */
	while ((rd = zsock_recv(fd, buf, sizeof(buf), 0)) > 0) {
		for (ssize_t i = 0; i < rd; i++) {
			frames += (prev == 0xFF && buf[i] == 0xD9);
			prev = buf[i];
		}
	}

	(void)zsock_close(fd);

	if (frames == SELFTEST_FRAMES) {
		LOG_INF("[selftest] PASS: received %d frames", frames);
	} else {
		LOG_ERR("[selftest] FAIL: %d frames, expected %d", frames, SELFTEST_FRAMES);
	}
}

K_THREAD_DEFINE(selftest, 4096, selftest_client, NULL, NULL, NULL, 5, 0, -1);

#endif /* CONFIG_SAMPLE_NET_STREAM_SELFTEST */

int main(void)
{
	const struct zbus_channel *chan;
	struct zbus_channel *bus;
	struct mpipe_message msg;
	int ret;

	ret = fs_mount(&mnt);
	if (ret != 0) {
		LOG_ERR("Failed to mount %s (%d)", MNT_POINT, ret);
		return 0;
	}

#ifdef CONFIG_SAMPLE_NET_STREAM_SELFTEST
	ret = write_input_file();
	if (ret != 0) {
		LOG_ERR("Failed to write %s (%d)", INPUT_FILE, ret);
		goto unmount;
	}
#endif

	if (mpipe_pipeline_init(&pipeline, PIPE_ID) != 0 ||
	    mpipe_file_src_init(&file_src, FILE_SRC_ID) != 0 ||
	    mpipe_tcp_server_sink_init(&tcp_sink, TCP_SINK_ID) != 0 ||
	    mpipe_object_set_properties((struct mpipe_object *)&file_src, MPIPE_PROP_FS_SRC_PATH,
					INPUT_FILE, MPIPE_PROP_LIST_END) != 0 ||
	    mpipe_bin_add((struct mpipe_bin *)&pipeline, (struct mpipe_element *)&file_src,
			  (struct mpipe_element *)&tcp_sink, NULL) != 0 ||
	    mpipe_element_link((struct mpipe_element *)&file_src, (struct mpipe_element *)&tcp_sink,
			       NULL) != 0) {
		LOG_ERR("Failed to build the pipeline");
		goto unmount;
	}

	bus = mpipe_element_get_bus_chan((struct mpipe_element *)&pipeline);
	ret = zbus_chan_add_obs(bus, &main_sub, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to observe the pipeline bus (%d)", ret);
		goto unmount;
	}

	LOG_INF("Waiting for a client on port %u", CONFIG_MPIPE_NET_SINK_PORT);
	IF_ENABLED(CONFIG_SAMPLE_NET_STREAM_SELFTEST, (k_thread_start(selftest);))

	/* tcp_server_sink blocks here until a client connects */
	ret = mpipe_element_set_state((struct mpipe_element *)&pipeline, MPIPE_STATE_PLAYING);
	if (ret != 0) {
		LOG_ERR("Failed to start the pipeline (%d)", ret);
		goto rm_obs;
	}

	do {
		ret = zbus_sub_wait_msg(&main_sub, &chan, &msg, K_FOREVER);
	} while (ret == 0 && (msg.type & (MPIPE_MESSAGE_ERROR | MPIPE_MESSAGE_EOS)) == 0);

	if (ret != 0) {
		LOG_ERR("Failed to read the pipeline bus (%d)", ret);
	} else if (msg.type == MPIPE_MESSAGE_EOS) {
		LOG_INF("EOS from element %u", msg.origin->object.id);
	} else {
		LOG_ERR("Error %d from element %u", msg.code, msg.origin->object.id);
	}

	(void)mpipe_element_set_state((struct mpipe_element *)&pipeline, MPIPE_STATE_READY);
rm_obs:
	(void)zbus_chan_rm_obs(bus, &main_sub, K_FOREVER);
unmount:
	(void)fs_unmount(&mnt);

	return 0;
}
