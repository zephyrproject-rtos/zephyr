/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HAL_CNTR_H_MOCK
#define HAL_CNTR_H_MOCK

#include <stdint.h>

/* Mock HAL counter interface for testing */

static inline void cntr_init(void) { }
static inline uint32_t cntr_start(void) { return 0; }
static inline uint32_t cntr_stop(void) { return 0; }

uint32_t cntr_cnt_get(void);
void cntr_cmp_set(uint8_t cmp, uint32_t value);

#endif /* HAL_CNTR_H_MOCK */
