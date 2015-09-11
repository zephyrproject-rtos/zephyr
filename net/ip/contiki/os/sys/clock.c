/* clock.c - System specific clock routines */

/*
 * Copyright (c) 2015 Intel Corporation
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

#include "sys/clock.h"
#include <stdlib.h>

#define DEBUG 0
#include "contiki/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

static int64_t start_time;

void clock_init(void)
{
	sys_tick_delta(&start_time);
}

clock_time_t clock_time(void)
{
	return sys_tick_get_32();
}

unsigned long clock_seconds(void)
{
	return clock_time() / sys_clock_ticks_per_sec;
}

void clock_delay(unsigned int d)
{
	switch (sys_execution_context_type_get()) {
	case NANO_CTX_FIBER:
		fiber_sleep(d);
		break;
#ifdef CONFIG_MICROKERNEL
	case NANO_CTX_TASK:
		task_sleep(d);
		break;
#endif
	default:
		return;
	}
}

/* Note that this function busy waits until the delay has passed. */
void clock_delay_usec_busywait(uint32_t dt)
{
#define USEC_TO_CYCLES(usec) ((usec) * sys_clock_hw_cycles_per_sec / USEC_PER_SEC)

	uint32_t usec = USEC_TO_CYCLES(dt);
	uint32_t start = sys_cycle_get_32();

	while ((start + usec) > sys_cycle_get_32()) {
	}
}
