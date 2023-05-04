/*
 * Copyright (c) 2023 Intel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_STDLIB_H_
#define ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_STDLIB_H_

#include <stddef.h>

void qsort_r(void *, size_t, size_t,
	     int (*)(const void *, const void *, void *), void *);

/* This should work on GCC and clang.
 *
 * If we need to support a toolchain without #include_next the CMake
 * infrastructure should be used to identify it and provide an
 * alternative solution.
 */
#include_next <stdlib.h>

#endif /* ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_STDLIB_H_ */
