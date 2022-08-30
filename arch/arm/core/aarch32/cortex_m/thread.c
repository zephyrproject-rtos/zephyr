/*
 * Copyright (c) 2021 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

bool z_arm_thread_is_in_user_mode(void)
{
	uint32_t value;

	/* return mode information */
	value = __get_CONTROL();
	return (value & CONTROL_nPRIV_Msk) != 0;
}
