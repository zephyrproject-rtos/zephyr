/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include "irq_ctrl.h"
#include "posix_core.h"
#include "posix_board_if.h"
#include "board_soc.h"
#include "bs_tracing.h"

/*
 *  Replacement for ARMs NVIC functions()
 */
void NVIC_SetPendingIRQ(IRQn_Type IRQn)
{
	hw_irq_ctrl_raise_im_from_sw(CONFIG_NATIVE_SIMULATOR_MCU_N, IRQn);
}

void NVIC_ClearPendingIRQ(IRQn_Type IRQn)
{
	hw_irq_ctrl_clear_irq(CONFIG_NATIVE_SIMULATOR_MCU_N, IRQn);
}

void NVIC_DisableIRQ(IRQn_Type IRQn)
{
	hw_irq_ctrl_disable_irq(CONFIG_NATIVE_SIMULATOR_MCU_N, IRQn);
}

uint32_t NVIC_GetPendingIRQ(IRQn_Type IRQn)
{
	return hw_irq_ctrl_is_irq_pending(CONFIG_NATIVE_SIMULATOR_MCU_N, IRQn);
}

void NVIC_EnableIRQ(IRQn_Type IRQn)
{
	hw_irq_ctrl_enable_irq(CONFIG_NATIVE_SIMULATOR_MCU_N, IRQn);
}

uint32_t NVIC_GetEnableIRQ(IRQn_Type IRQn)
{
	return hw_irq_ctrl_is_irq_enabled(CONFIG_NATIVE_SIMULATOR_MCU_N, IRQn);
}

void NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority)
{
	hw_irq_ctrl_prio_set(CONFIG_NATIVE_SIMULATOR_MCU_N, IRQn, priority);
}

uint32_t NVIC_GetPriority(IRQn_Type IRQn)
{
	return hw_irq_ctrl_get_prio(CONFIG_NATIVE_SIMULATOR_MCU_N, IRQn);
}

void NVIC_SystemReset(void)
{
	bs_trace_error_time_line("%s called. Exiting\n", __func__);
}

/*
 * Replacements for some other CMSIS functions
 */
void __enable_irq(void)
{
	hw_irq_ctrl_change_lock(CONFIG_NATIVE_SIMULATOR_MCU_N, false);
}

void __disable_irq(void)
{
	hw_irq_ctrl_change_lock(CONFIG_NATIVE_SIMULATOR_MCU_N, true);
}

uint32_t __get_PRIMASK(void)
{
	return hw_irq_ctrl_get_current_lock(CONFIG_NATIVE_SIMULATOR_MCU_N);
}

void __set_PRIMASK(uint32_t primask)
{
	hw_irq_ctrl_change_lock(CONFIG_NATIVE_SIMULATOR_MCU_N, primask != 0);
}

void __WFE(void)
{
	nrfbsim_WFE_model();
}

void __WFI(void)
{
	__WFE();
}

void __SEV(void)
{
	nrfbsim_SEV_model();
}

/*
 *  Implement the following ARM instructions
 *
 *  - STR Exclusive(8,16 & 32bit) (__STREX{B,H,W})
 *  - LDR Exclusive(8,16 & 32bit) (__LDREX{B,H,W})
 *  - CLREX : Exclusive lock removal (__CLREX)
 *
 *  Description:
 *    From ARMs description it is relatively unclear how the LDREX/STREX/CLREX
 *    are really implemented in M4/M33 devices.
 *
 *    The current model simply sets a local monitor (local to the processor)
 *    exclusive lock for the current MCU when a LDREX is executed.
 *    STREX check this lock, and succeeds if set, fails otherwise.
 *    The lock is cleared whenever STREX or CLREX are run, or when we return
 *    from an interrupt handler.
 *    See Arm v8-M Architecture Reference Manual: "B9.2 The local monitors" and
 *    "B9.4 Exclusive access instructions and the monitors".
 *
 *    The address is ignored, and we do not model a "system/global" monitor.
 *    The access width is ignored from the locking point of view.
 *    In principle this model would seem to fulfill the functionality described
 *    by ARM.
 *
 *    Note that as the POSIX arch will not make an embedded
 *    thread lose context while just executing its own code, and it does not
 *    allow parallel embedded SW threads to execute at the same exact time,
 *    there is no real need to protect atomicity.
 *    But, some embedded code may use this instructions in between busy waits,
 *    and expect that an interrupt in the meanwhile will indeed cause a
 *    following STREX to fail.
 *
 *    As this ARM exclusive access monitor mechanism can in principle be
 *    used for other, unexpected, purposes, this simple replacement may not be
 *    enough.
 */

static bool ex_lock; /* LDREX/STREX/CLREX lock state */

bool nrfbsim_STREXlock_model(void)
{
	if (ex_lock == false) {
		return true;
	}

	ex_lock = false;
	return false;
}

void nrfbsim_clear_excl_access(void)
{
	ex_lock = false;
}

/**
 * \brief   Pretend to execute a STR Exclusive (8 bit)
 * \details Executes an exclusive STR instruction for 8 bit values.
 * \param [in]  value  Value to store
 * \param [in]    ptr  Pointer to location
 * \return          0  Function succeeded
 * \return          1  Function did not succeeded (value not changed)
 */
uint32_t __STREXB(uint8_t value, volatile uint8_t *ptr)
{
	if (nrfbsim_STREXlock_model()) {
		return 1;
	}
	*ptr = value;
	return 0;
}

/**
 * \brief   Pretend to execute a STR Exclusive (16 bit)
 * \details Executes a exclusive STR instruction for 16 bit values.
 * \param [in]  value  Value to store
 * \param [in]    ptr  Pointer to location
 * \return          0  Function succeeded
 * \return          1  Function did not succeeded (value not changed)
 */
uint32_t __STREXH(uint16_t value, volatile uint16_t *ptr)
{
	if (nrfbsim_STREXlock_model()) {
		return 1;
	}
	*ptr = value;
	return 0;
}

/**
 * \brief   Pretend to execute a STR Exclusive (32 bit)
 * \details Executes a exclusive~ STR instruction for 32 bit values.
 * \param [in]  value  Value to store
 * \param [in]    ptr  Pointer to location
 * \return          0  Function succeeded
 * \return          1  Function did not succeeded (value not changed)
 */
uint32_t __STREXW(uint32_t value, volatile uint32_t *ptr)
{
	if (nrfbsim_STREXlock_model()) {
		return 1;
	}
	*ptr = value;
	return 0;
}

/**
 * \brief   Pretend to execute a LDR Exclusive (8 bit)
 * \details Executes an exclusive LDR instruction for 8 bit value.
 *          Meaning, set an exclusive lock, and load the stored value
 * \param [in]    ptr  Pointer to data
 * \return             value of type uint8_t at (*ptr)
 */
uint8_t __LDREXB(volatile uint8_t *ptr)
{
	ex_lock = true;
	return *ptr;
}

/**
 * \brief   Pretend to execute a LDR Exclusive (16 bit)
 * \details Executes an ~exclusive~ LDR instruction for 16 bit value.
 *          Meaning, set an exclusive lock, and load the stored value
 * \param [in]    ptr  Pointer to data
 * \return             value of type uint8_t at (*ptr)
 */
uint16_t __LDREXH(volatile uint16_t *ptr)
{
	ex_lock = true;
	return *ptr;
}

/**
 * \brief   Execute a LDR Exclusive (32 bit)
 * \details Executes an exclusive LDR instruction for 32 bit value.
 *          Meaning, set an exclusive lock, and load the stored value
 * \param [in]    ptr  Pointer to data
 * \return             value of type uint8_t at (*ptr)
 */
uint32_t __LDREXW(volatile uint32_t *ptr)
{
	ex_lock = true;
	return *ptr;
}

/**
 * \brief   Remove the exclusive lock
 * \details Removes the exclusive lock which is created by LDREX
 */
void __CLREX(void)
{
	ex_lock = false;
}
