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
#include <arch/cpu.h>
#include <irq.h>


void _arch_irq_enable(unsigned int irq)
{
	uint32_t ienable;
	int key;

	key = irq_lock();

	ienable = _nios2_creg_read(NIOS2_CR_IENABLE);
	ienable |= (1 << irq);
	_nios2_creg_write(NIOS2_CR_IENABLE, ienable);

	irq_unlock(key);
};


void _arch_irq_disable(unsigned int irq)
{
	uint32_t ienable;
	int key;

	key = irq_lock();

	ienable = _nios2_creg_read(NIOS2_CR_IENABLE);
	ienable &= ~(1 << irq);
	_nios2_creg_write(NIOS2_CR_IENABLE, ienable);

	irq_unlock(key);
};


int _arch_irq_connect_dynamic(unsigned int irq,
					 unsigned int priority,
					 void (*routine)(void *parameter),
					 void *parameter,
					 uint32_t flags)
{
	ARG_UNUSED(irq);
	ARG_UNUSED(priority);
	ARG_UNUSED(routine);
	ARG_UNUSED(parameter);
	ARG_UNUSED(flags);

	/* STUB. May never implement this, part of a deprecated API */
	return -1;
};

