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

#else

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <arch/arm/cortex_m/scs.h>
#include <misc/util.h>
#include <stdint.h>

#if defined(CONFIG_ARMV6_M)
#elif defined(CONFIG_ARMV7_M)
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
