/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Utilities supporting operation on time data structures.
 *
 * POSIX defines gmtime() to convert from time_t to struct tm, but all
 * inverse transformations are non-standard or require access to time
 * zone information.  timeutil_timegm() implements the functionality
 * of the GNU extension timegm() function, but changes the error value
 * as @c EOVERFLOW is not a standard C error identifier.
 *
 * timeutil_timegm64() is provided to support full precision
 * conversion on platforms where @c time_t is limited to 32 bits.
 */

#ifndef ZEPHYR_INCLUDE_SYS_TIMEUTIL_H_
#define ZEPHYR_INCLUDE_SYS_TIMEUTIL_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert broken-down time to a POSIX epoch offset in seconds.
 *
 * @param tm pointer to broken down time.
 *
 * @return the corresponding time in the POSIX epoch time scale.
 *
 * @see http://man7.org/linux/man-pages/man3/timegm.3.html
 */
s64_t timeutil_timegm64(const struct tm *tm);

/**
 * @brief Convert broken-down time to a POSIX epoch offset in seconds.
 *
 * @param tm pointer to broken down time.
 *
 * @return the corresponding time in the POSIX epoch time scale.  If
 * the time cannot be represented then @c (time_t)-1 is returned and
 * @c errno is set to @c ERANGE`.
 *
 * @see http://man7.org/linux/man-pages/man3/timegm.3.html
 */
time_t timeutil_timegm(const struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_TIMEUTIL_H_ */
