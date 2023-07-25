/*
 * Copyright (c) 2021 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/arch/arm/aarch32/cortex_a_r/cmsis.h>

bool z_arm_thread_is_in_user_mode(void)
{
	uint32_t value;

	/*
	 * For Cortex-R, the mode (lower 5) bits will be 0x10 for user mode.
	 */
	value = __get_CPSR();
	return ((value & CPSR_M_Msk) == CPSR_M_USR);
}
