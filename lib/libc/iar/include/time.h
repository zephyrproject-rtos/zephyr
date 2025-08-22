/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_IAR_INCLUDE_TIME_H_
#define ZEPHYR_LIB_LIBC_IAR_INCLUDE_TIME_H_

#include_next <time.h>

#include <zephyr/toolchain.h>
#include <zephyr/posix/posix_time.h>

#if __STDC_VERSION__ >= 202311L
struct tm *gmtime_r(const time_t *ZRESTRICT timep, struct tm *ZRESTRICT result);
#endif
#if __STDC_VERSION__ >= 202311L
struct tm *localtime_r(const time_t *ZRESTRICT timer, struct tm *ZRESTRICT result);
#endif

#endif /* ZEPHYR_LIB_LIBC_IAR_INCLUDE_TIME_H_ */
