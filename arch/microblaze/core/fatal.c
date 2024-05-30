/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include <offsets_short.h>

/* Hardware includes. */
#include "microblaze/mb_interface.h"
#include "microblaze/microblaze_regs.h"

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* The number of bytes a MicroBlaze instruction consumes. */
#define MICROBLAZE_INSTRUCTION_SIZE 4

extern void _exception_handler_entry(void *exception_id);

/* Used by assembly routine _exception_handler_entry to store sp */
void *stack_pointer_on_exception_entry;

FUNC_NORETURN void z_microblaze_fatal_error(unsigned int reason,
							const microblaze_register_dump_t *dump)
{
	if (dump != NULL && IS_ENABLED(CONFIG_MICROBLAZE_DUMP_ON_EXCEPTION)) {
		printk("r1:\t0x%08x\t(sp)\n", dump->esf.r1);
		printk("r2:\t0x%08x\t(small data area)\n", dump->esf.r2);
		printk("r3:\t0x%x\t\t(retval 1)\n", dump->esf.r3);
		printk("r4:\t0x%x\t\t(retval 2)\n", dump->esf.r4);
		printk("r5:\t0x%x\t\t(arg1)\n", dump->esf.r5);
		printk("r6:\t0x%x\t\t(arg2)\n", dump->esf.r6);
		printk("r7:\t0x%x\t\t(arg3)\n", dump->esf.r7);
		printk("r8:\t0x%x\t\t(arg4)\n", dump->esf.r8);
		printk("r9:\t0x%x\t\t(arg5)\n", dump->esf.r9);
		printk("r10:\t0x%x\t\t(arg6)\n", dump->esf.r10);
		printk("r11:\t0x%08x\t(temp1)\n", dump->esf.r11);
		printk("r12:\t0x%08x\t(temp2)\n", dump->esf.r12);
		printk("r13:\t0x%08x\t(rw small data area)\n", dump->esf.r13);
		printk("r14:\t0x%08x\t(return from interrupt)\n", dump->esf.r14);
		printk("r15:\t0x%08x\t(return from subroutine)\n", dump->esf.r15);
		printk("r16:\t0x%08x\t(return from trap)\n", dump->esf.r16);
		printk("r17:\t0x%08x\t(return from exception)\n", dump->esf.r17);
		printk("r18:\t0x%08x\t(compiler/assembler temp)\n", dump->esf.r18);
		printk("r19:\t0x%08x\t(global offset table ptr)\n", dump->esf.r19);
		printk("r20:\t0x%x\n", dump->esf.r20);
		printk("r21:\t0x%x\n", dump->esf.r21);
		printk("r22:\t0x%x\n", dump->esf.r22);
		printk("r23:\t0x%x\n", dump->esf.r23);
		printk("r24:\t0x%x\n", dump->esf.r24);
		printk("r25:\t0x%x\n", dump->esf.r25);
		printk("r26:\t0x%x\n", dump->esf.r26);
		printk("r27:\t0x%x\n", dump->esf.r27);
		printk("r28:\t0x%x\n", dump->esf.r28);
		printk("r29:\t0x%x\n", dump->esf.r29);
		printk("r30:\t0x%x\n", dump->esf.r30);
		printk("r31:\t0x%x\n", dump->esf.r31);

		printk("MSR:\t0x%08x\t(exc)\n", dump->esf.msr);
#if defined(CONFIG_MICROBLAZE_USE_HARDWARE_FLOAT_INSTR)
		printk("FSR:\t%08x\n", dump->esf.fsr);
#endif
		printk("ESR:\t0x%08x\n", dump->esr);
		printk("EAR:\t0x%x\n", dump->ear);
		printk("EDR:\t0x%x\n", dump->edr);
		printk("PC:\t0x%x\n", dump->pc);
	}

	/* This hack allows us to re-enable exceptions properly before continuing.
	 * r15 is safe to use becaue this function is noreturn.
	 */
	__asm__ volatile("\tmfs r15, rpc\n"
			 "\trted r15, 0x8\n"
			 "\tnop\n");

	printk("MSR:\t0x%08x\t(%s)\n", mfmsr(), __func__);

	z_fatal_error(reason, &dump->esf);
	CODE_UNREACHABLE;
}

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
static char *cause_str(uint32_t cause)
{
	switch (cause) {
	case 0:
		return "stream exception";
	case 1:
		return "unaligned data access exception";
	case 2:
		return "illegal op-code exception";
	case 3:
		return "instruction bus error exception";
	case 4:
		return "data bus error exception";
	case 5:
		return "divide exception";
	case 6:
		return "floating point unit exception";
	case 7:
		return "privileged instruction exception";
	case 8:
		return "stack protection violation exception";
	case 9:
		return "data storage exception";
	case 10:
		return "instruction storage exception";
	case 11:
		return "data TLB miss exception";
	case 12:
		return "instruction TLB miss exception";
	default:
		return "unknown";
	}
}
#endif /* defined(CONFIG_EXTRA_EXCEPTION_INFO) */

FUNC_NORETURN void _Fault(uint32_t esr, uint32_t ear, uint32_t edr)
{
	static microblaze_register_dump_t microblaze_register_dump = {0};
	/* Log the simplest possible exception information before anything */
	uint32_t cause = (mfesr() & CAUSE_EXP_MASK) >> CAUSE_EXP_SHIFT;

	LOG_ERR("");
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	LOG_ERR("Cause: %d, %s", cause, cause_str(cause));
#endif /* defined(CONFIG_EXTRA_EXCEPTION_INFO) */

	/* Fill an register dump structure with the MicroBlaze context as it
	 * was immediately before the exception occurrence.
	 */

	__ASSERT_NO_MSG(stack_pointer_on_exception_entry);
	z_arch_esf_t *sp_ptr = (z_arch_esf_t *)stack_pointer_on_exception_entry;

	/* Obtain the values of registers that were stacked prior to this function
	 * being called, and may have changed since they were stacked.
	 */
	microblaze_register_dump.esf = *sp_ptr;
	microblaze_register_dump.esf.r1 = ((uint32_t)sp_ptr) + sizeof(z_arch_esf_t);
	microblaze_register_dump.esr = esr;
	microblaze_register_dump.ear = ear;
	microblaze_register_dump.edr = edr;

	/* Move the saved program counter back to the instruction that was executed
	 * when the exception occurred.  This is only valid for certain types of
	 * exception.
	 */
	microblaze_register_dump.pc =
		microblaze_register_dump.esf.r17 - MICROBLAZE_INSTRUCTION_SIZE;

	/* Also fill in a string that describes what type of exception this is.
	 * The string uses the same ID names as defined in the MicroBlaze standard
	 * library exception header files.
	 */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	microblaze_register_dump.exception_cause_str = cause_str(cause);
#endif /* defined(CONFIG_EXTRA_EXCEPTION_INFO) */

	z_microblaze_fatal_error(K_ERR_CPU_EXCEPTION, &microblaze_register_dump);
	CODE_UNREACHABLE;
}
