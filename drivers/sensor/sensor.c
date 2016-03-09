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

#include <nanokernel.h>
#include <sensor.h>
#include <init.h>

static char __stack sensor_fiber_stack[CONFIG_SENSOR_DELAYED_WORK_STACK_SIZE];
static struct nano_fifo sensor_fifo;

struct nano_fifo *sensor_get_work_fifo(void)
{
	return &sensor_fifo;
}

static void sensor_fiber_main(int arg1, int arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	while (1) {
		struct sensor_work *work;

		work = nano_fiber_fifo_get(&sensor_fifo, TICKS_UNLIMITED);

		work->handler(work->arg);
	}
}

static int sensor_init(struct device *dev)
{
	ARG_UNUSED(dev);

	nano_fifo_init(&sensor_fifo);

	fiber_fiber_start(sensor_fiber_stack,
			  CONFIG_SENSOR_DELAYED_WORK_STACK_SIZE,
			  sensor_fiber_main, 0, 0,
			  CONFIG_SENSOR_DELAYED_WORK_PRIORITY, 0);

	return 0;
}

SYS_INIT(sensor_init, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
