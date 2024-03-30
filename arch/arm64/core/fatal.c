/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel fatal error handler for ARM64 Cortex-A
 *
 * This module provides the z_arm64_fatal_error() routine for ARM64 Cortex-A
 * CPUs and z_arm64_do_kernel_oops() routine to manage software-generated fatal
 * exceptions
 */

#include <zephyr/drivers/pm_cpu_ops.h>
#include <zephyr/arch/common/exc_handle.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>
#include <kernel_arch_func.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
K_KERNEL_PINNED_STACK_ARRAY_DEFINE(z_arm64_safe_exception_stacks,
				   CONFIG_MP_MAX_NUM_CPUS,
				   CONFIG_ARM64_SAFE_EXCEPTION_STACK_SIZE);

void z_arm64_safe_exception_stack_init(void)
{
	int cpu_id;
	char *safe_exc_sp;

	cpu_id = arch_curr_cpu()->id;
	safe_exc_sp = Z_KERNEL_STACK_BUFFER(z_arm64_safe_exception_stacks[cpu_id]) +
		      CONFIG_ARM64_SAFE_EXCEPTION_STACK_SIZE;
	arch_curr_cpu()->arch.safe_exception_stack = (uint64_t)safe_exc_sp;
	write_sp_el0((uint64_t)safe_exc_sp);

	arch_curr_cpu()->arch.current_stack_limit = 0UL;
	arch_curr_cpu()->arch.corrupted_sp = 0UL;
}
#endif

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_arm64_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_arm64_user_string_nlen),
};
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_EXCEPTION_DEBUG
static void dump_esr(uint64_t esr, bool *dump_far)
{
	const char *err;

	switch (GET_ESR_EC(esr)) {
	case 0b000000: /* 0x00 */
		err = "Unknown reason";
		break;
	case 0b000001: /* 0x01 */
		err = "Trapped WFI or WFE instruction execution";
		break;
	case 0b000011: /* 0x03 */
		err = "Trapped MCR or MRC access with (coproc==0b1111) that "
		      "is not reported using EC 0b000000";
		break;
	case 0b000100: /* 0x04 */
		err = "Trapped MCRR or MRRC access with (coproc==0b1111) "
		      "that is not reported using EC 0b000000";
		break;
	case 0b000101: /* 0x05 */
		err = "Trapped MCR or MRC access with (coproc==0b1110)";
		break;
	case 0b000110: /* 0x06 */
		err = "Trapped LDC or STC access";
		break;
	case 0b000111: /* 0x07 */
		err = "Trapped access to SVE, Advanced SIMD, or "
		      "floating-point functionality";
		break;
	case 0b001100: /* 0x0c */
		err = "Trapped MRRC access with (coproc==0b1110)";
		break;
	case 0b001101: /* 0x0d */
		err = "Branch Target Exception";
		break;
	case 0b001110: /* 0x0e */
		err = "Illegal Execution state";
		break;
	case 0b010001: /* 0x11 */
		err = "SVC instruction execution in AArch32 state";
		break;
	case 0b011000: /* 0x18 */
		err = "Trapped MSR, MRS or System instruction execution in "
		      "AArch64 state, that is not reported using EC "
		      "0b000000, 0b000001 or 0b000111";
		break;
	case 0b011001: /* 0x19 */
		err = "Trapped access to SVE functionality";
		break;
	case 0b100000: /* 0x20 */
		*dump_far = true;
		err = "Instruction Abort from a lower Exception level, that "
		      "might be using AArch32 or AArch64";
		break;
	case 0b100001: /* 0x21 */
		*dump_far = true;
		err = "Instruction Abort taken without a change in Exception "
		      "level.";
		break;
	case 0b100010: /* 0x22 */
		*dump_far = true;
		err = "PC alignment fault exception.";
		break;
	case 0b100100: /* 0x24 */
		*dump_far = true;
		err = "Data Abort from a lower Exception level, that might "
		      "be using AArch32 or AArch64";
		break;
	case 0b100101: /* 0x25 */
		*dump_far = true;
		err = "Data Abort taken without a change in Exception level";
		break;
	case 0b100110: /* 0x26 */
		err = "SP alignment fault exception";
		break;
	case 0b101000: /* 0x28 */
		err = "Trapped floating-point exception taken from AArch32 "
		      "state";
		break;
	case 0b101100: /* 0x2c */
		err = "Trapped floating-point exception taken from AArch64 "
		      "state.";
		break;
	case 0b101111: /* 0x2f */
		err = "SError interrupt";
		break;
	case 0b110000: /* 0x30 */
		err = "Breakpoint exception from a lower Exception level, "
		      "that might be using AArch32 or AArch64";
		break;
	case 0b110001: /* 0x31 */
		err = "Breakpoint exception taken without a change in "
		      "Exception level";
		break;
	case 0b110010: /* 0x32 */
		err = "Software Step exception from a lower Exception level, "
		      "that might be using AArch32 or AArch64";
		break;
	case 0b110011: /* 0x33 */
		err = "Software Step exception taken without a change in "
		      "Exception level";
		break;
	case 0b110100: /* 0x34 */
		*dump_far = true;
		err = "Watchpoint exception from a lower Exception level, "
		      "that might be using AArch32 or AArch64";
		break;
	case 0b110101: /* 0x35 */
		*dump_far = true;
		err = "Watchpoint exception taken without a change in "
		      "Exception level.";
		break;
	case 0b111000: /* 0x38 */
		err = "BKPT instruction execution in AArch32 state";
		break;
	case 0b111100: /* 0x3c */
		err = "BRK instruction execution in AArch64 state.";
		break;
	default:
		err = "Unknown";
	}

	LOG_ERR("ESR_ELn: 0x%016llx", esr);
	LOG_ERR("  EC:  0x%llx (%s)", GET_ESR_EC(esr), err);
	LOG_ERR("  IL:  0x%llx", GET_ESR_IL(esr));
	LOG_ERR("  ISS: 0x%llx", GET_ESR_ISS(esr));
}

static void esf_dump(const z_arch_esf_t *esf)
{
	LOG_ERR("x0:  0x%016llx  x1:  0x%016llx", esf->x0, esf->x1);
	LOG_ERR("x2:  0x%016llx  x3:  0x%016llx", esf->x2, esf->x3);
	LOG_ERR("x4:  0x%016llx  x5:  0x%016llx", esf->x4, esf->x5);
	LOG_ERR("x6:  0x%016llx  x7:  0x%016llx", esf->x6, esf->x7);
	LOG_ERR("x8:  0x%016llx  x9:  0x%016llx", esf->x8, esf->x9);
	LOG_ERR("x10: 0x%016llx  x11: 0x%016llx", esf->x10, esf->x11);
	LOG_ERR("x12: 0x%016llx  x13: 0x%016llx", esf->x12, esf->x13);
	LOG_ERR("x14: 0x%016llx  x15: 0x%016llx", esf->x14, esf->x15);
	LOG_ERR("x16: 0x%016llx  x17: 0x%016llx", esf->x16, esf->x17);
	LOG_ERR("x18: 0x%016llx  lr:  0x%016llx", esf->x18, esf->lr);
}

#ifdef CONFIG_ARM64_ENABLE_FRAME_POINTER
static void esf_unwind(const z_arch_esf_t *esf)
{
	/*
	 * For GCC:
	 *
	 *  ^  +-----------------+
	 *  |  |                 |
	 *  |  |                 |
	 *  |  |                 |
	 *  |  |                 |
	 *  |  | function stack  |
	 *  |  |                 |
	 *  |  |                 |
	 *  |  |                 |
	 *  |  |                 |
	 *  |  +-----------------+
	 *  |  |       LR        |
	 *  |  +-----------------+
	 *  |  |   previous FP   | <---+ FP
	 *  +  +-----------------+
	 */

	uint64_t *fp = (uint64_t *) esf->fp;
	unsigned int count = 0;
	uint64_t lr;

	LOG_ERR("");
	while (fp != NULL) {
		lr = fp[1];
		LOG_ERR("backtrace %2d: fp: 0x%016llx lr: 0x%016llx",
			 count++, (uint64_t) fp, lr);
		fp = (uint64_t *) fp[0];
	}
	LOG_ERR("");
}
#endif

#endif /* CONFIG_EXCEPTION_DEBUG */

#ifdef CONFIG_ARM64_STACK_PROTECTION
static bool z_arm64_stack_corruption_check(z_arch_esf_t *esf, uint64_t esr, uint64_t far)
{
	uint64_t sp, sp_limit, guard_start;
	/* 0x25 means data abort from current EL */
	if (GET_ESR_EC(esr) == 0x25) {
		sp_limit = arch_curr_cpu()->arch.current_stack_limit;
		guard_start = sp_limit - Z_ARM64_STACK_GUARD_SIZE;
		sp = arch_curr_cpu()->arch.corrupted_sp;
		if ((sp != 0 && sp <= sp_limit) || (guard_start <= far && far <= sp_limit)) {
#ifdef CONFIG_FPU_SHARING
			/*
			 * We are in exception stack, and now we are sure the stack does overflow,
			 * so flush the fpu context to its owner, and then set no fpu trap to avoid
			 * a new nested exception triggered by FPU accessing (var_args).
			 */
			arch_flush_local_fpu();
			write_cpacr_el1(read_cpacr_el1() | CPACR_EL1_FPEN_NOTRAP);
#endif
			arch_curr_cpu()->arch.corrupted_sp = 0UL;
			LOG_ERR("STACK OVERFLOW FROM KERNEL, SP: 0x%llx OR FAR: 0x%llx INVALID,"
				" SP LIMIT: 0x%llx", sp, far, sp_limit);
			return true;
		}
	}
#ifdef CONFIG_USERSPACE
	else if ((_current->base.user_options & K_USER) != 0 && GET_ESR_EC(esr) == 0x24) {
		sp_limit = (uint64_t)_current->stack_info.start;
		guard_start = sp_limit - Z_ARM64_STACK_GUARD_SIZE;
		sp = esf->sp;
		if (sp <= sp_limit || (guard_start <= far && far <= sp_limit)) {
			LOG_ERR("STACK OVERFLOW FROM USERSPACE, SP: 0x%llx OR FAR: 0x%llx INVALID,"
				" SP LIMIT: 0x%llx", sp, far, sp_limit);
			return true;
		}
	}
#endif
	return false;
}
#endif

static bool is_recoverable(z_arch_esf_t *esf, uint64_t esr, uint64_t far,
			   uint64_t elr)
{
	if (!esf)
		return false;

#ifdef CONFIG_USERSPACE
	for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
		/* Mask out instruction mode */
		uint64_t start = (uint64_t)exceptions[i].start;
		uint64_t end = (uint64_t)exceptions[i].end;

		if (esf->elr >= start && esf->elr < end) {
			esf->elr = (uint64_t)(exceptions[i].fixup);
			return true;
		}
	}
#endif

	return false;
}

void z_arm64_fatal_error(unsigned int reason, z_arch_esf_t *esf)
{
	uint64_t esr = 0;
	uint64_t elr = 0;
	uint64_t far = 0;
	uint64_t el;

	if (reason != K_ERR_SPURIOUS_IRQ) {
		el = read_currentel();

		switch (GET_EL(el)) {
		case MODE_EL1:
			esr = read_esr_el1();
			far = read_far_el1();
			elr = read_elr_el1();
			break;
		case MODE_EL3:
			esr = read_esr_el3();
			far = read_far_el3();
			elr = read_elr_el3();
			break;
		}

#ifdef CONFIG_ARM64_STACK_PROTECTION
		if (z_arm64_stack_corruption_check(esf, esr, far)) {
			reason = K_ERR_STACK_CHK_FAIL;
		}
#endif

		if (GET_EL(el) != MODE_EL0) {
#ifdef CONFIG_EXCEPTION_DEBUG
			bool dump_far = false;

			LOG_ERR("ELR_ELn: 0x%016llx", elr);

			dump_esr(esr, &dump_far);

			if (dump_far)
				LOG_ERR("FAR_ELn: 0x%016llx", far);

			LOG_ERR("TPIDRRO: 0x%016llx", read_tpidrro_el0());
#endif /* CONFIG_EXCEPTION_DEBUG */

			if (is_recoverable(esf, esr, far, elr) &&
			    reason != K_ERR_STACK_CHK_FAIL) {
				return;
			}
		}
	}

#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		esf_dump(esf);
	}

#ifdef CONFIG_ARM64_ENABLE_FRAME_POINTER
	esf_unwind(esf);
#endif /* CONFIG_ARM64_ENABLE_FRAME_POINTER */
#endif /* CONFIG_EXCEPTION_DEBUG */

	z_fatal_error(reason, esf);

	CODE_UNREACHABLE;
}

/**
 * @brief Handle a software-generated fatal exception
 * (e.g. kernel oops, panic, etc.).
 *
 * @param esf exception frame
 */
void z_arm64_do_kernel_oops(z_arch_esf_t *esf)
{
	/* x8 holds the exception reason */
	unsigned int reason = esf->x8;

#if defined(CONFIG_USERSPACE)
	/*
	 * User mode is only allowed to induce oopses and stack check
	 * failures via software-triggered system fatal exceptions.
	 */
	if (((_current->base.user_options & K_USER) != 0) &&
		reason != K_ERR_STACK_CHK_FAIL) {
		reason = K_ERR_KERNEL_OOPS;
	}
#endif

	z_arm64_fatal_error(reason, esf);
}

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_syscall_oops(void *ssf_ptr)
{
	z_arm64_fatal_error(K_ERR_KERNEL_OOPS, ssf_ptr);
	CODE_UNREACHABLE;
}
#endif

#if defined(CONFIG_PM_CPU_OPS_PSCI)
FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);

	(void)arch_irq_lock();

#ifdef CONFIG_POWEROFF
	sys_poweroff();
#endif /* CONFIG_POWEROFF */

	for (;;) {
		/* Spin endlessly as fallback */
	}
}
#endif
