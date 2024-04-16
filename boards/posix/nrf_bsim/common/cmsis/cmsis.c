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

void NVIC_EnableIRQ(IRQn_Type IRQn)
{
	hw_irq_ctrl_enable_irq(CONFIG_NATIVE_SIMULATOR_MCU_N, IRQn);
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
