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
#include "qm_soc_regs.h"
#include "qm_interrupt_router.h"
#include "qm_interrupt_router_regs.h"

/* Event router base addr for LMT interrupt routing, for linear IRQ mapping */
#define INTERRUPT_ROUTER_LMT_INT_MASK_BASE                                     \
	(&QM_INTERRUPT_ROUTER->i2c_master_0_int_mask)

void _qm_ir_unmask_int(uint32_t irq, uint32_t register_offset)
{
	uint32_t *interrupt_router_intmask;

	/* Route peripheral interrupt to Lakemont/Sensor Subsystem */
	interrupt_router_intmask =
	    (uint32_t *)INTERRUPT_ROUTER_LMT_INT_MASK_BASE + register_offset;

	if (!QM_IR_INT_LOCK_MASK(*interrupt_router_intmask)) {
		switch (irq) {
		case QM_IRQ_COMPARATOR_0_INT:
/*
 * Comparator mask uses 1 bit per comparator rather than the
 * generic host mask.
 */
#if (QM_SENSOR)
			QM_INTERRUPT_ROUTER->comparator_0_ss_int_mask &=
			    ~0x0007ffff;
#else
			QM_INTERRUPT_ROUTER->comparator_0_host_int_mask &=
			    ~0x0007ffff;
#endif
			break;
		case QM_IRQ_MAILBOX_0_INT:
			/* Masking MAILBOX irq is done inside mbox driver */
			break;
		case QM_IRQ_DMA_0_ERROR_INT:
/*
 * DMA error mask uses 1 bit per DMA channel rather than the
 * generic host mask.
 */
#if (QM_SENSOR)
			*interrupt_router_intmask &= ~QM_IR_DMA_ERROR_SS_MASK;
#else
			*interrupt_router_intmask &= ~QM_IR_DMA_ERROR_HOST_MASK;
#endif
			break;
		default:
			QM_IR_UNMASK_INTERRUPTS(*interrupt_router_intmask);
			break;
		}
	}
}

void _qm_ir_mask_int(uint32_t irq, uint32_t register_offset)
{
	uint32_t *interrupt_router_intmask;

	/* Route peripheral interrupt to Lakemont/Sensor Subsystem */
	interrupt_router_intmask =
	    (uint32_t *)INTERRUPT_ROUTER_LMT_INT_MASK_BASE + register_offset;

	/**/
	if (!QM_IR_INT_LOCK_MASK(*interrupt_router_intmask)) {
		switch (irq) {
		case QM_IRQ_COMPARATOR_0_INT:
#if (QM_SENSOR)
			QM_INTERRUPT_ROUTER->comparator_0_ss_int_mask |=
			    0x0007ffff;
#else
			QM_INTERRUPT_ROUTER->comparator_0_host_int_mask |=
			    0x0007ffff;
#endif
			break;
		case QM_IRQ_MAILBOX_0_INT:
			/* Masking MAILBOX irq id done inside mbox driver */
			break;
		case QM_IRQ_DMA_0_ERROR_INT:
/*
 * DMA error mask uses 1 bit per DMA channel rather than the
 * generic host mask.
 */
#if (QM_SENSOR)
			*interrupt_router_intmask |= QM_IR_DMA_ERROR_SS_MASK;
#else
			*interrupt_router_intmask |= QM_IR_DMA_ERROR_HOST_MASK;
#endif
			break;
		default:
			QM_IR_MASK_INTERRUPTS(*interrupt_router_intmask);
			break;
		}
	}
}
