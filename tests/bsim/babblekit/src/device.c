/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "argparse.h"
#include "bs_types.h"

unsigned int bk_device_get_number(void)
{
	return get_device_nbr();
}
