/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_XCLIB_INCLUDE_TIME_H_
#define ZEPHYR_LIB_LIBC_XCLIB_INCLUDE_TIME_H_

#include_next <time.h>

#include <zephyr/posix/posix_time.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

char *asctime_r(const struct tm *ZRESTRICT tp, char *ZRESTRICT buf);
struct tm *localtime_r(const time_t *ZRESTRICT timer, struct tm *ZRESTRICT result);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_XCLIB_INCLUDE_TIME_H_ */
