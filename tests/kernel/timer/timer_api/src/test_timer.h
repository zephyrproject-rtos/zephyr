/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
