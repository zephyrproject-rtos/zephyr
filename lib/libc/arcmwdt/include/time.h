/*
 * Copyright (c) 2024 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_TIME_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_TIME_H_

#include_next <time.h>

#include <zephyr/posix/posix_time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char *asctime_r(const struct tm *tp, char *buf);
extern char *ctime_r(const time_t *clock, char *buf);
extern struct tm *gmtime_r(const time_t *timep, struct tm *result);
extern struct tm *localtime_r(const time_t *timer, struct tm *result);

#ifdef __cplusplus
}
#endif

#endif /* LIB_LIBC_ARCMWDT_INCLUDE_TIME_H_ */
