/*
 * Copyright (c) 2016 Wind River Systems, Inc.
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

/**
 * @file
 *
 * System workqueue.
 */

#include <kernel.h>
#include <init.h>

static char __stack sys_work_q_stack[CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE];

struct k_work_q k_sys_work_q;

static int k_sys_work_q_init(struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_q_start(&k_sys_work_q,
		       sys_work_q_stack,
		       sizeof(sys_work_q_stack),
		       CONFIG_SYSTEM_WORKQUEUE_PRIORITY);

	return 0;
}

SYS_INIT(k_sys_work_q_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
