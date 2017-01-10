/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
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

#include <irq.h>
#include <irq_offload.h>
#include <misc/printk.h>

volatile irq_offload_routine_t _offload_routine;
static volatile void *offload_param;

/*
 * Called by _enter_irq
 *
 * Just in case the offload routine itself generates an unhandled
 * exception, clear the offload_routine global before executing.
 */
void _irq_do_offload(void)
{
	irq_offload_routine_t tmp;

	if (!_offload_routine)
		return;

	tmp = _offload_routine;
	_offload_routine = NULL;

	tmp((void *)offload_param);
}

void irq_offload(irq_offload_routine_t routine, void *parameter)
{
	int key;

	key = irq_lock();
	_offload_routine = routine;
	offload_param = parameter;

	__asm__ volatile ("ecall");

	irq_unlock(key);
}
