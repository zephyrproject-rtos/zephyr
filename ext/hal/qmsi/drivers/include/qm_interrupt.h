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

#ifndef __QM_INTERRUPT_H__
#define __QM_INTERRUPT_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/*
 * Linear mapping between IRQs and interrupt vectors
 */
#if (QUARK_SE)
#define QM_IRQ_TO_VECTOR(irq) (irq + 36) /* Get the vector of and IRQ. */

#elif(QUARK_D2000)
#define QM_IRQ_TO_VECTOR(irq) (irq + 32) /* Get the vector of and IRQ. */

#endif

/**
 * Interrupt driver.
 *
 * @defgroup groupINT Interrupt
 * @{
 */

/**
 * Interrupt service routine type
 */
typedef void (*qm_isr_t)(struct interrupt_frame *frame);

/**
 * Unconditionally enable interrupt delivery on the CPU.
 */
void qm_irq_enable(void);

/**
 * Unconditionally disable interrupt delivery on the CPU.
 */
void qm_irq_disable(void);

/**
 * Save interrupt state and disable all interrupts on the CPU.
 *
 * This routine disables interrupts. It can be called from either interrupt or
 * non-interrupt context.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to qm_irq_unlock() to re-enable interrupts.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * @return An architecture-dependent lock-out key representing the "interrupt
 * 	   disable state" prior to the call.
 *
 */
unsigned int qm_irq_lock(void);

/**
 *
 * Restore previous interrupt state on the CPU saved via qm_irq_lock().
 *
 * @param[in] key architecture-dependent lock-out key returned by a previous
 * 		  invocation of qm_irq_lock().
 */
void qm_irq_unlock(unsigned int key);

/**
 * Unmask a given interrupt line.
 *
 * @param[in] irq Which IRQ to unmask.
 */
void qm_irq_unmask(uint32_t irq);

/**
 * Mask a given interrupt line.
 *
 * @param[in] irq Which IRQ to mask.
 */
void qm_irq_mask(uint32_t irq);

void _qm_register_isr(uint32_t vector, qm_isr_t isr);

void _qm_irq_setup(uint32_t irq, uint16_t register_offset);

/*
 * Request a given IRQ and register Interrupt Service Routine to interrupt
 * vector.
 *
 * @param[in] irq IRQ number. Must be of type QM_IRQ_XXX.
 * @param[in] isr ISR to register to given IRQ.
 */
#if (QM_SENSOR)
#define qm_irq_request(irq, isr)                                               \
	do {                                                                   \
		_qm_register_isr(irq##_VECTOR, isr);                           \
		_qm_irq_setup(irq, irq##_MASK_OFFSET);                         \
	} while (0);
#else
#define qm_irq_request(irq, isr)                                               \
	do {                                                                   \
		qm_int_vector_request(irq##_VECTOR, isr);                      \
                                                                               \
		_qm_irq_setup(irq, irq##_MASK_OFFSET);                         \
	} while (0)
#endif /* QM_SENSOR */

/**
 * Request an interrupt vector and register Interrupt Service Routine to it.
 *
 * @param[in] vector Vector number.
 * @param[in] isr ISR to register to given IRQ.
 */
#if (UNIT_TEST)
void qm_int_vector_request(uint32_t vector, qm_isr_t isr);
#else
#if (__iamcu__)
/*
 * We assume that if the compiler supports the IAMCU ABI it also
 * supports the 'interrupt' attribute.
 */
static __inline__ void qm_int_vector_request(uint32_t vector, qm_isr_t isr)
{
	_qm_register_isr(vector, isr);
}

#else /* __iamcu__ */

/*
 * Using the standard SysV calling convention. A dummy (NULL in this case)
 * parameter is added to ISR handler, to maintain consistency with the API
 * imposed by the __attribute__((interrupt)) usage.
 */
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
				     "         push $0x00\n\t"                 \
				     "         call %P2\n\t"                   \
				     "         add $4, %%esp\n\t"              \
				     "         popal\n\t"                      \
				     "         iret\n\t"                       \
				     "2:\n\t" ::"g"(vector),                   \
				     "i"(_qm_register_isr), "i"(isr)           \
				     : "%eax", "%ecx", "%edx");                \
	} while (0)
#endif /* __iamcu__ */
#endif /* UNIT_TEST */

/**
 * @}
 */
#endif /* __QM_INTERRUPT_H__ */
