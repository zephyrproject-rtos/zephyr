/*
 * Copyright (c) 2025 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/fatal_types.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/common/exc_handle.h>
#include <zephyr/arch/tricore/exception.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#include "kernel_arch_data.h"

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_tricore_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_tricore_user_string_nlen),
};
#endif /* CONFIG_USERSPACE */

static const char *const z_tricore_trap_cause_str(uint8_t trap_class, uint8_t tin)
{
	/* Trap description tables indexed by trap class and TIN.
	 * For classes where TIN numbering starts at 1, index 0 is reserved.
	 */
	static const char *const trap_class0_cause_str[] = {
		/* TIN 0 */ "Virtual Address Fill",
		/* TIN 1 */ "Virtual Address Protection",
	};

	static const char *const trap_class1_cause_str[] = {
		/* TIN 0 */ "Reserved",
		/* TIN 1 */ "Privileged Instruction",
		/* TIN 2 */ "Memory Protection Read",
		/* TIN 3 */ "Memory Protection Write",
		/* TIN 4 */ "Memory Protection Execute",
		/* TIN 5 */ "Memory Protection Peripheral Access",
		/* TIN 6 */ "Memory Protection Null Address",
		/* TIN 7 */ "Global Register Write Protection",
	};

	static const char *const trap_class2_cause_str[] = {
		/* TIN 0 */ "Reserved",
		/* TIN 1 */ "Illegal Opcode",
		/* TIN 2 */ "Unimplemented Opcode",
		/* TIN 3 */ "Invalid Operand specification",
		/* TIN 4 */ "Data Address Alignment",
		/* TIN 5 */ "Invalid Local Memory Address",
	};

	static const char *const trap_class3_cause_str[] = {
		/* TIN 0 */ "Reserved",
		/* TIN 1 */ "Free Context List Depletion (FCX = LCX)",
		/* TIN 2 */ "Call Depth Overflow (CALL with PSW.CDC.COUNT at maximum level)",
		/* TIN 3 */ "Call Depth Underflow (RET with PSW.CDC.COUNT == zero)",
		/* TIN 4 */ "Free Context List Underflow (FCX = 0)",
		/* TIN 5 */ "Call Stack Underflow (PCX = 0)",
		/* TIN 6 */ "Context Type (PCXI.UL wrong)",
		/* TIN 7 */ "Nesting Error: RFE with non-zero call depth",
	};

	static const char *const trap_class4_cause_str[] = {
		/* TIN 0 */ "Reserved",
		/* TIN 1 */ "Program Fetch Synchronous Error",
		/* TIN 2 */ "Data Access Synchronous Error",
		/* TIN 3 */ "Data Access Asynchronous Error",
		/* TIN 4 */ "Coprocessor Trap Asynchronous Error",
		/* TIN 5 */ "Program Memory Integrity Error",
		/* TIN 6 */ "Data Memory Integrity Error",
		/* TIN 7 */ "Temporal Asynchronous Error",
	};

	static const char *const trap_class5_cause_str[] = {
		/* TIN 0 */ "Reserved",
		/* TIN 1 */ "Arithmetic Overflow",
		/* TIN 2 */ "Sticky Arithmetic Overflow",
	};

	static const char *const trap_class6_cause_str[] = {
		/* TIN 0 */ "System Call",
	};

	static const char *const trap_class7_cause_str[] = {
		/* TIN 0 */ "Non-Maskable Interrupt",
	};

	static const char *const *const trap_class_cause_str[] = {
		trap_class0_cause_str, trap_class1_cause_str, trap_class2_cause_str,
		trap_class3_cause_str, trap_class4_cause_str, trap_class5_cause_str,
		trap_class6_cause_str, trap_class7_cause_str,
	};
	static const size_t trap_class_tin_count[] = {
		ARRAY_SIZE(trap_class0_cause_str), ARRAY_SIZE(trap_class1_cause_str),
		ARRAY_SIZE(trap_class2_cause_str), ARRAY_SIZE(trap_class3_cause_str),
		ARRAY_SIZE(trap_class4_cause_str), ARRAY_SIZE(trap_class5_cause_str),
		ARRAY_SIZE(trap_class6_cause_str), ARRAY_SIZE(trap_class7_cause_str),
	};

	if (trap_class >= 8) {
		return "Unknown Trap Class";
	}
	if (tin >= trap_class_tin_count[trap_class]) {
		return "Unknown TIN";
	}

	return trap_class_cause_str[trap_class][tin];
}

void z_tricore_fatal_error(unsigned int reason, const struct arch_esf *lower)
{
#if CONFIG_EXCEPTION_DEBUG
	struct z_tricore_upper_context *upper =
		UINT_TO_POINTER(((lower->pcxi & 0xF0000) << 12) | ((lower->pcxi & 0xFFFF) << 6));

	LOG_ERR("A0:  %08x A1:  %08x A2:  %08x A3:  %08x", 0, 0, lower->a2, lower->a3);
	LOG_ERR("A4:  %08x A5:  %08x A6:  %08x A7:  %08x", lower->a4, lower->a5, lower->a6,
		lower->a7);
	LOG_ERR("A8:  %08x A9:  %08x A10: %08x A11: %08x", 0, 0, upper->a10, upper->a11);
	LOG_ERR("A12: %08x A13: %08x A14: %08x A15: %08x", upper->a12, upper->a13, upper->a14,
		upper->a15);
	LOG_ERR("D0:  %08x D1:  %08x D2:  %08x D3:  %08x", lower->d0, lower->d1, lower->d2,
		lower->d3);
	LOG_ERR("D4:  %08x D5:  %08x D6:  %08x D7:  %08x", lower->d4, lower->d5, lower->d6,
		lower->d7);
	LOG_ERR("D8:  %08x D9:  %08x D10: %08x D11: %08x", upper->d8, upper->d9, upper->d10,
		upper->d11);
	LOG_ERR("D12: %08x D13: %08x D14: %08x D15: %08x", upper->d12, upper->d13, upper->d14,
		upper->d15);
	LOG_ERR("PC:  %08x SP:  %08x PSW: %08x PCXI: %08x", upper->a11, upper->a10, upper->psw,
		upper->pcxi);
#endif

	z_fatal_error(reason, lower);
}

K_KERNEL_STACK_ARRAY_DECLARE(z_interrupt_stacks, CONFIG_MP_MAX_NUM_CPUS, CONFIG_ISR_STACK_SIZE);

static bool bad_stack_pointer(struct z_tricore_upper_context *upper)
{
#if CONFIG_MPU_STACK_GUARD
	if ((upper->psw & BIT(9)) != 0 &&
	    upper->a10 >= POINTER_TO_UINT(z_interrupt_stacks[0]) &&
	    upper->a10 <
		    POINTER_TO_UINT(z_interrupt_stacks[0]) + Z_TRICORE_STACK_GUARD_SIZE) {
		return true;
	} else if ((upper->psw & BIT(11)) != 0 &&
		   (upper->a10 >= _current->stack_info.start &&
		    upper->a10 < _current->stack_info.start + Z_TRICORE_STACK_GUARD_SIZE)) {
		return true;
	}
#endif

#if CONFIG_USERSPACE
	/* Check if the user stack pointer is outside of its allowed stack */
	if ((upper->psw & BIT(11)) == 0) {
		if (upper->a10 < _current->stack_info.start ||
		    upper->a10 >= _current->stack_info.start + _current->stack_info.size -
					  _current->stack_info.delta) {
			return true;
		}
	}
#endif

	return false;
}

void z_tricore_fault(uint8_t trap_class, uint8_t tin)
{
	uint32_t pcxi = cr_read(TRICORE_PCXI);
	struct arch_esf *lower = UINT_TO_POINTER(((pcxi & 0xF0000) << 12) | ((pcxi & 0xFFFF) << 6));
	struct z_tricore_upper_context *upper =
		UINT_TO_POINTER(((lower->pcxi & 0xF0000) << 12) | ((lower->pcxi & 0xFFFF) << 6));

#ifdef CONFIG_USERSPACE
	/*
	 * Perform an assessment whether an MPU fault shall be
	 * treated as recoverable.
	 */
	if (trap_class == TRICORE_TRAP_INTERNAL_PROTECTION_TRAPS &&
	    (tin == TRICORE_TRAP1_MPR || tin == TRICORE_TRAP1_MPW)) {
		for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
			unsigned long start = (unsigned long)exceptions[i].start;
			unsigned long end = (unsigned long)exceptions[i].end;

			if (lower->a11 >= start && lower->a11 < end) {
				lower->a11 = (unsigned long)exceptions[i].fixup;
				return;
			}
		}
	}
#endif /* CONFIG_USERSPACE */

	unsigned int reason = K_ERR_CPU_EXCEPTION;

	if (bad_stack_pointer(upper)) {
#ifdef CONFIG_MPU_STACK_GUARD
		void z_tricore_mpu_stackguard_disable(struct k_thread *thread);
		z_tricore_mpu_stackguard_disable(NULL);
#endif
		reason = K_ERR_STACK_CHK_FAIL;
	}

	LOG_ERR("TriCore Trap: Class %u TIN %u (%s)", trap_class, tin,
		z_tricore_trap_cause_str(trap_class, tin));

	z_tricore_fatal_error(reason, lower);
}

void __weak z_tricore_fault_fcu(void)
{
	while (1) {
		/* FCU faults are not expected to be recoverable, so just loop here. */
	}
}

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_syscall_oops(void *ssf_ptr)
{
	z_tricore_fatal_error(K_ERR_KERNEL_OOPS, ssf_ptr);

	CODE_UNREACHABLE;
}

#endif
