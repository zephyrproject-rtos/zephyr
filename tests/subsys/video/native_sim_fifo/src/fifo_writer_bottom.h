/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_VIDEO_NATIVE_SIM_FIFO_FIFO_WRITER_BOTTOM_H_
#define ZEPHYR_TESTS_SUBSYS_VIDEO_NATIVE_SIM_FIFO_FIFO_WRITER_BOTTOM_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Attach a writer to the host FIFO the driver reads from.
 *
 * The FIFO must already exist, which the driver takes care of when the stream
 * is started. The writer is non-blocking so that it can never stall the
 * simulated system.
 *
 * @param path Path of the FIFO on the host filesystem.
 *
 * @return A host file descriptor on success, -1 on error.
 */
int video_fifo_test_open_writer(const char *path);

/**
 * @brief Write raw frame bytes to a writer descriptor without ever blocking.
 *
 * @param fd  Descriptor returned by video_fifo_test_open_writer().
 * @param buf Bytes to write.
 * @param len Number of bytes to write.
 *
 * @return The number of bytes actually written, which is less than @p len when
 *         the host pipe is full or an error occurred.
 */
int video_fifo_test_write(int fd, const uint8_t *buf, size_t len);

/**
 * @brief Detach a writer, which the driver sees as an end of file.
 *
 * @param fd Descriptor returned by video_fifo_test_open_writer().
 *
 * @return 0 on success, -1 on error.
 */
int video_fifo_test_close(int fd);

/**
 * @brief Remove the FIFO from the host filesystem.
 *
 * Removing a path that does not exist is not an error.
 *
 * @param path Path of the FIFO on the host filesystem.
 *
 * @return 0 on success, -1 on error.
 */
int video_fifo_test_unlink(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_SUBSYS_VIDEO_NATIVE_SIM_FIFO_FIFO_WRITER_BOTTOM_H_ */
