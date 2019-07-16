/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <inttypes.h>
#include <kernel_arch_data.h>
#include <xtensa/specreg.h>

#ifdef XT_SIMULATOR
#include <xtensa/simcall.h>
#endif

/* Need to do this as a macro since regnum must be an immediate value */
#define get_sreg(regnum_p) ({ \
	unsigned int retval; \
	__asm__ volatile( \
	    "rsr %[retval], %[regnum]\n\t" \
	    : [retval] "=r" (retval) \
	    : [regnum] "i" (regnum_p)); \
	retval; \
	})


#if defined(CONFIG_PRINTK) || defined(CONFIG_LOG)
static char *cause_str(unsigned int cause_code)
{
	switch (cause_code) {
	case 0:
		return "illegal instruction";
	case 1:
		return "syscall";
	case 2:
		return "instr fetch error";
	case 3:
		return "load/store error";
	case 4:
		return "level-1 interrupt";
	case 5:
		return "alloca";
	case 6:
		return "divide by zero";
	case 8:
		return "privileged";
	case 9:
		return "load/store alignment";
	case 12:
		return "instr PIF data error";
	case 13:
		return "load/store PIF data error";
	case 14:
		return "instr PIF addr error";
	case 15:
		return "load/store PIF addr error";
	case 16:
		return "instr TLB miss";
	case 17:
		return "instr TLB multi hit";
	case 18:
		return "instr fetch privilege";
	case 20:
		return "inst fetch prohibited";
	case 24:
		return "load/store TLB miss";
	case 25:
		return "load/store TLB multi hit";
	case 26:
		return "load/store privilege";
	case 28:
		return "load prohibited";
	case 29:
		return "store prohibited";
	case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
		return "coprocessor disabled";
	default:
		return "unknown/reserved";
	}
}
#endif

static inline unsigned int get_bits(int offset, int num_bits, unsigned int val)
{
	int mask;

	mask = BIT(num_bits) - 1;
	val = val >> offset;
	return val & mask;
}

static void dump_exc_state(void)
{
#if defined(CONFIG_PRINTK) || defined (CONFIG_LOG)
	unsigned int cause, ps;

	cause = get_sreg(EXCCAUSE);
	ps = get_sreg(PS);

	z_fatal_print("Exception cause %d (%s):", cause, cause_str(cause));
	z_fatal_print("  EPC1     : 0x%08x EXCSAVE1 : 0x%08x EXCVADDR : 0x%08x",
		      get_sreg(EPC_1), get_sreg(EXCSAVE_1), get_sreg(EXCVADDR));

	z_fatal_print("Program state (PS):");
	z_fatal_print("  INTLEVEL : %02d EXCM    : %d UM  : %d RING : %d WOE : %d",
		      get_bits(0, 4, ps), get_bits(4, 1, ps),
		      get_bits(5, 1, ps), get_bits(6, 2, ps),
		      get_bits(18, 1, ps));
#ifndef __XTENSA_CALL0_ABI__
	z_fatal_print("  OWB      : %02d CALLINC : %d",
		      get_bits(8, 4, ps), get_bits(16, 2, ps));
#endif
#endif /* CONFIG_PRINTK || CONFIG_LOG */
}

XTENSA_ERR_NORET void z_xtensa_fatal_error(unsigned int reason,
					   const z_arch_esf_t *esf)
{
	dump_exc_state();

	if (esf) {
		z_fatal_print("Faulting instruction address = 0x%x", esf->pc);
	}

	z_fatal_error(reason, esf);
}

XTENSA_ERR_NORET void FatalErrorHandler(void)
{
	z_xtensa_fatal_error(K_ERR_CPU_EXCEPTION, NULL);
}

XTENSA_ERR_NORET void ReservedInterruptHandler(unsigned int intNo)
{
	z_fatal_print("INTENABLE = 0x%x INTERRUPT = 0x%x (%x)",
		      get_sreg(INTENABLE), (1 << intNo), intNo);
	z_xtensa_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

void exit(int return_code)
{
#ifdef XT_SIMULATOR
	__asm__ (
	    "mov a3, %[code]\n\t"
	    "movi a2, %[call]\n\t"
	    "simcall\n\t"
	    :
	    : [code] "r" (return_code), [call] "i" (SYS_exit)
	    : "a3", "a2");
#else
	z_fatal_print("exit(%d)", return_code);
	k_panic();
#endif
}

#ifdef XT_SIMULATOR
FUNC_NORETURN void z_system_halt(unsigned int reason)
{
	exit(255 - reason);
	CODE_UNREACHABLE;
}
#endif
