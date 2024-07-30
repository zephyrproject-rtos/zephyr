/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/kernel_structs.h>
#include <inttypes.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

FUNC_NORETURN void z_nios2_fatal_error(unsigned int reason,
				       const struct arch_esf *esf)
{
#if CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		/* Subtract 4 from EA since we added 4 earlier so that the
		 * faulting instruction isn't retried.
		 *
		 * TODO: Only caller-saved registers get saved upon exception
		 * entry.  We may want to introduce a config option to save and
		 * dump all registers, at the expense of some stack space.
		 */
		LOG_ERR("Faulting instruction: 0x%08x", esf->instr - 4);
		LOG_ERR("  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x  r4: 0x%08x",
			esf->r1, esf->r2, esf->r3, esf->r4);
		LOG_ERR("  r5: 0x%08x  r6: 0x%08x  r7: 0x%08x  r8: 0x%08x",
			esf->r5, esf->r6, esf->r7, esf->r8);
		LOG_ERR("  r9: 0x%08x r10: 0x%08x r11: 0x%08x r12: 0x%08x",
			esf->r9, esf->r10, esf->r11, esf->r12);
		LOG_ERR(" r13: 0x%08x r14: 0x%08x r15: 0x%08x  ra: 0x%08x",
			esf->r13, esf->r14, esf->r15, esf->ra);
		LOG_ERR("estatus: %08x", esf->estatus);
	}
#endif /* CONFIG_EXCEPTION_DEBUG */

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

#if defined(CONFIG_EXTRA_EXCEPTION_INFO) && \
	(defined(CONFIG_PRINTK) || defined(CONFIG_LOG)) \
	&& defined(ALT_CPU_HAS_EXTRA_EXCEPTION_INFO)
static char *cause_str(uint32_t cause_code)
{
	switch (cause_code) {
	case 0:
		return "reset";
	case 1:
		return "processor-only reset request";
	case 2:
		return "interrupt";
	case 3:
		return "trap";
	case 4:
		return "unimplemented instruction";
	case 5:
		return "illegal instruction";
	case 6:
		return "misaligned data address";
	case 7:
		return "misaligned destination address";
	case 8:
		return "division error";
	case 9:
		return "supervisor-only instruction address";
	case 10:
		return "supervisor-only instruction";
	case 11:
		return "supervisor-only data address";
	case 12:
		return "TLB miss";
	case 13:
		return "TLB permission violation (execute)";
	case 14:
		return "TLB permission violation (read)";
	case 15:
		return "TLB permission violation (write)";
	case 16:
		return "MPU region violation (instruction)";
	case 17:
		return "MPU region violation (data)";
	case 18:
		return "ECC TLB error";
	case 19:
		return "ECC fetch error (instruction)";
	case 20:
		return "ECC register file error";
	case 21:
		return "ECC data error";
	case 22:
		return "ECC data cache writeback error";
	case 23:
		return "bus instruction fetch error";
	case 24:
		return "bus data region violation";
	default:
		return "unknown";
	}
}
#endif

FUNC_NORETURN void _Fault(const struct arch_esf *esf)
{
#if defined(CONFIG_PRINTK) || defined(CONFIG_LOG)
	/* Unfortunately, completely unavailable on Nios II/e cores */
#ifdef ALT_CPU_HAS_EXTRA_EXCEPTION_INFO
	uint32_t exc_reg, badaddr_reg, eccftl;
	enum nios2_exception_cause cause;

	exc_reg = z_nios2_creg_read(NIOS2_CR_EXCEPTION);

	/* Bit 31 indicates potentially fatal ECC error */
	eccftl = (exc_reg & NIOS2_EXCEPTION_REG_ECCFTL_MASK) != 0U;

	/* Bits 2-6 contain the cause code */
	cause = (exc_reg & NIOS2_EXCEPTION_REG_CAUSE_MASK)
		 >> NIOS2_EXCEPTION_REG_CAUSE_OFST;

	LOG_ERR("Exception cause: %d ECCFTL: 0x%x", cause, eccftl);
#if CONFIG_EXTRA_EXCEPTION_INFO
	LOG_ERR("reason: %s", cause_str(cause));
#endif
	if (BIT(cause) & NIOS2_BADADDR_CAUSE_MASK) {
		badaddr_reg = z_nios2_creg_read(NIOS2_CR_BADADDR);
		LOG_ERR("Badaddr: 0x%x", badaddr_reg);
	}
#endif /* ALT_CPU_HAS_EXTRA_EXCEPTION_INFO */
#endif /* CONFIG_PRINTK || CONFIG_LOG */

	z_nios2_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}

#ifdef ALT_CPU_HAS_DEBUG_STUB
FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);

	z_nios2_break();
	CODE_UNREACHABLE;
}
#endif
