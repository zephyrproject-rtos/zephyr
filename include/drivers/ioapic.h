/* ioapic.h - public IOAPIC APIs */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
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

#ifndef __INCioapich
#define __INCioapich

#include <drivers/loapic.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Redirection table entry bits: lower 32 bit
 * Used as flags argument in ioapic_irq_set
 */

#define IOAPIC_INT_MASK 0x00010000
#define IOAPIC_TRIGGER_MASK 0x00008000
#define IOAPIC_LEVEL 0x00008000
#define IOAPIC_EDGE 0x00000000
#define IOAPIC_REMOTE 0x00004000
#define IOAPIC_LOW 0x00002000
#define IOAPIC_HIGH 0x00000000
#define IOAPIC_LOGICAL 0x00000800
#define IOAPIC_PHYSICAL 0x00000000
#define IOAPIC_FIXED 0x00000000
#define IOAPIC_LOWEST 0x00000100
#define IOAPIC_SMI 0x00000200
#define IOAPIC_NMI 0x00000400
#define IOAPIC_INIT 0x00000500
#define IOAPIC_EXTINT 0x00000700

#ifndef _ASMLANGUAGE
#include <device.h>
void _ioapic_irq_enable(unsigned int irq);
void _ioapic_irq_disable(unsigned int irq);
void _ioapic_int_vec_set(unsigned int irq, unsigned int vector);
void _ioapic_irq_set(unsigned int irq, unsigned int vector, uint32_t flags);
#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCioapich */
