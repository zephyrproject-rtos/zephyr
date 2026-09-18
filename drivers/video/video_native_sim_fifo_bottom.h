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
 * @brief Open the host FIFO used as video source, creating it if needed.
 *
 * The FIFO is opened read-only and non-blocking, so that it can be opened
 * before any writer is attached and so that no later read can stall the
 * simulated system.
 *
 * @param path    Path of the FIFO on the host filesystem.
 * @param created Set to true if this call created the FIFO, left untouched otherwise.
 *
 * @return A host file descriptor on success.
 * @return A negative intermediate errno value (see nsi_errno.h) on error.
 */
int video_nsi_fifo_open_bottom(const char *path, bool *created);

/**
 * @brief Accumulate the bytes available on the FIFO into a buffer.
 *
 * Read whatever the host made available without ever blocking, until either a
 * complete frame of @p frame_size bytes has been read, no more data is
 * available, or the last writer closed the FIFO.
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
