/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
static struct pcap *pcap = NULL;
static int fd1_in, fd2_in;

struct pcap_hdr {
	uint32_t magic_number;  /* magic number */
	uint16_t version_major; /* major version number */
	uint16_t version_minor; /* minor version number */
	int32_t  thiszone;      /* GMT to local correction */
	uint32_t sigfigs;       /* accuracy of timestamps */
	uint32_t snaplen;       /* max length of captured packets, in octets */
	uint32_t network;       /* data link type */
} __attribute__ ((packed));
#define PCAP_HDR_SIZE (sizeof(struct pcap_hdr))

struct pcap_pkt {
	uint32_t ts_sec;        /* timestamp seconds */
	uint32_t ts_usec;       /* timestamp microseconds */
	uint32_t incl_len;      /* number of octets of packet saved in file */
	uint32_t orig_len;      /* actual length of packet */
} __attribute__ ((packed));
#define PCAP_PKT_SIZE (sizeof(struct pcap_pkt))

struct pcap {
	int fd;
	bool closed;
	uint32_t type;
	uint32_t snaplen;
};

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

int log_init(const char *debug, gboolean detach)
{
	int option = LOG_NDELAY | LOG_PID | LOG_PERROR;
	const char *name = NULL, *file = NULL;

	openlog("monitor", option, LOG_DAEMON);

	syslog(LOG_INFO, "monitor 15.4");

	return 0;
}

void log_cleanup(void)
{
	syslog(LOG_INFO, "Exit");

	closelog();
}

struct pcap *pcap_open(const char *pathname)
{
	struct pcap *pcap;
	struct pcap_hdr hdr;
	ssize_t len;

	pcap = g_new0(struct pcap, 1);

	pcap->fd = open(pathname, O_RDONLY | O_CLOEXEC);
	if (pcap->fd < 0) {
		DBG("Failed to open PCAP file");
		g_free(pcap);
		return NULL;
	}

	len = read(pcap->fd, &hdr, PCAP_HDR_SIZE);
	if (len < 0) {
		DBG("Failed to read PCAP header");
		goto failed;
	}

	if (len != PCAP_HDR_SIZE) {
		DBG("Wrong PCAP header size\n");
		goto failed;
	}

	if (hdr.magic_number != 0xa1b2c3d4) {
		DBG("Wrong PCAP header magic\n");
		goto failed;
	}

	if (hdr.version_major != 2 || hdr.version_minor != 4) {
		DBG("Wrong PCAP version number\n");
		goto failed;
	}

	pcap->closed = false;
	pcap->snaplen = hdr.snaplen;
	pcap->type = hdr.network;

	return pcap;

failed:
	close(pcap->fd);
	g_free(pcap);

	return NULL;
}

void pcap_close(struct pcap *pcap)
{
	if (!pcap)
		return;

	if (pcap->fd >= 0)
		close(pcap->fd);

	g_free(pcap);
}

struct pcap *pcap_create(const char *pathname)
{
	struct pcap *pcap;
	struct pcap_hdr hdr;
	ssize_t len;

	pcap = g_new0(struct pcap, 1);
	pcap->fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (pcap->fd < 0) {
		DBG("Failed to create PCAP file");
		g_free(pcap);
		return NULL;
	}

	pcap->closed = false;
	pcap->snaplen = 0x0000ffff;
				 /* http://www.tcpdump.org/linktypes.html */
	pcap->type = 0x000000E6; /* LINKTYPE_IEEE802_15_4_NOFCS : 230 */

	memset(&hdr, 0, sizeof(hdr));
	hdr.magic_number = 0xa1b2c3d4;
	hdr.version_major = 0x0002;
	hdr.version_minor = 0x0004;
	hdr.thiszone = 0;
	hdr.sigfigs = 0;
	hdr.snaplen = pcap->snaplen;
	hdr.network = pcap->type;

	len = write(pcap->fd, &hdr, PCAP_HDR_SIZE);
	if (len < 0) {
		DBG("Failed to write PCAP header");
		goto failed;
	}

	if (len != PCAP_HDR_SIZE) {
		DBG("Written PCAP header size mimatch\n");
		goto failed;
	}

	return pcap;

failed:
	close(pcap->fd);
	g_free(pcap);

	return NULL;
}

bool pcap_write(struct pcap *pcap, const struct timeval *tv,
				const void *data, uint32_t size)
{
	struct iovec iov[3];
	struct pcap_pkt pkt;
	ssize_t written;

	if (!pcap)
		return false;

	if (pcap->closed)
		return false;

	memset(&pkt, 0, sizeof(pkt));
	if (tv) {
		pkt.ts_sec = tv->tv_sec;
		pkt.ts_usec = tv->tv_usec;
	}

	pkt.incl_len = size;
	pkt.orig_len = size;

	iov[0].iov_base = &pkt;
	iov[0].iov_len = PCAP_PKT_SIZE;
	iov[1].iov_base = (void *) data;
	iov[1].iov_len = size;

	written = writev(pcap->fd, iov, 2);
	if (written < 0) {
		pcap->closed = true;
		return false;
	}

	if (written < (ssize_t) (PCAP_PKT_SIZE + size)) {
		pcap->closed = true;
		return false;
	}

	return true;
}

static gboolean fifo_handler1(GIOChannel *channel, GIOCondition cond,
							gpointer user_data)
{
	guint buf[1];
	ssize_t result;
	int fd;
	struct timeval tv;

	if (cond & (G_IO_NVAL | G_IO_ERR | G_IO_HUP)) {
		DBG("something wrong");
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	fd = g_io_channel_unix_get_fd(channel);

	result = read(fd, &buf, 1);
	if (result != 1) {
		DBG("Failed to read %lu", result);
		return FALSE;
	}

	if (write(fd2_in, buf, 1) < 0) {
		DBG("Error, write failed to %s", pipe_2_in);
		return FALSE;
	}

	if (input1_len == 0 && input1_offset == 0 &&
		buf[0] == DUMMY_RADIO_15_4_FRAME_TYPE) {
		input1_type = buf[0];
		goto done;
	}

	if (input1_len == 0 && input1_offset == 0 &&
		input1_type == DUMMY_RADIO_15_4_FRAME_TYPE) {
		input1_len = buf[0];
		goto done;
	}

	if (input1_len) {
		input1[input1_offset++] = buf[0];
	}

	if (input1_len && input1_len == input1_offset) {
		DBG("Received %d, %d bytes", input1_len, input1_offset);

		/* write it to pcap file */
		gettimeofday(&tv, NULL);
		pcap_write(pcap, &tv, input1, input1_len);
		input1_len = input1_offset = 0;
		memset(input1, 0, sizeof(input1));
	}

done:
	return TRUE;
}

static gboolean fifo_handler2(GIOChannel *channel, GIOCondition cond,
							gpointer user_data)
{
	guint buf[1];
	ssize_t result;
	int fd;
	struct timeval tv;

	if (cond & (G_IO_NVAL | G_IO_ERR | G_IO_HUP)) {
		DBG("something wrong");
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	fd = g_io_channel_unix_get_fd(channel);

	result = read(fd, &buf, 1);
	if (result != 1) {
		DBG("Failed to read %lu", result);
		return FALSE;
	}

	if (write(fd1_in, buf, 1) < 0) {
		DBG("Error, write failed to %s", pipe_1_in);
		return FALSE;
	}

	if (input2_len == 0 && input2_offset == 0 &&
		buf[0] == DUMMY_RADIO_15_4_FRAME_TYPE) {
		input2_type = buf[0];
		goto done;
	}

	if (input2_len == 0 && input2_offset == 0 &&
		input2_type == DUMMY_RADIO_15_4_FRAME_TYPE) {
		input2_len = buf[0];
		goto done;
	}

	if (input2_len) {
		input2[input2_offset++] = buf[0];
	}

	if (input2_len && input2_len == input2_offset) {
		DBG("Received %d, %d bytes", input2_len, input2_offset);

		/* write it to pcap file */
		gettimeofday(&tv, NULL);
		pcap_write(pcap, &tv, input2, input2_len);
		input2_len = input2_offset = 0;
		memset(input2, 0, sizeof(input2));
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

	if (argc == 4)
		pipe2 = argv[3];

	main_loop = g_main_loop_new(NULL, FALSE);

	pipe_1_in = g_strconcat(pipe1, PIPE_IN, NULL);
	pipe_1_out = g_strconcat(pipe1, PIPE_OUT, NULL);
	pipe_2_in = g_strconcat(pipe2, PIPE_IN, NULL);
	pipe_2_out = g_strconcat(pipe2, PIPE_OUT, NULL);
	path = g_strdup(argv[1]);

	pcap = pcap_create(path);
	if (!pcap) {
		g_free(path);
		exit(-EINVAL);
	}

	log_init("log", FALSE);

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
	log_cleanup();
	g_main_loop_unref(main_loop);

	exit(ret);
}
