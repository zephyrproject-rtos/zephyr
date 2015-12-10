/* ioapic_priv.h - private IOAPIC APIs */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
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

#ifndef __INCioapic_privh
#define __INCioapic_privh

/* IO APIC direct register offsets */

#define IOAPIC_IND 0x00   /* Index Register */
#define IOAPIC_DATA 0x10  /* IO window (data) - pc.h */
#define IOAPIC_IRQPA 0x20 /* IRQ Pin Assertion Register */
#define IOAPIC_EOI 0x40   /* EOI Register */

/* IO APIC indirect register offset */

#define IOAPIC_ID 0x00     /* IOAPIC ID */
#define IOAPIC_VERS 0x01   /* IOAPIC Version */
#define IOAPIC_ARB 0x02    /* IOAPIC Arbitration ID */
#define IOAPIC_BOOT 0x03   /* IOAPIC Boot Configuration */
#define IOAPIC_REDTBL 0x10 /* Redirection Table (24 * 64bit) */

/* Interrupt delivery type */

#define IOAPIC_DT_APIC 0x0 /* APIC serial bus */
#define IOAPIC_DT_FS 0x1   /* Front side bus message*/

/* Version register bits */

#define IOAPIC_MRE_MASK 0x00ff0000 /* Max Red. entry mask */
#define IOAPIC_PRQ 0x00008000      /* this has IRQ reg */
#define IOAPIC_VERSION 0x000000ff  /* version number */

/* Redirection table entry number */

#define MAX_REDTABLE_ENTRIES 24

/* Redirection table entry bits: upper 32 bit */

#define IOAPIC_DESTINATION 0xff000000

/* Redirection table entry bits: lower 32 bit */

#define IOAPIC_VEC_MASK 0x000000ff

#endif
