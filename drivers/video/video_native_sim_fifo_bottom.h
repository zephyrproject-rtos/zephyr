/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_VIDEO_NATIVE_SIM_FIFO_BOTTOM_H_
#define ZEPHYR_DRIVERS_VIDEO_VIDEO_NATIVE_SIM_FIFO_BOTTOM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build the default FIFO path, unique to this process.
 *
 * @param name Device name.
 * @param buf  Buffer receiving "/tmp/zephyr-<name>-<pid>.fifo".
 * @param size Size of @p buf in bytes.
 *
 * @return 0 on success, -1 if the path does not fit in @p buf.
 */
int video_nsi_fifo_default_path_bottom(const char *name, char *buf, size_t size);

/**
 * @brief Remove a FIFO from the host filesystem.
 *
 * @param path Path of the FIFO on the host filesystem.
 */
void video_nsi_fifo_unlink_bottom(const char *path);

/**
 * @brief Open the host video source: a FIFO, created if needed, or a V4L2 device.
 *
 * The path is opened non-blocking, so that a FIFO can be opened before any
 * writer is attached and so that no later access can stall the simulated
 * system. A FIFO is opened read-only, a V4L2 device read-write, as its buffers
 * cannot be mapped otherwise.
 *
 * @param path    Path of the FIFO or V4L2 capture device on the host filesystem.
 * @param created Set to true if this call created the FIFO, left untouched otherwise.
 * @param v4l2    Set to true if the path is a character device, false if it is a FIFO.
 *
 * @return A host file descriptor on success.
 * @return A negative intermediate errno value (see nsi_errno.h) on error.
 */
int video_nsi_fifo_open_bottom(const char *path, bool *created, bool *v4l2);

/**
 * @brief Set the format of a V4L2 capture device and start streaming from it.
 *
 * RGB565 is converted on the host from YUYV when the device does not provide it.
 * When frames queue up in the device, the newest one is delivered.
 *
 * @param fd          File descriptor of a V4L2 device returned by video_nsi_fifo_open_bottom().
 * @param pixelformat Four character code of the pixel format, shared with V4L2.
 * @param width       Frame width in pixels.
 * @param height      Frame height in pixels.
 * @param pitch       Line length in bytes.
 *
 * @return 0 on success.
 * @return A negative intermediate errno value (see nsi_errno.h) on error:
 *         ENOTSUP when the device does not provide the requested format.
 */
int video_nsi_fifo_v4l2_start_bottom(int fd, unsigned int pixelformat, unsigned int width,
				     unsigned int height, unsigned int pitch);

/**
 * @brief Close the host video source, stopping a V4L2 stream first.
 *
 * @param fd File descriptor returned by video_nsi_fifo_open_bottom().
 */
void video_nsi_fifo_close_bottom(int fd);

/**
 * @brief Enlarge the host pipe.
 *
 * The pipe is never shrunk. This function tries to resize the pipe to @p size
 * bytes and, if that fails, retries with each smaller power of two until it
 * succeeds or reaches the current size. Note that unprivileged processes are
 * limited by /proc/sys/fs/pipe-max-size for each pipe, and by
 * /proc/sys/fs/pipe-user-pages-soft for all their pipes together.
 *
 * @param fd   File descriptor returned by video_nsi_fifo_open_bottom().
 * @param size Requested capacity in bytes.
 *
 * @return The capacity of the pipe in bytes, which may be less than @p size.
 * @return A negative intermediate errno value (see nsi_errno.h) on error.
 */
int video_nsi_fifo_grow_pipe_bottom(int fd, size_t size);

/**
 * @brief Accumulate the bytes available on the FIFO into a buffer.
 *
 * Read whatever the host made available without ever blocking, until either a
 * complete frame of @p frame_size bytes has been read, no more data is
 * available, or the last writer closed the FIFO. A V4L2 device provides whole
 * frames: one is copied when ready, @p offset is not used.
 *
 * @param fd         File descriptor returned by video_nsi_fifo_open_bottom().
 * @param buf        Buffer of at least @p frame_size bytes.
 * @param frame_size Size of a complete frame in bytes.
 * @param offset     In/out number of bytes of the frame read so far.
 *
 * @return 0 when the frame is complete, with @p offset equal to @p frame_size.
 * @return A negative intermediate errno value (see nsi_errno.h) otherwise:
 *         EAGAIN when more data is needed, EPIPE when the last writer closed
 *         the FIFO.
 */
int video_nsi_fifo_read_bottom(int fd, uint8_t *buf, size_t frame_size, size_t *offset);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_VIDEO_VIDEO_NATIVE_SIM_FIFO_BOTTOM_H_ */
