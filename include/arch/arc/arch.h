/* arch.h - ARC specific nanokernel interface header */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
This header contains the ARC specific nanokernel interface.  It is
included by the nanokernel interface architecture-abstraction header
(nanokernel/cpu.h)
 */

#ifndef _ARC_ARCH__H_
#define _ARC_ARCH__H_

#ifdef __cplusplus
extern "C" {
#endif

/* APIs need to support non-byte addressible architectures */

#define OCTET_TO_SIZEOFUNIT(X) (X)
#define SIZEOFUNIT_TO_OCTET(X) (X)

#ifdef CONFIG_CPU_ARCV2
#include <arch/arc/v2/exc.h>
#include <arch/arc/v2/irq.h>
#include <arch/arc/v2/ffs.h>
#include <arch/arc/v2/error.h>
#include <arch/arc/v2/misc.h>
#include <arch/arc/v2/aux_regs.h>
#include <arch/arc/v2/arcv2_irq_unit.h>
#include <arch/arc/v2/asm_inline.h>
#endif

#define STACK_ALIGN  4

#ifdef __cplusplus
}
#endif


/**
 * @brief Connect a routine to interrupt number
 *
 * For the device @a device associates IRQ number @a irq with priority
 * @a priority with the interrupt routine @a isr, that receives parameter
 * @a parameter.
 * IRQ connect static is currently not supported in ARC architecture.
 * The macro is defined as empty for code compatibility with other
 * architectures.
 *
 * @param device Device
 * @param irq IRQ number
 * @param priority IRQ Priority
 * @param isr Interrupt Service Routine
 * @param parameter ISR parameter
 *
 * @return N/A
 *
 */
#define IRQ_CONNECT_STATIC(device, irq, priority, isr, parameter)

/**
 *
 * @brief Configure interrupt for the device
 *
 * For the selected device, do the neccessary configuration
 * steps to connect and enable the IRQ line with an ISR
 * at the priority requested.
 * @param isr Pointer to the interruption service routine
 * @param irq IRQ number
 * @param priority Priority for the interruption
 *
 * @return N/A
 *
 */
#define IRQ_CONFIG(isr, irq, priority) \
		irq_connect(irq, priority, isr, NULL); \
		irq_enable(irq);

#endif /* _ARC_ARCH__H_ */
