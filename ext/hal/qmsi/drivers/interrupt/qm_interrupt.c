/*
 * Copyright (c) 2017, Intel Corporation
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

#include "qm_common.h"
#include "qm_interrupt.h"
#include "idt.h"

#if (HAS_APIC)
#include "apic.h"

#elif(HAS_MVIC)
#include "mvic.h"

#elif(QM_SENSOR)
#include "qm_ss_interrupt.h"
#include "qm_sensor_regs.h"
extern qm_ss_isr_t __ivt_vect_table[];

static void ss_register_irq(unsigned int vector);

#else
#error "Unsupported / unspecified processor detected."
#endif

/* x86 CPU FLAGS.IF register field (Interrupt enable Flag, bit 9), indicating
 * whether or not CPU interrupts are enabled.
 */
#define X86_FLAGS_IF BIT(9)

void qm_irq_disable(void)
{
#if (QM_SENSOR)
	qm_ss_irq_disable();
#else
	__asm__ __volatile__("cli");
#endif
}

void qm_irq_enable(void)
{
#if (QM_SENSOR)
	qm_ss_irq_enable();
#else
	__asm__ __volatile__("sti");
#endif
}

#if (QM_SENSOR)
unsigned int qm_irq_lock(void)
{
	unsigned int key = 0;

	/*
	 * Store the ARC STATUS32 register fields relating to interrupts into
	 * the variable `key' and disable interrupt delivery to the core.
	 */
	__asm__ __volatile__("clri %0" : "=r"(key));

	return key;
}

void qm_irq_unlock(unsigned int key)
{
	/*
	 * Restore the ARC STATUS32 register fields relating to interrupts based
	 * on the variable `key' populated by qm_irq_lock().
	 */
	__asm__ __volatile__("seti %0" : : "ir"(key));
}

#else  /* x86 */

unsigned int qm_irq_lock(void)
{
	unsigned int key = 0;

	/*
	 * Store the CPU FLAGS register into the variable `key' and disable
	 * interrupt delivery to the core.
	 */
	__asm__ __volatile__("pushfl;\n\t"
			     "cli;\n\t"
			     "popl %0;\n\t"
			     : "=g"(key)
			     :
			     : "memory");

	return key;
}

void qm_irq_unlock(unsigned int key)
{
	/*
	 * `key' holds the CPU FLAGS register content at the time when
	 * qm_irq_lock() was called.
	 */
	if (!(key & X86_FLAGS_IF)) {
		/*
		 * Interrupts were disabled when qm_irq_lock() was invoked:
		 * do not re-enable interrupts.
		 */
		return;
	}

	/* Enable interrupts */
	__asm__ __volatile__("sti;\n\t" : :);
}
#endif /* QM_SENSOR */

void qm_irq_mask(uint32_t irq)
{
#if (HAS_APIC)
	ioapic_mask_irq(irq);

#elif(HAS_MVIC)
	mvic_mask_irq(irq);

#elif(QM_SENSOR)
	qm_ss_irq_mask(irq);

#endif
}

void qm_irq_unmask(uint32_t irq)
{
#if (HAS_APIC)
	ioapic_unmask_irq(irq);

#elif(HAS_MVIC)
	mvic_unmask_irq(irq);

#elif(QM_SENSOR)
	qm_ss_irq_unmask(irq);

#endif
}

#if (ENABLE_RESTORE_CONTEXT)
#if (HAS_APIC)
int qm_irq_save_context(qm_irq_context_t *const ctx)
{
	uint32_t rte_low;
	uint8_t irq;

	QM_CHECK(ctx != NULL, -EINVAL);

	for (irq = 0; irq < QM_IOAPIC_NUM_RTES; irq++) {
		rte_low = _ioapic_get_redtbl_entry_lo(irq);
		ctx->redtbl_entries[irq] = rte_low;
	}

	return 0;
}

int qm_irq_restore_context(const qm_irq_context_t *const ctx)
{
	uint32_t rte_low;
	uint8_t irq;

	QM_CHECK(ctx != NULL, -EINVAL);

	apic_init();

	for (irq = 0; irq < QM_IOAPIC_NUM_RTES; irq++) {
		rte_low = ctx->redtbl_entries[irq];
		_ioapic_set_redtbl_entry_lo(irq, rte_low);
	}

	return 0;
}
#elif(QM_SENSOR) /* HAS_APIC */
int qm_irq_save_context(qm_irq_context_t *const ctx)
{
	uint8_t i;
	uint32_t status32;

	QM_CHECK(ctx != NULL, -EINVAL);

	/* Interrupts from 0 to 15 are exceptions and they are ignored
	 * by IRQ auxiliary registers. For that reason we skip those
	 * values in this loop.
	 */
	for (i = 0; i < (QM_SS_INT_VECTOR_NUM - QM_SS_EXCEPTION_NUM); i++) {
		__builtin_arc_sr(i + QM_SS_EXCEPTION_NUM, QM_SS_AUX_IRQ_SELECT);

		ctx->irq_config[i] = __builtin_arc_lr(QM_SS_AUX_IRQ_PRIORITY)
				     << 2;
		ctx->irq_config[i] |= __builtin_arc_lr(QM_SS_AUX_IRQ_TRIGGER)
				      << 1;
		ctx->irq_config[i] |= __builtin_arc_lr(QM_SS_AUX_IRQ_ENABLE);
	}

	status32 = __builtin_arc_lr(QM_SS_AUX_STATUS32);

	ctx->status32_irq_threshold = status32 & QM_SS_STATUS32_E_MASK;
	ctx->status32_irq_enable = status32 & QM_SS_STATUS32_IE_MASK;
	ctx->irq_ctrl = __builtin_arc_lr(QM_SS_AUX_IRQ_CTRL);

	return 0;
}

int qm_irq_restore_context(const qm_irq_context_t *const ctx)
{
	uint8_t i;
	uint32_t reg;

	QM_CHECK(ctx != NULL, -EINVAL);

	for (i = 0; i < (QM_SS_INT_VECTOR_NUM - QM_SS_EXCEPTION_NUM); i++) {
		__builtin_arc_sr(i + QM_SS_EXCEPTION_NUM, QM_SS_AUX_IRQ_SELECT);

		__builtin_arc_sr(ctx->irq_config[i] >> 2,
				 QM_SS_AUX_IRQ_PRIORITY);
		__builtin_arc_sr((ctx->irq_config[i] >> 1) & BIT(0),
				 QM_SS_AUX_IRQ_TRIGGER);
		__builtin_arc_sr(ctx->irq_config[i] & BIT(0),
				 QM_SS_AUX_IRQ_ENABLE);
	}

	__builtin_arc_sr(ctx->irq_ctrl, QM_SS_AUX_IRQ_CTRL);

	/* Setting an interrupt priority threshold. */
	reg = __builtin_arc_lr(QM_SS_AUX_STATUS32);
	reg |= (ctx->status32_irq_threshold & QM_SS_STATUS32_E_MASK);
	reg |= (ctx->status32_irq_enable & QM_SS_STATUS32_IE_MASK);

	/* This one has to be a kernel operation. */
	__builtin_arc_kflag(reg);

	return 0;
}
#endif		 /* QM_SENSOR */
#else		 /* !ENABLE_RESTORE_CONTEXT */
int qm_irq_save_context(qm_irq_context_t *const ctx)
{
	(void)ctx;

	return 0;
}

int qm_irq_restore_context(const qm_irq_context_t *const ctx)
{
	(void)ctx;

	return 0;
}
#endif		 /* ENABLE_RESTORE_CONTEXT */

void _qm_irq_setup(uint32_t irq)
{
#if (HAS_APIC)
	/*
	 * Quark SE SOC has an APIC. Other SoCs uses a simple, fixed-vector
	 * non-8259 PIC that requires no configuration.
	 */
	ioapic_register_irq(irq, QM_IRQ_TO_VECTOR(irq));
	ioapic_unmask_irq(irq);
#elif(HAS_MVIC)
	mvic_register_irq(irq);
	mvic_unmask_irq(irq);
#elif(QM_SENSOR)
	ss_register_irq(QM_IRQ_TO_VECTOR(irq));
	qm_ss_irq_unmask(QM_IRQ_TO_VECTOR(irq));
#endif
}

/*
 * Register an Interrupt Service Routine to a given interrupt vector.
 *
 * @param[in] vector Interrupt Vector number.
 * @param[in] isr ISR to register to given vector. Must be a valid x86 ISR.
 *            If this can't be provided, QM_IRQ_REQUEST() or
 *            qm_int_vector_request() should be used instead.
 */
void _qm_register_isr(uint32_t vector, qm_isr_t isr)
{
#if (QM_SENSOR)
	/* Invalidate the i-cache line which contains the IRQ vector. This
	 * will bypass i-cache and set vector with the good ISR. */
	__builtin_arc_sr((uint32_t)&__ivt_vect_table[0] + (vector * 4),
			 QM_SS_AUX_IC_IVIL);
	/* All SR accesses to the IC_IVIL register must be followed by three
	 * NOP instructions, see chapter 3.3.59 in the datasheet
	 * "ARC_V2_ProgrammersReference.pdf" */
	__builtin_arc_nop();
	__builtin_arc_nop();
	__builtin_arc_nop();
	__ivt_vect_table[vector] = isr;
#else
	idt_set_intr_gate_desc(vector, (uint32_t)isr);
#endif
}

#if (QM_SENSOR)
static void ss_register_irq(unsigned int vector)
{
	/*
	 * By hardware power-on default, SS interrupts are level triggered.
	 * The following switch statement sets some of the peripherals to edge
	 * triggered.
	 */
	switch (vector) {
	case QM_SS_IRQ_ADC_0_PWR_INT_VECTOR:
	case QM_IRQ_RTC_0_INT_VECTOR:
	case QM_IRQ_AONPT_0_INT_VECTOR:
	case QM_IRQ_WDT_0_INT_VECTOR:
		/* Edge sensitive. */
		__builtin_arc_sr(vector, QM_SS_AUX_IRQ_SELECT);
		__builtin_arc_sr(QM_SS_IRQ_EDGE_SENSITIVE,
				 QM_SS_AUX_IRQ_TRIGGER);
	}
}
#endif
