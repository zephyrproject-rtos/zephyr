/* pic.h - public Intel 8259 PIC APIs */

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

#ifndef __INCpich
#define __INCpich

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_PIC) || defined(CONFIG_SHUTOFF_PIC)

/* programmable interrupt controller info (pair of cascaded 8259A devices) */

#define PIC_MASTER_BASE_ADRS 0x20
#define PIC_SLAVE_BASE_ADRS 0xa0
#define PIC_MASTER_STRAY_INT_LVL 0x07 /* master PIC stray IRQ */
#define PIC_SLAVE_STRAY_INT_LVL 0x0f  /* slave PIC stray IRQ */
#define PIC_MAX_INT_LVL 0x0f	  /* max interrupt level in PIC */
#define PIC_REG_ADDR_INTERVAL 1
#define INT_VEC_IRQ0 0x20 /* vector number for PIC IRQ0 */
#define N_PIC_IRQS 16     /* number of PIC IRQs */

#define I8259_EOI 0x20 /* EOI bit in OCW2 */

/* register definitions */
#define PIC_ADRS(baseAdrs, reg) (baseAdrs + (reg * PIC_REG_ADDR_INTERVAL))

#define PIC_PORT1(base) PIC_ADRS(base, 0x00) /* port 1 */
#define PIC_PORT2(base) PIC_ADRS(base, 0x01) /* port 2 */

#define PIC_IMASK(base) PIC_PORT2(base) /* Int Mask Reg */
#define PIC_IACK(base) PIC_PORT1(base)  /* Int Ack Reg */

#define PIC_ISR_MASK(base) PIC_PORT1(base) /* in-service register mask */
#define PIC_IRR_MASK(base) PIC_PORT1(base) /* interrupt request reg */

#ifdef _ASMLANGUAGE

GTEXT(_i8259_boi_master)
GTEXT(_i8259_boi_slave)
GTEXT(_i8259_eoi_master)
GTEXT(_i8259_eoi_slave)

.macro pic_master_mkstub device isr
GTEXT(_\()\device\()_\()\isr\()_stub)

SECTION_FUNC(TEXT, _\()\device\()_\()\isr\()_stub)
        call    _IntEnt           /* Inform kernel interrupt has begun */
        pushl   $0                /* Push dummy parameter */
        call    \isr              /* Call actual interrupt handler */
        call    _i8259_eoi_master /* Inform PIC interrupt is done */
        addl    $4, %esp          /* Clean-up stack from push above */
        jmp     _IntExit          /* Inform kernel interrupt is done */
.endm

.macro pic_slave_mkstub device isr
GTEXT(_\()\device\()_\()\isr\()_stub)

SECTION_FUNC(TEXT, _\()\device\()_\()\isr\()_stub)
        call    _IntEnt           /* Inform kernel interrupt has begun */
        pushl   $0                /* Push dummy parameter */
        call    \isr              /* Call actual interrupt handler */
        call    _i8259_eoi_slave  /* Inform PIC interrupt is done */
        addl    $4, %esp          /* Clean-up stack from push above */
        jmp     _IntExit          /* Inform kernel interrupt is done */
.endm
#else /* _ASMLANGUAGE */

extern void _i8259_init(void);
extern void _i8259_boi_master(void);
extern void _i8259_boi_slave(void);
extern void _i8259_eoi_master(unsigned int irq);
extern void _i8259_eoi_slave(unsigned int irq);
extern void _i8259_irq_enable(unsigned int irq);
extern void _i8259_irq_disable(unsigned int irq);

#endif /* _ASMLANGUAGE */

#endif /* CONFIG_PIC || CONFIG_SHUTOFF_PIC */

#ifdef __cplusplus
}
#endif

#endif /* __INCpich */
