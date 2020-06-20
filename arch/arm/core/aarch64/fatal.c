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
 * CPUs
 */

#include <kernel.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(os);

static void print_EC_cause(uint64_t esr)
{
	uint32_t EC = (uint32_t)esr >> 26;

	switch (EC) {
	case 0b000000:
		LOG_ERR("Unknown reason");
		break;
	case 0b000001:
		LOG_ERR("Trapped WFI or WFE instruction execution");
		break;
	case 0b000011:
		LOG_ERR("Trapped MCR or MRC access with (coproc==0b1111) that "
			"is not reported using EC 0b000000");
		break;
	case 0b000100:
		LOG_ERR("Trapped MCRR or MRRC access with (coproc==0b1111) "
			"that is not reported using EC 0b000000");
		break;
	case 0b000101:
		LOG_ERR("Trapped MCR or MRC access with (coproc==0b1110)");
		break;
	case 0b000110:
		LOG_ERR("Trapped LDC or STC access");
		break;
	case 0b000111:
		LOG_ERR("Trapped access to SVE, Advanced SIMD, or "
			"floating-point functionality");
		break;
	case 0b001100:
		LOG_ERR("Trapped MRRC access with (coproc==0b1110)");
		break;
	case 0b001101:
		LOG_ERR("Branch Target Exception");
		break;
	case 0b001110:
		LOG_ERR("Illegal Execution state");
		break;
	case 0b010001:
		LOG_ERR("SVC instruction execution in AArch32 state");
		break;
	case 0b011000:
		LOG_ERR("Trapped MSR, MRS or System instruction execution in "
			"AArch64 state, that is not reported using EC "
			"0b000000, 0b000001 or 0b000111");
		break;
	case 0b011001:
		LOG_ERR("Trapped access to SVE functionality");
		break;
	case 0b100000:
		LOG_ERR("Instruction Abort from a lower Exception level, that "
			"might be using AArch32 or AArch64");
		break;
	case 0b100001:
		LOG_ERR("Instruction Abort taken without a change in Exception "
			"level.");
		break;
	case 0b100010:
		LOG_ERR("PC alignment fault exception.");
		break;
	case 0b100100:
		LOG_ERR("Data Abort from a lower Exception level, that might "
			"be using AArch32 or AArch64");
		break;
	case 0b100101:
		LOG_ERR("Data Abort taken without a change in Exception level");
		break;
	case 0b100110:
		LOG_ERR("SP alignment fault exception");
		break;
	case 0b101000:
		LOG_ERR("Trapped floating-point exception taken from AArch32 "
			"state");
		break;
	case 0b101100:
		LOG_ERR("Trapped floating-point exception taken from AArch64 "
			"state.");
		break;
	case 0b101111:
		LOG_ERR("SError interrupt");
		break;
	case 0b110000:
		LOG_ERR("Breakpoint exception from a lower Exception level, "
			"that might be using AArch32 or AArch64");
		break;
	case 0b110001:
		LOG_ERR("Breakpoint exception taken without a change in "
			"Exception level");
		break;
	case 0b110010:
		LOG_ERR("Software Step exception from a lower Exception level, "
			"that might be using AArch32 or AArch64");
		break;
	case 0b110011:
		LOG_ERR("Software Step exception taken without a change in "
			"Exception level");
		break;
	case 0b110100:
		LOG_ERR("Watchpoint exception from a lower Exception level, "
			"that might be using AArch32 or AArch64");
		break;
	case 0b110101:
		LOG_ERR("Watchpoint exception taken without a change in "
			"Exception level.");
		break;
	case 0b111000:
		LOG_ERR("BKPT instruction execution in AArch32 state");
		break;
	case 0b111100:
		LOG_ERR("BRK instruction execution in AArch64 state.");
		break;
	}
}

static void esf_dump(const z_arch_esf_t *esf)
{
	LOG_ERR("x1:  %-8llx  x0:  %llx",
		esf->basic.regs[18], esf->basic.regs[19]);
	LOG_ERR("x2:  %-8llx  x3:  %llx",
		esf->basic.regs[16], esf->basic.regs[17]);
	LOG_ERR("x4:  %-8llx  x5:  %llx",
		esf->basic.regs[14], esf->basic.regs[15]);
	LOG_ERR("x6:  %-8llx  x7:  %llx",
		esf->basic.regs[12], esf->basic.regs[13]);
	LOG_ERR("x8:  %-8llx  x9:  %llx",
		esf->basic.regs[10], esf->basic.regs[11]);
	LOG_ERR("x10: %-8llx  x11: %llx",
		esf->basic.regs[8], esf->basic.regs[9]);
	LOG_ERR("x12: %-8llx  x13: %llx",
		esf->basic.regs[6], esf->basic.regs[7]);
	LOG_ERR("x14: %-8llx  x15: %llx",
		esf->basic.regs[4], esf->basic.regs[5]);
	LOG_ERR("x16: %-8llx  x17: %llx",
		esf->basic.regs[2], esf->basic.regs[3]);
	LOG_ERR("x18: %-8llx  x30: %llx",
		esf->basic.regs[0], esf->basic.regs[1]);
}

void z_arm64_fatal_error(unsigned int reason, const z_arch_esf_t *esf)
{
	uint64_t el, esr, elr, far;

	if (reason != K_ERR_SPURIOUS_IRQ) {
		__asm__ volatile("mrs %0, CurrentEL" : "=r" (el));

		switch (GET_EL(el)) {
		case MODE_EL1:
			__asm__ volatile("mrs %0, esr_el1" : "=r" (esr));
			__asm__ volatile("mrs %0, far_el1" : "=r" (far));
			__asm__ volatile("mrs %0, elr_el1" : "=r" (elr));
			break;
		case MODE_EL2:
			__asm__ volatile("mrs %0, esr_el2" : "=r" (esr));
			__asm__ volatile("mrs %0, far_el2" : "=r" (far));
			__asm__ volatile("mrs %0, elr_el2" : "=r" (elr));
			break;
		case MODE_EL3:
			__asm__ volatile("mrs %0, esr_el3" : "=r" (esr));
			__asm__ volatile("mrs %0, far_el3" : "=r" (far));
			__asm__ volatile("mrs %0, elr_el3" : "=r" (elr));
			break;
		default:
			/* Just to keep the compiler happy */
			esr = elr = far = 0;
			break;
		}

		if (GET_EL(el) != MODE_EL0) {
			LOG_ERR("ESR_ELn: %llx", esr);
			LOG_ERR("FAR_ELn: %llx", far);
			LOG_ERR("ELR_ELn: %llx", elr);

			print_EC_cause(esr);
		}

	}

	if (esf != NULL) {
		esf_dump(esf);
	}
	z_fatal_error(reason, esf);

	CODE_UNREACHABLE;
}
