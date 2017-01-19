/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_TIMER_H__
#define __TEST_TIMER_H__
#include <stdint.h>

struct timer_data {
	int expire_cnt;
	int stop_cnt;
	int64_t timestamp;
};

void test_timer_duration_period(void);
void test_timer_period_0(void);
void test_timer_expirefn_null(void);
void test_timer_status_get(void);
void test_timer_status_get_anytime(void);
void test_timer_status_sync(void);
void test_timer_k_define(void);
void test_timer_user_data(void);

#endif /* __TEST_TIMER_H__ */
