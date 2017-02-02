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

#ifndef __QM_INTERRUPT_ROUTER_H__
#define __QM_INTERRUPT_ROUTER_H__

/**
 * Quark D2000 SoC Interrupt Router.
 *
 * @defgroup groupQUARKD2000INTERRUPTROUTER SoC Interrupt Router (D2000)
 * @{
 */

void _qm_ir_mask_int(uint32_t irq, uint32_t register_offset);
void _qm_ir_unmask_int(uint32_t irq, uint32_t register_offset);

/*
 * Unmask a given IRQ in the Interrupt Router.
 *
 * @param[in] irq IRQ number. Must be of type QM_IRQ_XXX.
 */
#define QM_IR_UNMASK_INT(irq)                                                  \
	do {                                                                   \
		_qm_ir_unmask_int(irq, irq##_MASK_OFFSET);                     \
	} while (0);

/*
 * Mask a given IRQ in the Interrupt Router.
 *
 * @param[in] irq IRQ number. Must be of type QM_IRQ_XXX.
 */
#define QM_IR_MASK_INT(irq)                                                    \
	do {                                                                   \
		_qm_ir_mask_int(irq, irq##_MASK_OFFSET);                       \
	} while (0);

/** @} */

#endif /* __QM_INTERRUPT_ROUTER_H__ */
