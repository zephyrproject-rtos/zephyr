/* object_monitor.c - object monitor */

/*
 * Copyright (c) 2016 Intel Corporation.
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

#include <zephyr.h>
#include <tc_util.h>
#include <util_test_common.h>
#include <misc/debug/object_tracing.h>
#include "phil.h"

/**
 *
 * @brief Thread that traverses, counts and reports
 * the kernel objects in the philosophers application.
 *
 */
#ifdef CONFIG_NANOKERNEL
#define OBJ_LIST_NAME nano_sem
#define OBJ_LIST_TYPE struct nano_sem
#else  /*CONFIG_MICROKERNEL*/
#define OBJ_LIST_NAME micro_mutex
#define OBJ_LIST_TYPE struct _k_mutex_struct
#endif  /*CONFIG_NANOKERNEL*/

void object_monitor(void)
{
	int obj_counter;
	void *obj_list   = NULL;

	TC_START("OBJECT TRACING TEST");

	obj_counter = 0;
	obj_list   = SYS_TRACING_HEAD(OBJ_LIST_TYPE, OBJ_LIST_NAME);
	while (obj_list != NULL) {
		TC_PRINT("SEMAPHORE REF: %p\n", obj_list);
		obj_list = SYS_TRACING_NEXT(OBJ_LIST_TYPE, OBJ_LIST_NAME, obj_list);
		obj_counter++;
	}
	TC_PRINT("SEMAPHORE QUANTITY: %d\n", obj_counter);

	if (obj_counter == N_PHILOSOPHERS) {
		TC_END_RESULT(TC_PASS);
		TC_END_REPORT(TC_PASS);
	} else {
		TC_END_RESULT(TC_FAIL);
		TC_END_REPORT(TC_FAIL);
	}
}
