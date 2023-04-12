/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/kernel_structs.h>
#include <inttypes.h>
#include <xtensa/config/specreg.h>
#include <xtensa-asm2-context.h>
#if defined(CONFIG_XTENSA_ENABLE_BACKTRACE)
#if XCHAL_HAVE_WINDOWED
#include <xtensa_backtrace.h>
#endif
#endif
#include <zephyr/debug/coredump.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

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


char *z_xtensa_exccause(unsigned int cause_code)
{
#if defined(CONFIG_PRINTK) || defined(CONFIG_LOG)
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
	case 63:
		/* i.e. z_except_reason */
		return "zephyr exception";
	default:
		return "unknown/reserved";
	}
#else
	ARG_UNUSED(cause_code);
	return "na";
#endif
}

void z_xtensa_fatal_error(unsigned int reason, const z_arch_esf_t *esf)
{
	if (esf) {
		/* Don't want to get elbowed by xtensa_switch
		 * in between printing registers and dumping them;
		 * corrupts backtrace
		 */
		unsigned int key = arch_irq_lock();

		z_xtensa_dump_stack(esf);

		coredump(reason, esf, IS_ENABLED(CONFIG_MULTITHREADING) ? k_current_get() : NULL);

#if defined(CONFIG_XTENSA_ENABLE_BACKTRACE)
#if XCHAL_HAVE_WINDOWED
		z_xtensa_backtrace_print(100, (int *)esf);
#endif
#endif

		arch_irq_unlock(key);
	}

	z_fatal_error(reason, esf);
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
	LOG_ERR("exit(%d)", return_code);
	k_panic();
#endif
	CODE_UNREACHABLE;
}

#ifdef XT_SIMULATOR
FUNC_NORETURN void z_system_halt(unsigned int reason)
{
	exit(255 - reason);
	CODE_UNREACHABLE;
}
#endif
