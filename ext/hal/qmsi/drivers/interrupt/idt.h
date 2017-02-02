/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __IDT_H__
#define __IDT_H__

#include <stdint.h>
#include <string.h>
#include "qm_common.h"
#include "qm_soc_regs.h"

#if (QUARK_SE)
#define IDT_NUM_GATES (68)
#elif(QUARK_D2000)
#define IDT_NUM_GATES (52)
#endif

#define IDT_SIZE (sizeof(intr_gate_desc_t) * IDT_NUM_GATES)

typedef struct idtr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idtr_t;

typedef struct intr_gate_desc {
	uint16_t isr_low;
	uint16_t selector; /* Segment selector */

	/* The format of conf is the following:

	    15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
	   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	   |p |dpl  |ss|d |type |         unused           |

	   type: Gate type
	   d: size of Gate
	   ss: Storage Segment
	   dpl: Descriptor Privilege level
	   p: Segment present level
	*/
	uint16_t conf;
	uint16_t isr_high;
} __attribute__((packed)) intr_gate_desc_t;

extern intr_gate_desc_t __idt_start[];

/*
 * Setup IDT gate as an interrupt descriptor and assing the ISR entry point
 */
static __inline__ void idt_set_intr_gate_desc(uint32_t vector, uint32_t isr)
{
	intr_gate_desc_t *desc;
	idtr_t idtr;

	desc = __idt_start + vector;

	desc->isr_low = isr & 0xFFFF;
	desc->selector = 0x08; /* Code segment offset in GDT */

	desc->conf = 0x8E00; /* type: 0b11 (Interrupt)
				d: 1 (32 bits)
				ss: 0
				dpl: 0
				p: 1
			     */
	desc->isr_high = (isr >> 16) & 0xFFFF;

	/* The following reloads the IDTR register. If a lookaside buffer is
	 * being used this will invalidate it. This is required as it's possible
	 * for an application to change the registered ISR. */
	idtr.limit = IDT_SIZE - 1;
	idtr.base = (uint32_t)__idt_start;
	__asm__ __volatile__("lidt %0\n\t" ::"m"(idtr));
}

/*
 * Initialize Interrupt Descriptor Table.
 * The IDT is initialized with null descriptors: any interrupt at this stage
 * will cause a triple fault.
 */
static __inline__ void idt_init(void)
{
	idtr_t idtr;

	memset(__idt_start, 0x00, IDT_SIZE);

	/* Initialize idtr structure */
	idtr.limit = IDT_SIZE - 1;
	idtr.base = (uint32_t)__idt_start;

	/* Load IDTR register */
	__asm__ __volatile__("lidt %0\n\t" ::"m"(idtr));
}
#endif /* __IDT_H__ */
