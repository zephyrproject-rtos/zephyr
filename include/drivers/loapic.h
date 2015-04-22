/* loapic.h - public LOAPIC APIs */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __INCloapich
#define __INCloapich

#ifdef __cplusplus
extern "C" {
#endif

/* Local APIC Register Offset */

#define LOAPIC_ID 0x020		  /* Local APIC ID Reg */
#define LOAPIC_VER 0x030	  /* Local APIC Version Reg */
#define LOAPIC_TPR 0x080	  /* Task Priority Reg */
#define LOAPIC_APR 0x090	  /* Arbitration Priority Reg */
#define LOAPIC_PPR 0x0a0	  /* Processor Priority Reg */
#define LOAPIC_EOI 0x0b0	  /* EOI Reg */
#define LOAPIC_LDR 0x0d0	  /* Logical Destination Reg */
#define LOAPIC_DFR 0x0e0	  /* Destination Format Reg */
#define LOAPIC_SVR 0x0f0	  /* Spurious Interrupt Reg */
#define LOAPIC_ISR 0x100	  /* In-service Reg */
#define LOAPIC_TMR 0x180	  /* Trigger Mode Reg */
#define LOAPIC_IRR 0x200	  /* Interrupt Request Reg */
#define LOAPIC_ESR 0x280	  /* Error Status Reg */
#define LOAPIC_ICRLO 0x300	/* Interrupt Command Reg */
#define LOAPIC_ICRHI 0x310	/* Interrupt Command Reg */
#define LOAPIC_TIMER 0x320	/* LVT (Timer) */
#define LOAPIC_THERMAL 0x330      /* LVT (Thermal) */
#define LOAPIC_PMC 0x340	  /* LVT (PMC) */
#define LOAPIC_LINT0 0x350	/* LVT (LINT0) */
#define LOAPIC_LINT1 0x360	/* LVT (LINT1) */
#define LOAPIC_ERROR 0x370	/* LVT (ERROR) */
#define LOAPIC_TIMER 0x320	/* LVT (Timer) */
#define LOAPIC_TIMER_ICR 0x380    /* Timer Initial Count Reg */
#define LOAPIC_TIMER_CCR 0x390    /* Timer Current Count Reg */
#define LOAPIC_TIMER_CONFIG 0x3e0 /* Timer Divide Config Reg */

/* Local APIC Vector Table Bits */
#define LOAPIC_LVT_MASKED 0x00010000   /* mask */

#ifdef _ASMLANGUAGE
GTEXT(_loapic_eoi)
#else /* _ASMLANGUAGE */
extern void _loapic_init(void);
extern void _loapic_eoi(unsigned int irq);
extern void _loapic_int_vec_set(unsigned int irq, unsigned int vector);
extern void _loapic_irq_enable(unsigned int irq);
extern void _loapic_irq_disable(unsigned int irq);
extern void _loapic_enable(void);
extern void _loapic_disable(void);
#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCloapich */
