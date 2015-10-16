/*
 * Copyright (c) 2012-2015, Wind River Systems, Inc.
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

#define _ASMLANGUAGE

#include <arch/x86/asm.h>
#include <drivers/ioapic.h>


#if CONFIG_SHARED_IRQ_0
	ioapic_mkstub shared_irq_0 shared_irq_isr_0 0
#endif /* CONFIG_SHARED_IRQ_0 */

#if CONFIG_SHARED_IRQ_1
	ioapic_mkstub shared_irq_1 shared_irq_isr_1 0
#endif /* CONFIG_SHARED_IRQ_1 */
