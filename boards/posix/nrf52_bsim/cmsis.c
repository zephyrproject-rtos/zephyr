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

/*
 *  Replacement for ARMs NVIC functions()
 */
void NVIC_SetPendingIRQ(IRQn_Type IRQn)
{
	hw_irq_ctrl_raise_im_from_sw(IRQn);
}

void NVIC_ClearPendingIRQ(IRQn_Type IRQn)
{
	hw_irq_ctrl_clear_irq(IRQn);
}

void NVIC_DisableIRQ(IRQn_Type IRQn)
{
	hw_irq_ctrl_disable_irq(IRQn);
}

void NVIC_EnableIRQ(IRQn_Type IRQn)
{
	hw_irq_ctrl_enable_irq(IRQn);
}

void NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority)
{
	hw_irq_ctrl_prio_set(IRQn, priority);
}

uint32_t NVIC_GetPriority(IRQn_Type IRQn)
{
	return hw_irq_ctrl_get_prio(IRQn);
}

void NVIC_SystemReset(void)
{
	posix_print_warning("%s called. Exiting\n", __func__);
	posix_exit(1);
}

/*
 * Replacements for some other CMSIS functions
 */
void __enable_irq(void)
{
	hw_irq_ctrl_change_lock(false);
}

void __disable_irq(void)
{
	hw_irq_ctrl_change_lock(true);
}

uint32_t __get_PRIMASK(void)
{
	return hw_irq_ctrl_get_current_lock();
}
