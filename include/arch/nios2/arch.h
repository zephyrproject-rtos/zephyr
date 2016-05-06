/*
 * Copyright (c) 2016 Intel Corporation
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

/**
 * @file
 * @brief Nios II specific nanokernel interface header
 * This header contains the Nios II specific nanokernel interface.  It is
 * included by the generic nanokernel interface header (nanokernel.h)
 */

#ifndef _ARCH_IFACE_H
#define _ARCH_IFACE_H

#include <system.h>
#include "nios2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ALIGN  4

#define _NANO_ERR_HW_EXCEPTION (0)      /* MPU/Bus/Usage fault */
#define _NANO_ERR_INVALID_TASK_EXIT (1) /* Invalid task exit */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <irq.h>
#include <arch/nios2/asm_inline.h>

/* STUB. Eventually port ARC/ARM interrupt stuff */
#define _ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)

static ALWAYS_INLINE unsigned int _arch_irq_lock(void)
{
	unsigned int key;

	NIOS2_READ_STATUS(key);
	NIOS2_WRITE_STATUS(key & ~NIOS2_STATUS_PIE_MSK);

	return key;
}

static ALWAYS_INLINE void _arch_irq_unlock(unsigned int key)
{
	/* If the CPU is built without certain features, then
	 * the only writable bit in the status register is PIE
	 * in which case we can just write the value stored in key,
	 * all the other writable bits will be the same.
	 *
	 * If not, other stuff could have changed and we need to
	 * specifically flip just that bit.
	 */

#if (NIOS2_NUM_OF_SHADOW_REG_SETS > 0) || \
		(defined NIOS2_EIC_PRESENT) || \
		(defined NIOS2_MMU_PRESENT) || \
		(defined NIOS2_MPU_PRESENT)
	uint32_t status_reg;

	/* Interrupts were already locked when irq_lock() was called,
	 * so don't do anything
	 */
	if (!(key & NIOS2_STATUS_PIE_MSK))
		return;

	NIOS2_READ_STATUS(status_reg);
	NIOS2_WRITE_STATUS(status_reg | NIOS2_STATUS_PIE_MSK);
#else
	NIOS2_WRITE_STATUS(key);
#endif
}

int _arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(void *parameter), void *parameter,
			      uint32_t flags);
void _arch_irq_enable(unsigned int irq);
void _arch_irq_disable(unsigned int irq);

struct __esf {
	/* XXX - not defined yet */
	uint32_t placeholder;
};

typedef struct __esf NANO_ESF;
extern const NANO_ESF _default_esf;

FUNC_NORETURN void _SysFatalErrorHandler(unsigned int reason,
					 const NANO_ESF *esf);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif
