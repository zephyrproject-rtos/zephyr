/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM CORTEX-M System Control Block interface
 *
 * Provide an interface to the System Control Block found on ARM Cortex-M
 * processors.
 *
 * The API does not account for all possible usages of the SCB, only the
 * functionalities needed by the kernel. It does not contain NVIC
 * functionalities either: these can be found in nvic.h. MPU functionalities
 * are not implemented.
 *
 * The same effect can be achieved by directly writing in the registers of the
 * SCB, with the layout available from scs.h, using the __scs.scb data
 * structure (or hardcoded values), but the APIs found here are less
 * error-prone, especially for registers with multiple instances to account
 * for 16 exceptions.
 *
 * If access to a missing functionality is needed, directly writing to the
 * registers is the way to implement it.
 */

#ifndef _SCB__H_
#define _SCB__H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* needed by k_cpu_atomic_idle() written in asm */
#define _SCB_SCR 0xE000ED10

#define _SCB_SCR_SEVONPEND (1 << 4)
#define _SCB_SCR_SLEEPDEEP (1 << 2)
#define _SCB_SCR_SLEEPONEXIT (1 << 1)

#else

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <arch/arm/cortex_m/scs.h>
#include <misc/util.h>
#include <stdint.h>

/**
 *
 * @brief Pend the NMI exception
 *
 * Pend the NMI exception: it should fire immediately.
 *
 * @return N/A
 */

static inline void _ScbNmiPend(void)
{
	__scs.scb.icsr.bit.nmipendset = 1;
}

/**
 *
 * @brief Set the PendSV exception
 *
 * Set the PendSV exception: it will be handled when the last nested exception
 * returns, or immediately if running in thread mode.
 *
 * @return N/A
 */

static inline void _ScbPendsvSet(void)
{
	__scs.scb.icsr.bit.pendsvset = 1;
}

/**
 *
 * @brief Find out if running in thread mode
 *
 * This routine determines if the current mode is thread mode.
 *
 * @return 1 if in thread mode, 0 otherwise
 */

static inline int _ScbIsInThreadMode(void)
{
	/* 0 == thread mode */
	return !__scs.scb.icsr.bit.vectactive;
}

/**
 *
 * @brief Obtain the currently executing vector
 *
 * If currently handling an exception/interrupt, return the executing vector
 * number. If not, return 0.
 *
 * @return the currently executing vector number, 0 if in thread mode.
 */

static inline uint32_t _ScbActiveVectorGet(void)
{
	return __scs.scb.icsr.bit.vectactive;
}

/**
 *
 * @brief Set priority of an exception
 *
 * Only works with exceptions; i.e. do not use this for interrupts, which
 * are exceptions 16+.
 *
 * Note that the processor might not implement all 8 bits, in which case the
 * lower N bits are ignored.
 *
 * ARMv6-M: Exceptions 1 to 3 priorities are fixed (-3, -2, -1) and 4 to 9 are
 * reserved exceptions.
 * ARMv7-M: Exceptions 1 to 3 priorities are fixed (-3, -2, -1).
 *
 * @param exc  exception number, 10 to 15 on ARMv6-M and 4 to 15 on ARMv7-M
 * @param pri  priority, 0 to 255
 * @return N/A
 */

static inline void _ScbExcPrioSet(uint8_t exc, uint8_t pri)
{
#if defined(CONFIG_ARMV6_M)
	volatile uint32_t * const shpr = &__scs.scb.shpr[_PRIO_SHP_IDX(exc)];
	__ASSERT((exc > 10) && (exc < 16), "");
	*shpr = ((*shpr & ~((uint32_t)0xff << _PRIO_BIT_SHIFT(exc))) |
		 ((uint32_t)pri << _PRIO_BIT_SHIFT(exc)));
#elif defined(CONFIG_ARMV7_M)
	/* For priority exception handler 4-15 */
	__ASSERT((exc > 3) && (exc < 16), "");
	__scs.scb.shpr[exc - 4] = pri;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
}

#if defined(CONFIG_ARMV6_M)
#elif defined(CONFIG_ARMV7_M)
/**
 *
 * @brief Find out if the currently executing exception is nested
 *
 * This routine determines if the currently executing exception is nested.
 *
 * @return 1 if nested, 0 otherwise
 */

static inline int _ScbIsNestedExc(void)
{
	/* !bit == preempted exceptions */
	return !__scs.scb.icsr.bit.rettobase;
}

/**
 *
 * @brief Enable faulting on division by zero
 *
 * This routine enables the divide by zero fault.
 * By default, the CPU ignores the error.
 *
 * @return N/A
 */

static inline void _ScbDivByZeroFaultEnable(void)
{
	__scs.scb.ccr.bit.div_0_trp = 1;
}

/**
 *
 * @brief Enable usage fault exceptions
 *
 * This routine enables usage faults.
 * By default, the CPU does not raise usage fault exceptions.
 *
 * @return N/A
 */

static inline void _ScbUsageFaultEnable(void)
{
	__scs.scb.shcsr.bit.usgfaultena = 1;
}

/**
 *
 * @brief Enable bus fault exceptions
 *
 * This routine enables bus faults.
 * By default, the CPU does not raise bus fault exceptions.
 *
 * @return N/A
 */

static inline void _ScbBusFaultEnable(void)
{
	__scs.scb.shcsr.bit.busfaultena = 1;
}

/**
 *
 * @brief Enable MPU faults exceptions
 *
 * This routine enables the MPU faults.
 * By default, the CPU does not raise MPU fault exceptions.
 *
 * @return N/A
 */

static inline void _ScbMemFaultEnable(void)
{
	__scs.scb.shcsr.bit.memfaultena = 1;
}

/**
 *
 * @brief Find out if a hard fault is caused by a bus error on vector read
 *
 * This routine determines if a hard fault is caused by a bus error during
 * a vector table read operation.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbHardFaultIsBusErrOnVectorRead(void)
{
	return __scs.scb.hfsr.bit.vecttbl;
}

/**
 *
 * @brief Find out if a fault was escalated to hard fault
 *
 * Happens if a fault cannot be triggered because of priority or because it was
 * disabled.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbHardFaultIsForced(void)
{
	return __scs.scb.hfsr.bit.forced;
}

/**
 *
 * @brief Clear all hard faults (HFSR register)
 *
 * HFSR register is a 'write-one-to-clear' (W1C) register.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbHardFaultAllFaultsReset(void)
{
	return __scs.scb.hfsr.val = 0xffff;
}

/**
 *
 * @brief Find out if a hard fault is an MPU fault
 *
 * This routine determines if a hard fault is an MPU fault.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbIsMemFault(void)
{
	return !!__scs.scb.cfsr.byte.mmfsr.val;
}

/**
 *
 * @brief Find out if the MMFAR register contains a valid value
 *
 * The MMFAR register contains the faulting address on an MPU fault.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbMemFaultIsMmfarValid(void)
{
	return !!__scs.scb.cfsr.byte.mmfsr.bit.mmarvalid;
}

/**
 *
 * @brief Invalid the value in MMFAR
 *
 * This routine invalidates the MMFAR value. This should be done after
 * processing an MPU fault.
 *
 * @return N/A
 */

static inline void _ScbMemFaultMmfarReset(void)
{
	__scs.scb.cfsr.byte.mmfsr.bit.mmarvalid = 0;
}

/**
 *
 * @brief Clear all MPU faults (MMFSR register)
 *
 * CFSR/MMFSR register is a 'write-one-to-clear' (W1C) register.
 *
 * @return 1 if so, 0 otherwise
 */

static inline void _ScbMemFaultAllFaultsReset(void)
{
	__scs.scb.cfsr.byte.mmfsr.val = 0xfe;
}

/**
 *
 * @brief Find out if an MPU fault is a stacking fault
 *
 * This routine determines if an MPU fault is a stacking fault.
 * This may occur upon exception entry.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbMemFaultIsStacking(void)
{
	return !!__scs.scb.cfsr.byte.mmfsr.bit.mstkerr;
}

/**
 *
 * @brief Find out if an MPU fault is an unstacking fault
 *
 * This routine determines if an MPU fault is an unstacking fault.
 * This may occur upon exception exit.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbMemFaultIsUnstacking(void)
{
	return !!__scs.scb.cfsr.byte.mmfsr.bit.munstkerr;
}

/**
 *
 * @brief Find out if an MPU fault is a data access violation
 *
 * If this routine returns 1, read the MMFAR register via _ScbMemFaultAddrGet()
 * to get the faulting address.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbMemFaultIsDataAccessViolation(void)
{
	return !!__scs.scb.cfsr.byte.mmfsr.bit.daccviol;
}

/**
 *
 * @brief Find out if an MPU fault is an instruction access violation
 *
 * This routine determines if an MPU fault is due to an instruction access
 * violation.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbMemFaultIsInstrAccessViolation(void)
{
	return !!__scs.scb.cfsr.byte.mmfsr.bit.iaccviol;
}

/**
 *
 * @brief Find out the faulting address on an MPU fault
 *
 * @return the faulting address
 */

static inline uint32_t _ScbMemFaultAddrGet(void)
{
	return __scs.scb.mmfar;
}

/**
 *
 * @brief Find out if a hard fault is a bus fault
 *
 * This routine determines if a hard fault is a bus fault.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbIsBusFault(void)
{
	return !!__scs.scb.cfsr.byte.bfsr.val;
}

/**
 *
 * @brief Find out if the BFAR register contains a valid value
 *
 * The BFAR register contains the faulting address on bus fault.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbBusFaultIsBfarValid(void)
{
	return !!__scs.scb.cfsr.byte.bfsr.bit.bfarvalid;
}

/**
 *
 * @brief Invalid the value in BFAR
 *
 * This routine clears/invalidates the Bus Fault Address Register.
 * It should be done after processing a bus fault.
 *
 * @return N/A
 */

static inline void _ScbBusFaultBfarReset(void)
{
	__scs.scb.cfsr.byte.bfsr.bit.bfarvalid = 0;
}

/**
 *
 * @brief Clear all bus faults (BFSR register)
 *
 * CFSR/BFSR register is a 'write-one-to-clear' (W1C) register.
 *
 * @return N/A
 */

static inline void _ScbBusFaultAllFaultsReset(void)
{
	__scs.scb.cfsr.byte.bfsr.val = 0xfe;
}

/**
 *
 * @brief Find out if a bus fault is a stacking fault
 *
 * This routine determines if a bus fault is a stacking fault.
 * This may occurs upon exception entry.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbBusFaultIsStacking(void)
{
	return !!__scs.scb.cfsr.byte.bfsr.bit.stkerr;
}

/**
 *
 * @brief Find out if a bus fault is an unstacking fault
 *
 * This routine determines if a bus fault is an unstacking fault.
 * This may occur upon exception exit.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbBusFaultIsUnstacking(void)
{
	return !!__scs.scb.cfsr.byte.bfsr.bit.unstkerr;
}

/**
 *
 * @brief Find out if a bus fault is an imprecise error
 *
 * This routine determines if a bus fault is an imprecise error.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbBusFaultIsImprecise(void)
{
	return !!__scs.scb.cfsr.byte.bfsr.bit.impreciserr;
}

/**
 *
 * @brief Find out if a bus fault is an precise error
 *
 * Read the BFAR register via _ScbBusFaultAddrGet() if this routine returns 1,
 * as it will contain the faulting address.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbBusFaultIsPrecise(void)
{
	return !!__scs.scb.cfsr.byte.bfsr.bit.preciserr;
}

/**
 *
 * @brief Find out if a bus fault is an instruction bus error
 *
 * This routine determines if a bus fault is an instruction bus error.
 * It is signalled only if the instruction is issued.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbBusFaultIsInstrBusErr(void)
{
	return !!__scs.scb.cfsr.byte.bfsr.bit.ibuserr;
}

/**
 *
 * @brief Get the faulting address on a precise bus fault
 *
 * This routine returns the faulting address for a precise bus fault.
 *
 * @return the faulting address
 */

static inline uint32_t _ScbBusFaultAddrGet(void)
{
	return __scs.scb.bfar;
}

/**
 *
 * @brief Find out if a hard fault is a usage fault
 *
 * This routine determines if a hard fault is a usage fault.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbIsUsageFault(void)
{
	return !!__scs.scb.cfsr.byte.ufsr.val;
}

/**
 *
 * @brief Find out if a usage fault is a 'divide by zero' fault
 *
 * This routine determines if a usage fault is a 'divide by zero' fault.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbUsageFaultIsDivByZero(void)
{
	return !!__scs.scb.cfsr.byte.ufsr.bit.divbyzero;
}

/**
 *
 * @brief Find out if a usage fault is a unaligned access error
 *
 * This routine determines if a usage fault is an unaligned access error.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbUsageFaultIsUnaligned(void)
{
	return !!__scs.scb.cfsr.byte.ufsr.bit.unaligned;
}

/**
 *
 * @brief Find out if a usage fault is a co-processor access error
 *
 * This routine determines if a usage fault is caused by a co-processor access.
 * This happens if the co-processor is either absent or disabled.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbUsageFaultIsNoCp(void)
{
	return !!__scs.scb.cfsr.byte.ufsr.bit.nocp;
}

/**
 *
 * @brief Find out if a usage fault is a invalid PC load error
 *
 * Happens if the the instruction address on an exception return is not
 * halfword-aligned.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbUsageFaultIsInvalidPcLoad(void)
{
	return !!__scs.scb.cfsr.byte.ufsr.bit.invpc;
}

/**
 *
 * @brief Find out if a usage fault is a invalid state error
 *
 * Happens if the the instruction address loaded in the PC via a branch, LDR or
 * POP, or if the instruction address installed in a exception vector, does not
 * have bit 0 set; i.e, is not halfword-aligned.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbUsageFaultIsInvalidState(void)
{
	return !!__scs.scb.cfsr.byte.ufsr.bit.invstate;
}

/**
 *
 * @brief Find out if a usage fault is a undefined instruction error
 *
 * The processor tried to execute an invalid opcode.
 *
 * @return 1 if so, 0 otherwise
 */

static inline int _ScbUsageFaultIsUndefinedInstr(void)
{
	return !!__scs.scb.cfsr.byte.ufsr.bit.undefinstr;
}

/**
 *
 * @brief Clear all usage faults (UFSR register)
 *
 * CFSR/UFSR register is a 'write-one-to-clear' (W1C) register.
 *
 * @return N/A
 */

static inline void _ScbUsageFaultAllFaultsReset(void)
{
	__scs.scb.cfsr.byte.ufsr.val = 0xffff;
}

#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SCB__H_ */
