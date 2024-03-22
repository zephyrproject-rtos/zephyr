/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tfm_hal_defs.h"
#include "tfm_hal_platform_common.h"

enum tfm_hal_status_t tfm_hal_platform_init(void)
{
	return tfm_hal_platform_common_init();
}
