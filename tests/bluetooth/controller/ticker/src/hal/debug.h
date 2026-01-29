/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HAL_DEBUG_H_MOCK
#define HAL_DEBUG_H_MOCK

/* Mock HAL debug interface for testing */

/* Empty debug macros for testing */
#define DEBUG_INIT()
#define DEBUG_CPU_SLEEP(x)
#define DEBUG_TICKER_ISR(x)
#define DEBUG_TICKER_TASK(x)
#define DEBUG_TICKER_JOB(x)

#endif /* HAL_DEBUG_H_MOCK */
