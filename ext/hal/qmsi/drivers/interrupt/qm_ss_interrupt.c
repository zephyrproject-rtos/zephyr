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

#include "qm_ss_interrupt.h"
#include "qm_soc_regs.h"
#include "qm_sensor_regs.h"

/* SCSS base addr for Sensor Subsystem interrupt routing, for linear IRQ
 * mapping */
#define INTERRUPT_ROUTER_SS_INT_MASK_BASE                                      \
	(&QM_INTERRUPT_ROUTER->ss_adc_0_error_int_mask)

#if (UNIT_TEST)
qm_ss_isr_t __ivt_vect_table[QM_SS_INT_VECTOR_NUM];
#else
extern qm_ss_isr_t __ivt_vect_table[];
#endif

void qm_ss_irq_disable(void)
{
	__builtin_arc_clri();
}

void qm_ss_irq_enable(void)
{
	__builtin_arc_seti(0);
}

void qm_ss_irq_mask(uint32_t irq)
{
	__builtin_arc_sr(irq, QM_SS_AUX_IRQ_SELECT);
	__builtin_arc_sr(QM_SS_INT_DISABLE, QM_SS_AUX_IRQ_ENABLE);
}

void qm_ss_irq_unmask(uint32_t irq)
{
	__builtin_arc_sr(irq, QM_SS_AUX_IRQ_SELECT);
	__builtin_arc_sr(QM_SS_INT_ENABLE, QM_SS_AUX_IRQ_ENABLE);
}

void qm_ss_int_vector_request(uint32_t vector, qm_ss_isr_t isr)
{
	/* Invalidate the I-cache line which contains the irq vector. This
	 * will bypass I-Cach and set vector with the good isr. */
	__builtin_arc_sr((uint32_t)&__ivt_vect_table[0] + (vector * 4),
			 QM_SS_AUX_IC_IVIL);
	/* All SR accesses to the IC_IVIL register must be followed by three
	 * NOP instructions, see chapter 3.3.59 in the datasheet
	 * "ARC_V2_ProgrammersReference.pdf" */
	__builtin_arc_nop();
	__builtin_arc_nop();
	__builtin_arc_nop();
	__ivt_vect_table[vector] = isr;
}

void qm_ss_irq_request(uint32_t irq, qm_ss_isr_t isr)
{
	uint32_t vector = irq + (QM_SS_EXCEPTION_NUM + QM_SS_INT_TIMER_NUM);

	/* Guarding the IRQ set-up */
	qm_ss_irq_mask(vector);

	qm_ss_int_vector_request(vector, isr);

	qm_ss_irq_unmask(vector);
}
