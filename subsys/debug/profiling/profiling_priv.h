/*
 * Copyright (c) 2019 Kevin Townsend
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DBG_PROFILING_PRIV_H_
#define ZEPHYR_INCLUDE_DBG_PROFILING_PRIV_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Resets the CoreSight DWT cycle counter for the Cortex M3/M4/M7. */
#define DBG_PROF_RST_CYCCNT do { CoreDebug->DEMCR =			\
				 CoreDebug->DEMCR | 0x01000000;	        \
				 DWT->CYCCNT = 0;			\
				 DWT->CTRL = DWT->CTRL | 1; } while (0)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DBG_PROFILING_PRIV_H_ */
