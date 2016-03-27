/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef __QM_INTERRUPT_H__
#define __QM_INTERRUPT_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/*
 * Linear mapping between IRQs and interrupt vectors
 */
#if (QUARK_SE)
#define QM_IRQ_TO_VECTOR(irq) (irq + 36)

#elif(QUARK_D2000)
#define QM_IRQ_TO_VECTOR(irq) (irq + 32)

#endif

/**
 * Interrupt driver for Quark Microcontrollers.
 *
 * @brief Interrupt for QM.
 * @defgroup groupINT Interrupt
 * @{
 */

/**
 * Interrupt service routine type
 */
typedef void (*qm_isr_t)(void);

/**
 * Enable interrupt delivery for the SoC.
 */
void qm_irq_enable(void);

/**
 * Disable interrupt delivery for the SoC.
 */
void qm_irq_disable(void);

/**
 * Unmask a given interrupt line.
 *
 * @param [in] irq Which IRQ to unmask.
 */
void qm_irq_unmask(uint32_t irq);

/**
 * Mask a given interrupt line.
 *
 * @param [in] irq Which IRQ to mask.
 */
void qm_irq_mask(uint32_t irq);

/**
 * Register an Interrupt Service Routine to a given interrupt vector.
 *
 * @param [in] vector Interrupt Vector number.
 * @param [in] isr ISR to register to given vector. Must be a valid x86 ISR.
 *             If this can't be provided, qm_irq_request() or
 *             qm_int_vector_request() should be used instead.
 */
void _qm_register_isr(uint32_t vector, qm_isr_t isr);

/**
 * Setup an IRQ and its routing on the Interrupt Controller.
 *
 * @param [in] irq IRQ number. Must be of type QM_IRQ_XXX.
 * @param [in] register_offset Interrupt Mask Register offset on SCSS.
 *             Must be of type QM_IRQ_XXX_MASK_OFFSET.
 */
void _qm_irq_setup(uint32_t irq, uint16_t register_offset);

/**
 * Request a given IRQ and register Interrupt Service Routine to interrupt
 * vector.
 *
 * @brief Request an IRQ and attach an ISR to it.
 * @param [in] irq IRQ number.  Must be of type QM_IRQ_XXX.
 * @param [in] isr ISR to register to given IRQ.
 */
#if (UNIT_TEST)
#define qm_irq_request(irq, isr)
#else
#define qm_irq_request(irq, isr)                                               \
	do {                                                                   \
		qm_int_vector_request(irq##_VECTOR, isr);                      \
                                                                               \
		_qm_irq_setup(irq, irq##_MASK_OFFSET);                         \
	} while (0)
#endif /* UNIT_TEST */

/**
 * Request an interrupt vector and register Interrupt Service Routine to it.
 *
 * @brief Request an interrupt vector and attach an ISR to it.
 * @param [in] vector Vector number.
 * @param [in] isr ISR to register to given IRQ.
 */
#if (UNIT_TEST)
#define qm_int_vector_request(vector, isr)
#else
#if (__iamcu == 1)
/* Using the IAMCU calling convention */
#define qm_int_vector_request(vector, isr)                                     \
	do {                                                                   \
		__asm__ __volatile__("mov $1f, %%edx\n\t"                      \
				     "mov %0, %%eax\n\t"                       \
				     "call %P1\n\t"                            \
				     "jmp 2f\n\t"                              \
				     ".align 4\n\t"                            \
				     "1:\n\t"                                  \
				     "         pushal\n\t"                     \
				     "         call %P2\n\t"                   \
				     "         popal\n\t"                      \
				     "         iret\n\t"                       \
				     "2:\n\t" ::"g"(vector),                   \
				     "i"(_qm_register_isr), "i"(isr)           \
				     : "%eax", "%ecx", "%edx");                \
	} while (0)
#else
/* Using the standard SysV calling convention */
#define qm_int_vector_request(vector, isr)                                     \
	do {                                                                   \
		__asm__ __volatile__("push $1f\n\t"                            \
				     "push %0\n\t"                             \
				     "call %P1\n\t"                            \
				     "add $8, %%esp\n\t"                       \
				     "jmp 2f\n\t"                              \
				     ".align 4\n\t"                            \
				     "1:\n\t"                                  \
				     "         pushal\n\t"                     \
				     "         call %P2\n\t"                   \
				     "         popal\n\t"                      \
				     "         iret\n\t"                       \
				     "2:\n\t" ::"g"(vector),                   \
				     "i"(_qm_register_isr), "i"(isr)           \
				     : "%eax", "%ecx", "%edx");                \
	} while (0)
#endif /* __iamcu == 1 */

#endif /* UNIT_TEST */

/**
 * @}
 */
#endif /* __QM_INTERRUPT_H__ */
