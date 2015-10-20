/* arch.h - ARC specific nanokernel interface header */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * DESCRIPTION
 * This header contains the ARC specific nanokernel interface.  It is
 * included by the nanokernel interface architecture-abstraction header
 * (nanokernel/cpu.h)
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
		irq_connect(irq, priority, isr, NULL);

#endif /* _ARC_ARCH__H_ */
