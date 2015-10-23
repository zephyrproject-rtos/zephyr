/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define EXTRA_DEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/uio.h>
#include <sys/signalfd.h>
#include <sys/time.h>
#include <linux/if_ether.h>
#include <glib.h>
#include <pcap/pcap.h>

#define PIPE_IN		".in"
#define PIPE_OUT	".out"
#define DUMMY_RADIO_15_4_FRAME_TYPE     0xF0

static uint8_t input1[128];
static uint8_t input1_len, input1_offset, input1_type;

static uint8_t input2[128];
static uint8_t input2_len, input2_offset, input2_type;

static GMainLoop *main_loop = NULL;
static char *path = NULL;
static char *pipe_1_in = NULL, *pipe_1_out = NULL;
static char *pipe_2_in = NULL, *pipe_2_out = NULL;
static struct pcap_fd *pcap;
static int fd1_in, fd2_in;

struct pcap_fd {
	int fd;
};

#define PCAP_FILE_HDR_SIZE (sizeof(struct pcap_file_header))

struct pcap_frame {
	uint32_t ts_sec;
	uint32_t ts_usec;
	uint32_t caplen;
	uint32_t len;
} __attribute__ ((packed));
#define PCAP_FRAME_SIZE (sizeof(struct pcap_frame))

struct debug_desc {
	const char *name;
	const char *file;
#define DEBUG_FLAG_DEFAULT (0)
#define DEBUG_FLAG_PRINT   (1 << 0)
	unsigned int flags;
} __attribute__((aligned(8)));

void debug(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	vsyslog(LOG_DEBUG, format, ap);

	va_end(ap);
}

#define DBG(fmt, arg...) do { \
	debug("%s:%s() " fmt, __FILE__, __FUNCTION__ , ## arg); \
} while (0)

int log_init(const char *debug, gboolean detach, gboolean perror)
{
	int option = LOG_NDELAY | LOG_PID;
	const char *name = NULL, *file = NULL;

	if (perror)
		 option |= LOG_PERROR;

	openlog("monitor", option, LOG_DAEMON);

	syslog(LOG_INFO, "monitor 15.4");

	return 0;
}

void log_cleanup(void)
{
	syslog(LOG_INFO, "Exit");

	closelog();
}

void monitor_pcap_free(void)
{
	if (!pcap)
		return;

	if (pcap->fd >= 0)
		close(pcap->fd);

	g_free(pcap);
}

bool monitor_pcap_create(const char *pathname)
{
	struct pcap_file_header hdr;
	ssize_t len;

	pcap = g_new0(struct pcap_fd, 1);
	pcap->fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (pcap->fd < 0) {
		DBG("Failed to create PCAP file");
		g_free(pcap);
		return false;
	}

	memset(&hdr, 0, PCAP_FILE_HDR_SIZE);
	hdr.magic = 0xa1b2c3d4;
	hdr.version_major = 0x0002;
	hdr.version_minor = 0x0004;
	hdr.thiszone = 0;
	hdr.sigfigs = 0;
	hdr.snaplen = 0x0000ffff;
	/*
	 * http://www.tcpdump.org/linktypes.html
	 * LINKTYPE_IEEE802_15_4_NOFCS : 230
	 */
	hdr.linktype = 0x000000E6;

	len = write(pcap->fd, &hdr, PCAP_FILE_HDR_SIZE);
	if (len < 0) {
		DBG("Failed to write PCAP header");
		goto failed;
	}

	if (len != PCAP_FILE_HDR_SIZE) {
		DBG("Written PCAP header size mimatch\n");
		goto failed;
	}

	return true;

failed:
	monitor_pcap_free();

	return false;
}

bool monitor_pcap_write(const void *data, uint32_t size)
{
	struct pcap_frame frame;
	struct timeval tv;
	ssize_t len;

	if (!pcap)
		return false;

	memset(&frame, 0, PCAP_FRAME_SIZE);

	gettimeofday(&tv, NULL);
	frame.ts_sec = tv.tv_sec;
	frame.ts_usec = tv.tv_usec;
	frame.caplen = size;
	frame.len = size;

	len = write(pcap->fd, &frame, PCAP_FRAME_SIZE);
	if (len < 0 || len != PCAP_FRAME_SIZE) {
		DBG("Failed to write PCAP frame or size mismatch %d\n", len);
		return false;
	}

	fsync(pcap->fd);

	len = write(pcap->fd, data, size);
	if (len < 0 || len != size) {
		DBG("Failed to write PCAP data or size mismatch %d\n", len);
		return false;
	}

	fsync(pcap->fd);
	return true;
}

static gboolean fifo_handler1(GIOChannel *channel, GIOCondition cond,
							gpointer user_data)
{
	static bool starting = true;
	unsigned char buf[1];
	ssize_t result;
	int fd;

	if (cond & (G_IO_NVAL | G_IO_ERR | G_IO_HUP)) {
		DBG("First pipe closed");
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	fd = g_io_channel_unix_get_fd(channel);

	result = read(fd, buf, 1);
	if (result != 1) {
		DBG("Failed to read %lu", result);
		return FALSE;
	}

	if (write(fd2_in, buf, 1) < 0) {
		DBG("Error, write failed to %s", pipe_2_in);
		return FALSE;
	}
#if EXTRA_DEBUG
	DBG("[%d/%d] starting %d fifo 1 read from %d and wrote %d bytes to %d",
	    input1_offset, input1_len, starting, fd, 1, fd2_in);
#endif

	if (starting && input1_len == 0 && input1_offset == 0 &&
	    buf[0] == 0 && input1_type == 0) {
		/* sync byte */
		goto done;
	} else if (starting && input1_len == 0 && input1_offset == 0
		   && buf[0] != 0) {
		starting = false;
	}

#if EXTRA_DEBUG
	DBG("Pipe 1 byte 0x%x", buf[0]);
#endif
	if (input1_len == 0 && input1_offset == 0 &&
		buf[0] == DUMMY_RADIO_15_4_FRAME_TYPE) {
		input1_type = buf[0];
#if EXTRA_DEBUG
		DBG("Frame starting in pipe 1");
#endif
		goto done;
	}

	if (input1_len == 0 && input1_offset == 0 &&
		input1_type == DUMMY_RADIO_15_4_FRAME_TYPE) {
		input1_len = buf[0];
#if EXTRA_DEBUG
		DBG("Expecting pipe 1 buf len %d\n", input1_len);
#endif
		goto done;
	}

	if (input1_len) {
		input1[input1_offset++] = buf[0];
	}

	if (input1_len && input1_len == input1_offset) {
		DBG("Received %d bytes in pipe 1", input1_len);

		monitor_pcap_write(input1, input1_len);
		input1_len = input1_offset = 0;
		memset(input1, 0, sizeof(input1));

		fsync(fd2_in);
	}

done:
	return TRUE;
}

static gboolean fifo_handler2(GIOChannel *channel, GIOCondition cond,
							gpointer user_data)
{
	static bool starting = true;
	unsigned char buf[1];
	ssize_t result;
	int fd;

	if (cond & (G_IO_NVAL | G_IO_ERR | G_IO_HUP)) {
		DBG("Second pipe closed");
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	fd = g_io_channel_unix_get_fd(channel);

	result = read(fd, buf, 1);
	if (result != 1) {
		DBG("Failed to read %lu", result);
		return FALSE;
	}

	if (write(fd1_in, buf, 1) < 0) {
		DBG("Error, write failed to %s", pipe_1_in);
		return FALSE;
	}
#if EXTRA_DEBUG
	DBG("[%d/%d] starting %d fifo 2 read from %d and wrote %d bytes to %d",
	    input2_offset, input2_len, starting, fd, 1, fd1_in);
#endif
	if (starting && input2_len == 0 && input2_offset == 0 &&
	    buf[0] == 0 && input2_type == 0) {
		/* sync byte */
		goto done;
	} else if (starting && input2_len == 0 && input2_offset == 0
		   && buf[0] != 0) {
		starting = false;
	}

#if EXTRA_DEBUG
	DBG("Pipe 2 byte 0x%x", buf[0]);
#endif
	if (input2_len == 0 && input2_offset == 0 &&
		buf[0] == DUMMY_RADIO_15_4_FRAME_TYPE) {
#if EXTRA_DEBUG
		DBG("Frame starting in pipe 2");
#endif
		input2_type = buf[0];
		goto done;
	}

	if (input2_len == 0 && input2_offset == 0 &&
		input2_type == DUMMY_RADIO_15_4_FRAME_TYPE) {
		input2_len = buf[0];
#if EXTRA_DEBUG
		DBG("Expecting pipe 2 buf len %d\n", input2_len);
#endif
		goto done;
	}

	if (input2_len) {
		input2[input2_offset++] = buf[0];
	}

	if (input2_len && input2_len == input2_offset) {
		DBG("Received %d bytes in pipe 2", input2_len);

		monitor_pcap_write(input2, input2_len);
		input2_len = input2_offset = 0;
		memset(input2, 0, sizeof(input2));

		fsync(fd1_in);
	}

done:
	return TRUE;
}

static int setup_fifofd1(void)
{
	GIOChannel *channel;
	guint source;
	int fd;

	fd = open(pipe_1_out, O_RDONLY);
	if (fd < 0) {
		DBG("Failed to open fifo %s", pipe_1_out);
		return fd;
	}

	DBG("Pipe 1 OUT fd %d", fd);

	channel = g_io_channel_unix_new(fd);

	g_io_channel_set_close_on_unref(channel, TRUE);

	source = g_io_add_watch(channel,
				G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
				fifo_handler1, NULL);

	g_io_channel_unref(channel);

	return source;
}

static int setup_fifofd2(void)
{
	GIOChannel *channel;
	guint source;
	int fd;

	fd = open(pipe_2_out, O_RDONLY);
	if (fd < 0) {
		DBG("Failed to open fifo %s", pipe_2_out);
		return fd;
	}

	DBG("Pipe 2 OUT fd %d", fd);

	channel = g_io_channel_unix_new(fd);

	g_io_channel_set_close_on_unref(channel, TRUE);

	source = g_io_add_watch(channel,
				G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
				fifo_handler2, NULL);

	g_io_channel_unref(channel);

	return source;
}

int main(int argc, char *argv[])
{
	int fifo1, fifo2, ret;
	char *pipe1 = "/tmp/ip-15-4-1";
	char *pipe2 = "/tmp/ip-15-4-2";

	if (argc < 2) {
		printf("Usage: %s <pcapfile> [<pipe_1> <pipe_2>]\n", argv[0]);
		printf("   e.g.: monitor_15_4 sample.pcap [/tmp/ip-15-4-1 /tmp/ip-15-4-2]\n");
		exit(-EINVAL);
	}

	if (argc == 3)
		pipe1 = argv[2];

	if (argc == 4) {
		pipe1 = argv[2];
		pipe2 = argv[3];
	}

	main_loop = g_main_loop_new(NULL, FALSE);

	pipe_1_in = g_strconcat(pipe1, PIPE_IN, NULL);
	pipe_1_out = g_strconcat(pipe1, PIPE_OUT, NULL);
	pipe_2_in = g_strconcat(pipe2, PIPE_IN, NULL);
	pipe_2_out = g_strconcat(pipe2, PIPE_OUT, NULL);
	path = g_strdup(argv[1]);

	log_init("log", FALSE, argc > 4 ? TRUE : FALSE);

	DBG("Pipe 1 IN %s OUT %s", pipe_1_in, pipe_1_out);
	DBG("Pipe 2 IN %s OUT %s", pipe_2_in, pipe_2_out);

	if (!monitor_pcap_create(path)) {
		g_free(path);
		exit(-EINVAL);
	}

	fifo1 = setup_fifofd1();
	if (fifo1 < 0) {
		ret = -EINVAL;
		goto exit;
	}

	fifo2 = setup_fifofd2();
	if (fifo2 < 0) {
		ret = -EINVAL;
		goto exit;
	}

	fd1_in = open(pipe_1_in, O_WRONLY);
	if (fd1_in < 0) {
		DBG("Failed to open fifo %s", pipe_1_in);
		ret = -EINVAL;
		goto exit;
	}

	fd2_in = open(pipe_2_in, O_WRONLY);
	if (fd2_in < 0) {
		DBG("Failed to open fifo %s", pipe_2_in);
		ret = -EINVAL;
		goto exit;
	}

	DBG("Pipe 1 IN %d, pipe 2 IN %d", fd1_in, fd2_in);

	g_main_loop_run(main_loop);
	ret = 0;
exit:
	g_source_remove(fifo1);
	g_source_remove(fifo2);
	close(fd1_in);
	close(fd2_in);
	g_free(path);
	g_free(pipe_1_in);
	g_free(pipe_1_out);
	g_free(pipe_2_in);
	g_free(pipe_2_out);
	monitor_pcap_free();
	log_cleanup();
	g_main_loop_unref(main_loop);

	exit(ret);
}
