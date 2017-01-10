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

#include <toolchain.h>
#include <kernel_structs.h>
#include <misc/printk.h>

void _irq_spurious(void *unused)
{
	uint32_t mcause;

	ARG_UNUSED(unused);

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= SOC_MCAUSE_IRQ_MASK;

	printk("Spurious interrupt detected! IRQ: %d\n", (int)mcause);

	_NanoFatalErrorHandler(_NANO_ERR_SPURIOUS_INT, &_default_esf);
}
