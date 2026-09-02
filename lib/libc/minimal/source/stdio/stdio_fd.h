/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_SOURCE_STDIO_STDIO_FD_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_SOURCE_STDIO_STDIO_FD_H_

/*
 * Lowest fd that names a real ZVFS description. With reserved stdio slots,
 * stdin/stdout/stderr (0/1/2) are backed by the console description and honor
 * dup2() redirection (pipes, files); otherwise only fopen()'d fds (>= 3) are
 * real and the standard streams are handled specially (console / EOF).
 */
#ifdef CONFIG_ZVFS_RESERVE_STDIO_FDS
#define MINIMAL_STDIO_FD_MIN 0
#else
#define MINIMAL_STDIO_FD_MIN 3
#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_SOURCE_STDIO_STDIO_FD_H_ */
