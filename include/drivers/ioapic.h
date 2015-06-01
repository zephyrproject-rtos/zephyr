/* ioapic.h - public IOAPIC APIs */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
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

#ifndef __INCioapich
#define __INCioapich

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_IOAPIC

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

#ifdef _ASMLANGUAGE
GTEXT(_ioapic_eoi)

.macro ioapic_mkstub device isr
GTEXT(_\()\device\()_\()\isr\()_stub)

SECTION_FUNC(TEXT, _\()\device\()_\()\isr\()_stub)
        call    _IntEnt         /* Inform kernel interrupt has begun */
        pushl   $0              /* Push dummy parameter */
        call    \isr            /* Call actual interrupt handler */
        call    _ioapic_eoi     /* Inform ioapic interrupt is done */
        addl    $4, %esp        /* Clean-up stack from push above */
        jmp     _IntExit        /* Inform kernel interrupt is done */
.endm
#else /* _ASMLANGUAGE */
void _ioapic_init(void);
void _ioapic_eoi(unsigned int irq);
void *_ioapic_eoi_get(unsigned int irq, char *argRequired, void **arg);
void _ioapic_irq_enable(unsigned int irq);
void _ioapic_irq_disable(unsigned int irq);
void _ioapic_int_vec_set(unsigned int irq, unsigned int vector);
void _ioapic_irq_set(unsigned int irq, unsigned int vector, uint32_t flags);
#endif /* _ASMLANGUAGE */

#endif /* CONFIG_IOAPIC */

#ifdef __cplusplus
}
#endif

#endif /* __INCioapich */
