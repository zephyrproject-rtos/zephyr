/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
 * @brief ARM Cortex-M interrupt initialization
 *
 * The ARM Cortex-M architecture provides its own fiber_abort() to deal with
 * different CPU modes (handler vs thread) when a fiber aborts. When its entry
 * point returns or when it aborts itself, the CPU is in thread mode and must
 * call _Swap() (which triggers a service call), but when in handler mode, the
 * CPU must exit handler mode to cause the context switch, and thus must queue
 * the PendSV exception.
 */

#include <toolchain.h>
#include <sections.h>
#include <nanokernel.h>
#include <arch/cpu.h>

/**
 *
 * @brief Initialize interrupts
 *
 * Ensures all interrupts have their priority set to _EXC_IRQ_DEFAULT_PRIO and
 * not 0, which they have it set to when coming out of reset. This ensures that
 * interrupt locking via BASEPRI works as expected.
 *
 * @return N/A
 */

void _IntLibInit(void)
{
	int irq = 0;

	for (; irq < CONFIG_NUM_IRQS; irq++) {
		_NvicIrqPrioSet(irq, _EXC_IRQ_DEFAULT_PRIO);
	}
}
