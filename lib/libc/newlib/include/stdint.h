/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_STDINT_H_
#define ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_STDINT_H_

/* Work around -ffreestanding absence of defines required to support
 * PRI.64 macros in <inttypes.h> by including the newlib header that
 * provides the flag macros.
 */

#include <newlib.h>

#ifdef __NEWLIB__
/* Has this header.  Older versions do it in <stdint.h>. */
#include <sys/_stdint.h>
#endif /* __NEWLIB__ */

/* This should work on GCC and clang.
 *
 * If we need to support a toolchain without #include_next the CMake
 * infrastructure should be used to identify it and provide an
 * alternative solution.
 */
#include_next <stdint.h>

#endif /* ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_STDINT_H_ */
