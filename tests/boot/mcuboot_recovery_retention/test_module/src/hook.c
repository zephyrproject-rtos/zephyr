/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <bootutil/mcuboot_status.h>

void mcuboot_status_change(mcuboot_status_type_t status)
{
	printf("mcuboot_status: %d\n", status);
}
