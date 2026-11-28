/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Host (bottom) half of the native simulator host video source: a FIFO or a V4L2 device.
 *
 * This file is built in the native simulator runner context: it uses the host C
 * library and must not include any Zephyr header.
 */

/* For F_SETPIPE_SZ and F_GETPIPE_SZ */
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <nsi_errno.h>
#include <nsi_tracing.h>

#include "video_native_sim_fifo_bottom.h"

/* Buffers queued on a V4L2 device, enough for the minimum some drivers need to stream */
#define VIDEO_NSI_V4L2_BUFFERS 4U

struct video_nsi_v4l2_dev {
	int fd;
	/* Mapped buffers, zero for a free slot */
	unsigned int count;
	void *mem[VIDEO_NSI_V4L2_BUFFERS];
	size_t len[VIDEO_NSI_V4L2_BUFFERS];
	/* The device streams YUYV, converted on the host to the RGB565 requested */
	bool yuyv_to_rgb565;
};

/* The device streaming on fd, or a free slot when fd is -1: the table grows as needed */
static struct video_nsi_v4l2_dev *video_nsi_v4l2_find(int fd)
{
	static struct video_nsi_v4l2_dev *devs;
	static size_t dev_count;
	struct video_nsi_v4l2_dev *grown;

	for (size_t i = 0; i < dev_count; i++) {
		if ((devs[i].count > 0U) ? (devs[i].fd == fd) : (fd == -1)) {
			return &devs[i];
		}
	}

	if (fd != -1) {
		return NULL;
	}

	grown = realloc(devs, (dev_count + 1U) * sizeof(*devs));
	if (grown == NULL) {
		return NULL;
	}

	devs = grown;
	(void)memset(&devs[dev_count], 0, sizeof(*devs));
	dev_count++;

	return &devs[dev_count - 1U];
}

static int video_nsi_v4l2_capture(struct video_nsi_v4l2_dev *dev, uint8_t *buf, size_t size);

int video_nsi_fifo_default_path_bottom(const char *name, char *buf, size_t size)
{
	int len = snprintf(buf, size, "/tmp/zephyr-%s-%ld.fifo", name, (long)getpid());

	return ((len < 0) || ((size_t)len >= size)) ? -1 : 0;
}

void video_nsi_fifo_unlink_bottom(const char *path)
{
	(void)unlink(path);
}

int video_nsi_fifo_open_bottom(const char *path, bool *created, bool *v4l2)
{
	struct stat statbuf;
	int fd;
	int err;

	/*
	 * Create the FIFO unconditionally and validate what was actually opened
	 * afterwards: inspecting the path before opening it would leave a window for it
	 * to be replaced in between (time-of-check to time-of-use race).
	 */
	if (strncmp(path, "/dev/", strlen("/dev/")) == 0) {
		/* A device path: no FIFO is created there, a missing device fails to open */
	} else if (mkfifo(path, 0666) == 0) {
		*created = true;
	} else if (errno != EEXIST) {
		err = errno;
		nsi_print_warning("Could not create the FIFO %s (%s)\n", path, strerror(err));
		return -nsi_errno_to_mid(err);
	}

	/*
	 * Opening read-only and non-blocking succeeds even when no writer is attached,
	 * and guarantees that no later read() can stall the simulated system.
	 */
	fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) {
		err = errno;
		nsi_print_warning("Could not open %s (%s)\n", path, strerror(err));
		return -nsi_errno_to_mid(err);
	}

	if (fstat(fd, &statbuf) != 0) {
		err = errno;
		nsi_print_warning("Could not stat %s (%s)\n", path, strerror(err));
		(void)close(fd);
		return -nsi_errno_to_mid(err);
	}

	if (S_ISFIFO(statbuf.st_mode)) {
		*v4l2 = false;
		return fd;
	}

	if (!S_ISCHR(statbuf.st_mode)) {
		nsi_print_warning("%s exists but is neither a FIFO nor a character device, it "
				  "cannot be used as video source\n",
				  path);
		(void)close(fd);
		return -nsi_errno_to_mid(EEXIST);
	}

	/* The buffers of a V4L2 device can only be mapped from a read-write descriptor */
	(void)close(fd);
	fd = open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) {
		err = errno;
		nsi_print_warning("Could not open the V4L2 device %s (%s)\n", path, strerror(err));
		return -nsi_errno_to_mid(err);
	}

	*v4l2 = true;

	return fd;
}

int video_nsi_fifo_grow_pipe_bottom(int fd, size_t size)
{
	int capacity = fcntl(fd, F_GETPIPE_SZ);

	if (capacity < 0) {
		return -nsi_errno_to_mid(errno);
	}

	/*
	 * An unprivileged process gets EPERM above /proc/sys/fs/pipe-max-size, 1 MiB by
	 * default, or once its pipes together reach /proc/sys/fs/pipe-user-pages-soft.
	 * The kernel rounds the size up to a power of two, so halving the request,
	 * rounded up, tries each smaller capacity until one is allowed.
	 */
	for (size_t req = size; req > (size_t)capacity; req = (req + 1) / 2) {
		int ret = fcntl(fd, F_SETPIPE_SZ, (int)req);

		if (ret >= 0) {
			return ret;
		}

		if (errno != EPERM) {
			return -nsi_errno_to_mid(errno);
		}
	}

	return capacity;
}

int video_nsi_fifo_read_bottom(int fd, uint8_t *buf, size_t frame_size, size_t *offset)
{
	struct video_nsi_v4l2_dev *dev = video_nsi_v4l2_find(fd);

	if (dev != NULL) {
		return video_nsi_v4l2_capture(dev, buf, frame_size);
	}

	while (*offset < frame_size) {
		ssize_t ret = read(fd, &buf[*offset], frame_size - *offset);

		if (ret > 0) {
			*offset += (size_t)ret;
		} else if (ret == 0) {
			/* All the writers closed the FIFO */
			return -nsi_errno_to_mid(EPIPE);
		} else if (errno != EINTR) {
			return -nsi_errno_to_mid(errno);
		}
	}

	return 0;
}

static int video_nsi_v4l2_set_fmt(int fd, struct v4l2_format *fmt, unsigned int pixelformat,
				  unsigned int width, unsigned int height)
{
	(void)memset(fmt, 0, sizeof(*fmt));
	fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt->fmt.pix.pixelformat = pixelformat;
	fmt->fmt.pix.width = width;
	fmt->fmt.pix.height = height;
	fmt->fmt.pix.field = V4L2_FIELD_NONE;

	if (ioctl(fd, VIDIOC_S_FMT, fmt) < 0) {
		int err = errno;

		nsi_print_warning("Could not set the format of the V4L2 device (%s)\n",
				  strerror(err));
		return -nsi_errno_to_mid(err);
	}

	return 0;
}

static void video_nsi_v4l2_stop(struct video_nsi_v4l2_dev *dev)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	struct v4l2_requestbuffers req = {
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.memory = V4L2_MEMORY_MMAP,
	};

	(void)ioctl(dev->fd, VIDIOC_STREAMOFF, &type);

	for (unsigned int i = 0U; i < dev->count; i++) {
		(void)munmap(dev->mem[i], dev->len[i]);
	}

	/* Release the device buffers, so that another process can stream from it */
	(void)ioctl(dev->fd, VIDIOC_REQBUFS, &req);
	dev->count = 0U;
}

int video_nsi_fifo_v4l2_start_bottom(int fd, unsigned int pixelformat, unsigned int width,
				     unsigned int height, unsigned int pitch)
{
	struct video_nsi_v4l2_dev *dev = video_nsi_v4l2_find(-1);
	struct v4l2_requestbuffers req = {
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.memory = V4L2_MEMORY_MMAP,
		.count = VIDEO_NSI_V4L2_BUFFERS,
	};
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	unsigned int devfmt = pixelformat;
	struct v4l2_format fmt;
	int err;

	if (dev == NULL) {
		nsi_print_warning("No memory for the state of the V4L2 device\n");
		return -nsi_errno_to_mid(ENOMEM);
	}

	err = video_nsi_v4l2_set_fmt(fd, &fmt, devfmt, width, height);
	/* RGB565, the default format, is rarely provided by webcams: convert it from YUYV */
	if ((err == 0) && (fmt.fmt.pix.pixelformat != devfmt) && (devfmt == V4L2_PIX_FMT_RGB565)) {
		devfmt = V4L2_PIX_FMT_YUYV;
		err = video_nsi_v4l2_set_fmt(fd, &fmt, devfmt, width, height);
	}
	if (err < 0) {
		return err;
	}

	/* V4L2 adjusts a format it cannot provide instead of rejecting it */
	if ((fmt.fmt.pix.pixelformat != devfmt) || (fmt.fmt.pix.width != width) ||
	    (fmt.fmt.pix.height != height) || (fmt.fmt.pix.bytesperline != pitch)) {
		nsi_print_warning("The V4L2 device does not provide %ux%u %.4s with %u bytes per "
				  "line, it offers %ux%u %.4s with %u: see "
				  "v4l2-ctl --list-formats-ext\n",
				  width, height, (const char *)&pixelformat, pitch,
				  fmt.fmt.pix.width, fmt.fmt.pix.height,
				  (const char *)&fmt.fmt.pix.pixelformat, fmt.fmt.pix.bytesperline);
		return -NSI_ERRNO_MID_ENOTSUP;
	}

	if ((ioctl(fd, VIDIOC_REQBUFS, &req) < 0) || (req.count == 0U)) {
		err = (req.count == 0U) ? ENOTSUP : errno;
		nsi_print_warning("The V4L2 device does not support memory mapped streaming (%s)\n",
				  strerror(err));
		return -nsi_errno_to_mid(err);
	}

	dev->fd = fd;
	dev->yuyv_to_rgb565 = (devfmt != pixelformat);

	/* The device may allocate more buffers than requested; the extra ones stay unused */
	for (unsigned int i = 0U; (i < req.count) && (i < VIDEO_NSI_V4L2_BUFFERS); i++) {
		struct v4l2_buffer buf = {
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.memory = V4L2_MEMORY_MMAP,
			.index = i,
		};
		void *mem;

		if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
			err = errno;
			goto fail;
		}

		mem = mmap(NULL, buf.length, PROT_READ, MAP_SHARED, fd, buf.m.offset);
		if (mem == MAP_FAILED) {
			err = errno;
			goto fail;
		}

		dev->mem[i] = mem;
		dev->len[i] = buf.length;
		dev->count = i + 1U;

		if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
			err = errno;
			goto fail;
		}
	}

	if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
		err = errno;
		goto fail;
	}

	return 0;

fail:
	nsi_print_warning("Could not start streaming from the V4L2 device (%s)\n", strerror(err));
	video_nsi_v4l2_stop(dev);
	return -nsi_errno_to_mid(err);
}

static uint8_t video_nsi_clamp8(int32_t v)
{
	return (v < 0) ? 0U : (v > 255) ? 255U : (uint8_t)v;
}

/* BT.601 limited range, two pixels per YUYV macropixel, RGB565 stored little-endian */
static void video_nsi_yuyv_to_rgb565(uint8_t *dst, const uint8_t *src, size_t len)
{
	for (size_t i = 0U; (i + 3U) < len; i += 4U) {
		int32_t d = (int32_t)src[i + 1U] - 128;
		int32_t e = (int32_t)src[i + 3U] - 128;

		for (size_t p = 0U; p < 2U; p++) {
			int32_t c = 298 * ((int32_t)src[i + 2U * p] - 16) + 128;
			uint8_t r = video_nsi_clamp8((c + 409 * e) / 256);
			uint8_t g = video_nsi_clamp8((c - 100 * d - 208 * e) / 256);
			uint8_t b = video_nsi_clamp8((c + 516 * d) / 256);
			uint16_t rgb =
				(uint16_t)(((r >> 3U) << 11U) | ((g >> 2U) << 5U) | (b >> 3U));

			dst[i + 2U * p] = (uint8_t)(rgb & 0xffU);
			dst[i + 2U * p + 1U] = (uint8_t)(rgb >> 8U);
		}
	}
}

static int video_nsi_v4l2_dqbuf(int fd, struct v4l2_buffer *vbuf)
{
	(void)memset(vbuf, 0, sizeof(*vbuf));
	vbuf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vbuf->memory = V4L2_MEMORY_MMAP;

	return ioctl(fd, VIDIOC_DQBUF, vbuf);
}

static int video_nsi_v4l2_capture(struct video_nsi_v4l2_dev *dev, uint8_t *buf, size_t size)
{
	struct v4l2_buffer vbuf;
	struct v4l2_buffer newer;
	bool valid;

	/* The device is non-blocking: EAGAIN when no frame is ready */
	if (video_nsi_v4l2_dqbuf(dev->fd, &vbuf) < 0) {
		return -nsi_errno_to_mid((errno == EINTR) ? EAGAIN : errno);
	}

	/* Frames queue up in the device while the application is slower: keep the newest */
	while (video_nsi_v4l2_dqbuf(dev->fd, &newer) == 0) {
		if (ioctl(dev->fd, VIDIOC_QBUF, &vbuf) < 0) {
			return -nsi_errno_to_mid(errno);
		}
		vbuf = newer;
	}

	/* A frame the device flags as corrupted or a short one is skipped, like no frame */
	valid = ((vbuf.flags & V4L2_BUF_FLAG_ERROR) == 0U) && (vbuf.bytesused >= size);
	if (valid && dev->yuyv_to_rgb565) {
		video_nsi_yuyv_to_rgb565(buf, dev->mem[vbuf.index], size);
	} else if (valid) {
		(void)memcpy(buf, dev->mem[vbuf.index], size);
	} else {
		/* Frame flagged as error or incomplete, skip without copying */
	}

	if (ioctl(dev->fd, VIDIOC_QBUF, &vbuf) < 0) {
		return -nsi_errno_to_mid(errno);
	}

	return valid ? 0 : -nsi_errno_to_mid(EAGAIN);
}

void video_nsi_fifo_close_bottom(int fd)
{
	struct video_nsi_v4l2_dev *dev = video_nsi_v4l2_find(fd);

	if (dev != NULL) {
		video_nsi_v4l2_stop(dev);
	}

	(void)close(fd);
}
