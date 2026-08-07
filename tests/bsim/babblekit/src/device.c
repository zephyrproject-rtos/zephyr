/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsim_args_runner.h"

unsigned int bk_device_get_number(void)
{
	return get_device_nbr();
}
