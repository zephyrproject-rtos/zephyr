/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_VIDEO_NATIVE_SIM_FIFO_BOTTOM_H_
#define ZEPHYR_DRIVERS_VIDEO_VIDEO_NATIVE_SIM_FIFO_BOTTOM_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Status returned by video_nsi_fifo_read_bottom() */
enum video_nsi_fifo_status {
	/** No complete frame is available yet, more data is needed */
	VIDEO_NSI_FIFO_NO_FRAME = 0,
	/** A complete frame is available in the staging buffer */
	VIDEO_NSI_FIFO_FRAME = 1,
	/** All writers closed the FIFO, no partial frame was pending */
	VIDEO_NSI_FIFO_WRITER_GONE = 2,
	/** All writers closed the FIFO, an incomplete frame has been discarded */
	VIDEO_NSI_FIFO_WRITER_GONE_PARTIAL = 3,
};

/**
 * @brief Open the host FIFO used as video source, creating it if needed.
 *
 * The FIFO is opened read-only and non-blocking, so that it can be opened
 * before any writer is attached and so that no later read can stall the
 * simulated system.
 *
 * @param path Path of the FIFO on the host filesystem.
 *
 * @return A host file descriptor on success.
 * @return A negative intermediate errno value (see nsi_errno.h) on error.
 */
int video_nsi_fifo_open_bottom(const char *path);

/**
 * @brief Accumulate the bytes available on the FIFO into a staging buffer.
 *
 * Read whatever the host made available without ever blocking, until either a
 * complete frame of @p frame_size bytes has been staged, no more data is
 * available, or the last writer closed the FIFO.
 *
 * @param fd         File descriptor returned by video_nsi_fifo_open_bottom().
 * @param buf        Staging buffer of at least @p frame_size bytes.
 * @param frame_size Size of a complete frame in bytes.
 * @param offset     In/out number of bytes already staged. Reset to 0 when a
 *                   frame is completed or when a partial frame is discarded.
 *
 * @return A @ref video_nsi_fifo_status value on success.
 * @return A negative intermediate errno value (see nsi_errno.h) on error.
 */
int video_nsi_fifo_read_bottom(int fd, uint8_t *buf, size_t frame_size, size_t *offset);

/**
 * @brief Close a file descriptor returned by video_nsi_fifo_open_bottom().
 *
 * @param fd File descriptor to close.
 *
 * @return 0 on success.
 * @return A negative intermediate errno value (see nsi_errno.h) on error.
 */
int video_nsi_fifo_close_bottom(int fd);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_VIDEO_VIDEO_NATIVE_SIM_FIFO_BOTTOM_H_ */
