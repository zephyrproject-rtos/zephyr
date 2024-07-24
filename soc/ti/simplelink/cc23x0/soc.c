/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <driverlib/setup.h>

void soc_reset_hook(void)
{
	/* Perform necessary trim of the device. */
	SetupTrimDevice();
}
