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
 * @file Software interrupts utility code - ARM implementation
 */

#include <nanokernel.h>
#include <irq_offload.h>

static irq_offload_routine_t offload_routine;
static void *offload_param;

/* Called by __svc */
void _irq_do_offload(void)
{
	offload_routine(offload_param);
}

void irq_offload(irq_offload_routine_t routine, void *parameter)
{
	int key;

	key = irq_lock();
	offload_routine = routine;
	offload_param = parameter;

	__asm__ volatile ("svc #1");

	irq_unlock(key);
}
