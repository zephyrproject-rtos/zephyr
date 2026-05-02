/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_TIME_H_
#define ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_TIME_H_

#include_next <time.h>
#include <sys/timespec.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tm *gmtime_r(const time_t *timer, struct tm *result);
struct tm *localtime_r(const time_t *timer, struct tm *result);
char *asctime_r(const struct tm *timeptr, char *buf);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_TIME_H_ */
