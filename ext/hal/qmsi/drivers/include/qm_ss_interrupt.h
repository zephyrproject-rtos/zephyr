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

#ifndef __QM_SS_INTERRUPT_H__
#define __QM_SS_INTERRUPT_H__

#include "qm_common.h"
#include "qm_sensor_regs.h"

/**
 * Interrupt driver for Sensor Subsystem.
 *
 * @defgroup groupSSINT SS Interrupt
 * @{
 */

/**
 * Interrupt service routine type.
 */
typedef void (*qm_ss_isr_t)(struct interrupt_frame *frame);

/**
 * Enable interrupt delivery for the Sensor Subsystem.
 */
void qm_ss_irq_enable(void);

/**
 * Disable interrupt delivery for the Sensor Subsystem.
 */
void qm_ss_irq_disable(void);

/**
 * Unmask a given interrupt line.
 *
 * @param [in] irq Which IRQ to unmask.
 */
void qm_ss_irq_unmask(uint32_t irq);

/**
 * Mask a given interrupt line.
 *
 * @param [in] irq Which IRQ to mask.
 */
void qm_ss_irq_mask(uint32_t irq);

/**
 * Request a given IRQ and register ISR to interrupt vector.
 *
 * @param [in] irq IRQ number.
 * @param [in] isr ISR to register to given IRQ.
 */
void qm_ss_irq_request(uint32_t irq, qm_ss_isr_t isr);

/**
 * Register an Interrupt Service Routine to a given interrupt vector.
 *
 * @param [in] vector Interrupt Vector number.
 * @param [in] isr ISR to register to given vector. Must be a valid Sensor
 *             Subsystem ISR.
 */
void qm_ss_int_vector_request(uint32_t vector, qm_ss_isr_t isr);

/**
 * @}
 */
#endif /* __QM_SS_INTERRUPT_H__ */
