/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fault handlers for ARMv5 (ARM9 / XScale)
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/exception.h>
#include <kernel_internal.h>
#include <zephyr/arch/arm/arm9/lib_helpers.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/**
 * @brief Map an ARMv5 fault status code onto a Zephyr fatal error reason
 */
static uint32_t fault_reason(uint32_t fsr, uint32_t addr, const char *kind)
{
	uint32_t status = fsr & FSR_STATUS_MASK;
	uint32_t reason = K_ERR_CPU_EXCEPTION;
	const char *desc;

	switch (status) {
	case FSR_ALIGNMENT:
		desc = "Alignment fault";
		reason = K_ERR_ARM_ALIGNMENT_FAULT;
		break;
	case FSR_TRANSLATION_SECTION:
		desc = "Section translation fault";
		reason = K_ERR_ARM_TRANSLATION_FAULT;
		break;
	case FSR_TRANSLATION_PAGE:
		desc = "Page translation fault";
		reason = K_ERR_ARM_TRANSLATION_FAULT_2ND_LEVEL;
		break;
	case FSR_DOMAIN_SECTION:
		desc = "Section domain fault";
		reason = K_ERR_ARM_DOMAIN_FAULT_1ST_LEVEL;
		break;
	case FSR_DOMAIN_PAGE:
		desc = "Page domain fault";
		reason = K_ERR_ARM_DOMAIN_FAULT_2ND_LEVEL;
		break;
	case FSR_PERMISSION_SECTION:
		desc = "Section permission fault";
		reason = K_ERR_ARM_PERMISSION_FAULT;
		break;
	case FSR_PERMISSION_PAGE:
		desc = "Page permission fault";
		reason = K_ERR_ARM_PERMISSION_FAULT_2ND_LEVEL;
		break;
	case FSR_EXT_ABORT_LINEFETCH_S:
	case FSR_EXT_ABORT_LINEFETCH_P:
	case FSR_EXT_ABORT_NONLINEFETCH_S:
	case FSR_EXT_ABORT_NONLINEFETCH_P:
		desc = "External abort";
		reason = K_ERR_ARM_SYNC_EXTERNAL_ABORT;
		break;
	case FSR_EXT_ABORT_TT_L1:
	case FSR_EXT_ABORT_TT_L2:
		desc = "External abort on translation table walk";
		reason = K_ERR_ARM_SYNC_EXTERNAL_ABORT;
		break;
	default:
		desc = "Unknown fault";
		break;
	}

	LOG_ERR("%s: %s (FSR 0x%08x, domain %u) at 0x%08x", kind, desc, fsr,
		(fsr >> FSR_DOMAIN_SHIFT) & FSR_DOMAIN_MASK, addr);

	return IS_ENABLED(CONFIG_SIMPLIFIED_EXCEPTION_CODES) ? K_ERR_CPU_EXCEPTION : reason;
}

/**
 * @brief Undefined instruction fault handler
 */
bool z_arm_fault_undef_instruction(struct arch_esf *esf)
{
	uint32_t reason = IS_ENABLED(CONFIG_SIMPLIFIED_EXCEPTION_CODES)
				  ? K_ERR_CPU_EXCEPTION
				  : K_ERR_ARM_UNDEFINED_INSTRUCTION;

	LOG_ERR("Undefined instruction at 0x%08x", esf->basic.pc);

	z_arm_fatal_error(reason, esf);

	return true;
}

/**
 * @brief Prefetch abort fault handler
 *
 * ARMv5 has no IFAR, so the faulting address is the recorded return address.
 */
bool z_arm_fault_prefetch(struct arch_esf *esf)
{
	uint32_t reason = fault_reason(__get_IFSR(), esf->basic.pc, "Prefetch abort");

	z_arm_fatal_error(reason, esf);

	return true;
}

/**
 * @brief Data abort fault handler
 */
bool z_arm_fault_data(struct arch_esf *esf)
{
	uint32_t reason = fault_reason(__get_DFSR(), __get_DFAR(), "Data abort");

	z_arm_fatal_error(reason, esf);

	return true;
}
