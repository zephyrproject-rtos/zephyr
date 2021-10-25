/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_GDBSTUB_H_
#define ZEPHYR_INCLUDE_DEBUG_GDBSTUB_H_

/* Map from CPU excpetions to GDB  */
#define GDB_EXCEPTION_INVALID_INSTRUCTION   4UL
#define GDB_EXCEPTION_BREAKPOINT            5UL
#define GDB_EXCEPTION_MEMORY_FAULT          7UL
#define GDB_EXCEPTION_DIVIDE_ERROR          8UL
#define GDB_EXCEPTION_INVALID_MEMORY        11UL
#define GDB_EXCEPTION_OVERFLOW              16UL

/**
 * @brief Convert a binary array into string representation.
 *
 * Note that this is similar to bin2hex() but does not force
 * a null character at the end of the hexadeciaml output buffer.
 *
 * @param buf     The binary array to convert
 * @param buflen  The length of the binary array to convert
 * @param hex     Address of where to store the string representation.
 * @param hexlen  Size of the storage area for string representation.
 *
 * @return The length of the converted string, or 0 if an error occurred.
 */
size_t gdb_bin2hex(const uint8_t *buf, size_t buflen,
		   char *hex, size_t hexlen);

#endif
