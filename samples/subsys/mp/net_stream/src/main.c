/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MediaPipe MJPEG-over-TCP network streaming sample.
 *
 * Pipeline: filesrc -> tcpsink
 *
 * filesrc reads an MJPEG file from a mounted FAT volume and tcpsink pushes the
 * raw bytes to a single TCP client. Parsing and decoding are left to the
 * client, for instance:
 *
 *   ffplay -f mjpeg tcp://192.0.2.1:5000
 *
 * With CONFIG_SAMPLE_NET_STREAM_SELFTEST=y the sample generates its own input
 * file, runs a client thread over the loopback interface, and checks that the
 * expected number of JPEG frames comes back out.
 */

#include <errno.h>
#include <ff.h>

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <zephyr/mp/mp.h>
#include <zephyr/mp/fs/mp_filesrc.h>
#include <zephyr/mp/net/mp_tcpsink.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* Element IDs (values are arbitrary; only uniqueness within the pipeline matters) */
enum {
	PIPE_ID,
	FILE_SRC_ID,
	TCP_SINK_ID,
};

#define MNT_POINT  "/SD:"
#define INPUT_FILE "sample.mjp"

static FATFS fat_fs;

static struct fs_mount_t mnt = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = MNT_POINT,
};

static struct mp_pipeline pipe;
static struct mp_filesrc filesrc;
static struct mp_tcpsink tcpsink;

#if defined(CONFIG_SAMPLE_NET_STREAM_SELFTEST)

#define SELFTEST_FRAMES   3
#define CLIENT_STACK_SIZE 4096
#define CLIENT_PRIORITY   5
#define CLIENT_RECV_SIZE  1024

/* One JPEG frame carrying a comment segment instead of image data. Enough for
 * the self-test, which only checks that SOI/EOI framing survives the round trip.
 */
static const uint8_t jpeg_frame[] = {
	0xFF, 0xD8,             /* SOI */
	0xFF, 0xFE, 0x00, 0x08, /* COM, segment length 8 */
	'm',  'p',  't',  'e',  's',  't',
	0xFF, 0xD9,             /* EOI */
};

static K_THREAD_STACK_DEFINE(client_stack, CLIENT_STACK_SIZE);
static struct k_thread client_thread;
static uint8_t client_recv_buf[CLIENT_RECV_SIZE];

static int write_input_file(const char *path)
{
	struct fs_file_t fh;
	int ret;

	fs_file_t_init(&fh);

	ret = fs_open(&fh, path, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (ret != 0) {
		LOG_ERR("fs_open failed for %s (%d)", path, ret);
		return ret;
	}

	for (int i = 0; i < SELFTEST_FRAMES; i++) {
		ssize_t wr = fs_write(&fh, jpeg_frame, sizeof(jpeg_frame));

		if (wr != (ssize_t)sizeof(jpeg_frame)) {
			LOG_ERR("fs_write returned %d", (int)wr);
			(void)fs_close(&fh);
			return -EIO;
		}
	}

	ret = fs_close(&fh);
	if (ret != 0) {
		LOG_ERR("fs_close failed (%d)", ret);
		return ret;
	}

	LOG_INF("Wrote %d frames to %s", SELFTEST_FRAMES, path);

	return 0;
}

static void selftest_client(void *p1, void *p2, void *p3)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(CONFIG_SAMPLE_NET_STREAM_PORT),
	};
	int frames = 0;
	uint8_t prev = 0;
	bool in_frame = false;
	ssize_t rd;
	int fd;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Give the sink time to bind and listen */
	k_sleep(K_MSEC(500));

	(void)zsock_inet_pton(AF_INET, CONFIG_SAMPLE_NET_STREAM_SELFTEST_ADDR, &addr.sin_addr);

	fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0) {
		LOG_ERR("[selftest] socket() failed (%d)", errno);
		return;
	}

	if (zsock_connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOG_ERR("[selftest] connect() failed (%d)", errno);
		(void)zsock_close(fd);
		return;
	}

	LOG_INF("[selftest] Connected to the sink");

	/* Count SOI (FF D8) / EOI (FF D9) pairs in the received byte stream */
	while ((rd = zsock_recv(fd, client_recv_buf, sizeof(client_recv_buf), 0)) > 0) {
		for (ssize_t i = 0; i < rd; i++) {
			uint8_t b = client_recv_buf[i];

			if (!in_frame && prev == 0xFF && b == 0xD8) {
				in_frame = true;
			} else if (in_frame && prev == 0xFF && b == 0xD9) {
				in_frame = false;
				frames++;
			}

			prev = b;
		}
	}

	(void)zsock_close(fd);

	if (frames == SELFTEST_FRAMES) {
		LOG_INF("[selftest] PASS: received %d frames", frames);
	} else {
		LOG_ERR("[selftest] FAIL: received %d frames, expected %d", frames,
			SELFTEST_FRAMES);
	}
}

static void start_selftest_client(void)
{
	k_thread_create(&client_thread, client_stack, K_THREAD_STACK_SIZEOF(client_stack),
			selftest_client, NULL, NULL, NULL, CLIENT_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&client_thread, "selftest_client");
}

#endif /* CONFIG_SAMPLE_NET_STREAM_SELFTEST */

int main(void)
{
	uint16_t port = CONFIG_SAMPLE_NET_STREAM_PORT;
	struct mp_message msg;
	struct mp_bus *bus;
	int ret;

	ret = fs_mount(&mnt);
	if (ret != 0) {
		LOG_ERR("Failed to mount disk (%d)", ret);
		goto err;
	}

	LOG_INF("Filesystem mounted at %s", MNT_POINT);

#if defined(CONFIG_SAMPLE_NET_STREAM_SELFTEST)
	ret = write_input_file(MNT_POINT "/" INPUT_FILE);
	if (ret != 0) {
		goto unmount;
	}
#endif

	/* Build the pipeline */
	MP_ELEMENT_INIT(&pipe, mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&filesrc, mp_filesrc_init, FILE_SRC_ID);
	MP_ELEMENT_INIT(&tcpsink, mp_tcpsink_init, TCP_SINK_ID);

	ret = mp_object_set_properties((struct mp_object *)&filesrc, MP_PROP_FS_SRC_PATH,
				       MNT_POINT "/" INPUT_FILE, MP_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set the filesrc path (%d)", ret);
		goto unmount;
	}

	ret = mp_object_set_properties((struct mp_object *)&tcpsink, MP_PROP_NET_SINK_PORT, &port,
				       MP_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set the tcpsink port (%d)", ret);
		goto unmount;
	}

	/* Add elements to the pipeline - order does not matter */
	ret = mp_bin_add((struct mp_bin *)&pipe, (struct mp_element *)&filesrc,
			 (struct mp_element *)&tcpsink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto unmount;
	}

	/* Link elements together - order does matter */
	ret = mp_element_link((struct mp_element *)&filesrc, (struct mp_element *)&tcpsink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		goto unmount;
	}

	LOG_INF("Pipeline: filesrc -> tcpsink on port %u", port);

#if defined(CONFIG_SAMPLE_NET_STREAM_SELFTEST)
	start_selftest_client();
#else
	LOG_INF("Connect with: ffplay -f mjpeg tcp://192.0.2.1:%u", port);
#endif

	/* Start the pipeline. tcpsink blocks here until a client connects. */
	if (mp_element_set_state((struct mp_element *)&pipe, MP_STATE_PLAYING) !=
	    MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start the pipeline");
		goto unmount;
	}

	/* Handle message from the pipeline */
	bus = mp_element_get_bus((struct mp_element *)&pipe);
	mp_bus_pop_msg(bus, MP_MESSAGE_ERROR | MP_MESSAGE_EOS, &msg);

	switch (msg.type) {
	case MP_MESSAGE_EOS:
		LOG_INF("EOS message from element %d", msg.origin->object.id);
		break;
	case MP_MESSAGE_ERROR:
		LOG_ERR("ERROR message from element %d", msg.origin->object.id);
		break;
	default:
		LOG_ERR("Unexpected message from element %d", msg.origin->object.id);
		break;
	}

	/* Stop/Deinit the pipeline */
	(void)mp_element_set_state((struct mp_element *)&pipe, MP_STATE_READY);

unmount:
	ret = fs_unmount(&mnt);
	if (ret != 0) {
		LOG_ERR("Failed to unmount disk (%d)", ret);
	}

	LOG_INF("Done.");

	return 0;

err:
	LOG_ERR("Aborting sample");

	return 0;
}
