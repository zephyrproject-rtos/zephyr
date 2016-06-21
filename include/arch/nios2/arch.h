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
#include <arch/nios2/asm_inline.h>
#include "nios2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ALIGN  4

#define _NANO_ERR_CPU_EXCEPTION (0)     /* Any unhandled exception */
#define _NANO_ERR_INVALID_TASK_EXIT (1) /* Invalid task exit */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */
#define _NANO_ERR_SPURIOUS_INT (4)	/* Spurious interrupt */

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <irq.h>

/* STUB. Eventually port ARC/ARM interrupt stuff */
#define _ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)

static ALWAYS_INLINE unsigned int _arch_irq_lock(void)
{
	unsigned int key;

	key = _nios2_creg_read(NIOS2_CR_STATUS);
	_nios2_creg_write(NIOS2_CR_STATUS, key & ~NIOS2_STATUS_PIE_MSK);

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

	status_reg = _nios2_creg_read(NIOS2_CR_STATUS);
	_nios2_creg_write(NIOS2_CR_STATUS, status_reg | NIOS2_STATUS_PIE_MSK);
#else
	_nios2_creg_write(NIOS2_CR_STATUS, key);
#endif
}

int _arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(void *parameter), void *parameter,
			      uint32_t flags);
void _arch_irq_enable(unsigned int irq);
void _arch_irq_disable(unsigned int irq);

struct __esf {
	uint32_t ra; /* return address r31 */
	uint32_t r0; /* zero register */
	uint32_t r1; /* at */
	uint32_t r2; /* return value */
	uint32_t r3; /* return value */
	uint32_t r4; /* register args */
	uint32_t r5; /* register args */
	uint32_t r6; /* register args */
	uint32_t r7; /* register args */
	uint32_t r8; /* Caller-saved general purpose */
	uint32_t r9; /* Caller-saved general purpose */
	uint32_t r10; /* Caller-saved general purpose */
	uint32_t r11; /* Caller-saved general purpose */
	uint32_t r12; /* Caller-saved general purpose */
	uint32_t r13; /* Caller-saved general purpose */
	uint32_t r14; /* Caller-saved general purpose */
	uint32_t r15; /* Caller-saved general purpose */
	uint32_t estatus;
	uint32_t instr; /* Instruction being executed when exc occurred */
};

typedef struct __esf NANO_ESF;
extern const NANO_ESF _default_esf;

FUNC_NORETURN void _SysFatalErrorHandler(unsigned int reason,
					 const NANO_ESF *esf);


enum nios2_exception_cause {
	NIOS2_EXCEPTION_UNKNOWN                      = -1,
	NIOS2_EXCEPTION_RESET                        = 0,
	NIOS2_EXCEPTION_CPU_ONLY_RESET_REQUEST       = 1,
	NIOS2_EXCEPTION_INTERRUPT                    = 2,
	NIOS2_EXCEPTION_TRAP_INST                    = 3,
	NIOS2_EXCEPTION_UNIMPLEMENTED_INST           = 4,
	NIOS2_EXCEPTION_ILLEGAL_INST                 = 5,
	NIOS2_EXCEPTION_MISALIGNED_DATA_ADDR         = 6,
	NIOS2_EXCEPTION_MISALIGNED_TARGET_PC         = 7,
	NIOS2_EXCEPTION_DIVISION_ERROR               = 8,
	NIOS2_EXCEPTION_SUPERVISOR_ONLY_INST_ADDR    = 9,
	NIOS2_EXCEPTION_SUPERVISOR_ONLY_INST         = 10,
	NIOS2_EXCEPTION_SUPERVISOR_ONLY_DATA_ADDR    = 11,
	NIOS2_EXCEPTION_TLB_MISS                     = 12,
	NIOS2_EXCEPTION_TLB_EXECUTE_PERM_VIOLATION   = 13,
	NIOS2_EXCEPTION_TLB_READ_PERM_VIOLATION      = 14,
	NIOS2_EXCEPTION_TLB_WRITE_PERM_VIOLATION     = 15,
	NIOS2_EXCEPTION_MPU_INST_REGION_VIOLATION    = 16,
	NIOS2_EXCEPTION_MPU_DATA_REGION_VIOLATION    = 17,
	NIOS2_EXCEPTION_ECC_TLB_ERR                  = 18,
	NIOS2_EXCEPTION_ECC_FETCH_ERR                = 19,
	NIOS2_EXCEPTION_ECC_REGISTER_FILE_ERR        = 20,
	NIOS2_EXCEPTION_ECC_DATA_ERR                 = 21,
	NIOS2_EXCEPTION_ECC_DATA_CACHE_WRITEBACK_ERR = 22
};

/* Bitfield indicating which exception cause codes report a valid
 * badaddr register. NIOS2_EXCEPTION_TLB_MISS and NIOS2_EXCEPTION_ECC_TLB_ERR
 * are deliberately not included here, you need to check if TLBMISC.D=1
 */
#define NIOS2_BADADDR_CAUSE_MASK \
	(BIT(NIOS2_EXCEPTION_SUPERVISOR_ONLY_DATA_ADDR) | \
	 BIT(NIOS2_EXCEPTION_MISALIGNED_DATA_ADDR) | \
	 BIT(NIOS2_EXCEPTION_MISALIGNED_TARGET_PC) | \
	 BIT(NIOS2_EXCEPTION_TLB_READ_PERM_VIOLATION) | \
	 BIT(NIOS2_EXCEPTION_TLB_WRITE_PERM_VIOLATION) | \
	 BIT(NIOS2_EXCEPTION_MPU_DATA_REGION_VIOLATION) | \
	 BIT(NIOS2_EXCEPTION_ECC_DATA_ERR))


#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif
