/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_DRIVERS_UART_UART_NATIVE_TTY_SRC_PTY_BOTTOM_H_
#define ZEPHYR_TESTS_DRIVERS_UART_UART_NATIVE_TTY_SRC_PTY_BOTTOM_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Open a host pseudo terminal pair.
 *
 * @param path	Receives the path of the terminal side.
 * @param size	Size of @p path.
 *
 * @return The controlling side's file descriptor, or -1 on error.
 */
int native_tty_test_open_pty(char *path, long size);

/**
 * @brief Write @p size bytes of @p data to a host file descriptor.
 *
 * @return 0 if all bytes were written, -1 otherwise.
 */
int native_tty_test_write(int fd, const char *data, long size);

/**
 * @brief Wait until a host file descriptor is readable.
 *
 * The kernel hands written data to the other side of a pseudo terminal
 * asynchronously. There is no timeout.
 */
void native_tty_test_wait_readable(int fd);

/** @brief Close a host file descriptor. */
void native_tty_test_close(int fd);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_DRIVERS_UART_UART_NATIVE_TTY_SRC_PTY_BOTTOM_H_ */
