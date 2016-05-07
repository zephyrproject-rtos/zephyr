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

#ifndef __INC_SYS_APIC_H
#define __INC_SYS_APIC_H

#if !defined(LOAPIC_IRQ_BASE) && !defined(LOAPIC_IRQ_COUNT)

/* Default IA32 system APIC definitions with local APIC IRQs after IO APIC. */

#define LOAPIC_IRQ_BASE  CONFIG_IOAPIC_NUM_RTES
#define LOAPIC_IRQ_COUNT 6  /* Default to LOAPIC_TIMER to LOAPIC_ERROR */

#define IS_IOAPIC_IRQ(irq)  (irq < LOAPIC_IRQ_BASE)

#define HARDWARE_IRQ_LIMIT ((LOAPIC_IRQ_BASE + LOAPIC_IRQ_COUNT) - 1)

#else

 /* Assumption for boards that define LOAPIC_IRQ_BASE & LOAPIC_IRQ_COUNT that
  * local APIC IRQs are within IOAPIC RTE range.
  */

#define IS_IOAPIC_IRQ(irq)  ((irq < LOAPIC_IRQ_BASE) || \
	(irq >= (LOAPIC_IRQ_BASE + LOAPIC_IRQ_COUNT)))

#define HARDWARE_IRQ_LIMIT (CONFIG_IOAPIC_NUM_RTES - 1)

#endif

#endif /* __INC_SYS_APIC_H */
