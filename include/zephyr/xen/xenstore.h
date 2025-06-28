/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_XEN_XENSTORE_H_
#define ZEPHYR_XEN_XENSTORE_H_

#include <sys/types.h>

/**
 * @brief Read the value of a Xenstore path entry.
 *
 * Sends a read request to the Xenstore service and retrieves the
 * value at the specified path.
 *
 * @param path       Null-terminated Xenstore path string.
 * @param buf        Buffer to receive the returned value.
 * @param len        Maximum buffer length.
 * @return           Number of bytes read (excluding terminating null), or negative errno on error.
 */
ssize_t xs_read(const char *path, char *buf, size_t len);

/**
 * @brief List the entries in a Xenstore directory.
 *
 * Sends a directory request to the Xenstore service and populates
 * the buffer with a whitespace-separated list of entries.
 *
 * @param path       Null-terminated Xenstore directory path string.
 * @param buf        Buffer to receive the directory listing.
 * @param len        Maximum buffer length.
 * @return           Number of bytes written (excluding terminating null), or negative errno on
 * error.
 */
ssize_t xs_directory(const char *path, char *buf, size_t len);

#endif /* ZEPHYR_XEN_XENSTORE_H_ */
