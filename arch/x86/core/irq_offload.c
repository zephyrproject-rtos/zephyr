/*
 * Copyright (c) 2015 Intel corporation
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
 * @file IRQ offload - x86 implementation
 */

#include <nanokernel.h>
#include <irq_offload.h>

extern void (*_irq_sw_handler)(void);
NANO_CPU_INT_REGISTER(_irq_sw_handler, NANO_SOFT_IRQ,
		      CONFIG_IRQ_OFFLOAD_VECTOR / 16,
		      CONFIG_IRQ_OFFLOAD_VECTOR, 0);

static irq_offload_routine_t offload_routine;
static void *offload_param;

/* Called by asm stub */
void _irq_do_offload(void)
{
	offload_routine(offload_param);
}

void irq_offload(irq_offload_routine_t routine, void *parameter)
{
	int key;

	/*
	 * Lock interrupts here to prevent any concurrency issues with
	 * the two globals
	 */
	key = irq_lock();
	offload_routine = routine;
	offload_param = parameter;

	__asm__ volatile("int %[vector]" : :
			 [vector] "i" (CONFIG_IRQ_OFFLOAD_VECTOR));

	irq_unlock(key);
}
