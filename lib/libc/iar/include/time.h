/*
 * Copyright (c) 2025 IAR Systems AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Declares additional time related functions based on POSIX
 */

#ifndef ZEPHYR_LIB_LIBC_IAR_INCLUDE_TIME_H_
#define ZEPHYR_LIB_LIBC_IAR_INCLUDE_TIME_H_

#include <zephyr/toolchain.h>
#include_next <time.h>

#ifdef __cplusplus
extern "C" {
#endif

char      *asctime_r(const struct tm *ZRESTRICT tp, char *ZRESTRICT buf);
char      *ctime_r(const time_t *clock, char *buf);
struct tm *gmtime_r(const time_t *ZRESTRICT timep, struct tm *ZRESTRICT result);
struct tm *localtime_r(const time_t *ZRESTRICT timer, struct tm *ZRESTRICT result);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_IAR_INCLUDE_TIME_H_ */
