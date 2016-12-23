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

#ifndef __TEST_SCHED_H__
#define __TEST_SCHED_H__

#include <zephyr.h>
#include <ztest.h>

#define STACK_SIZE 256

struct thread_data {
	k_tid_t tid;
	int priority;
	int executed;
};

void test_priority_cooperative(void);
void test_priority_preemptible(void);
void test_yield_cooperative(void);
void test_sleep_cooperative(void);
void test_sleep_wakeup_preemptible(void);
void test_time_slicing_preemptible(void);
void test_time_slicing_disable_preemptible(void);
void test_lock_preemptible(void);
void test_unlock_preemptible(void);

#endif /* __TEST_SCHED_H__ */
