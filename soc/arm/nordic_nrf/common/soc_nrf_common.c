/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRFxx family processors
 */

#include "../soc_nrf_common.h"

void z_platform_init(void)
{
	SystemInit();
}
