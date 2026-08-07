/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_TIMES_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_TIMES_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_POSIX_MULTI_PROCESS) || defined(__DOXYGEN__)

#if !defined(_TMS_DECLARED) && !defined(__tms_defined)
struct tms {
	clock_t tms_utime;
	clock_t tms_stime;
	clock_t tms_cutime;
	clock_t tms_cstime;
};
#define _TMS_DECLARED
#define __tms_defined
#endif

clock_t times(struct tms *buf);

#endif /* _POSIX_MULTI_PROCESS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_TIMES_H_ */
