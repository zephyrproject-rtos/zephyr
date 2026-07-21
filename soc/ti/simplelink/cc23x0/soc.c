/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <driverlib/setup.h>

const uint_least8_t GPIO_pinLowerBound;
const uint_least8_t GPIO_pinUpperBound = 25;

void soc_reset_hook(void)
{
	/* Perform necessary trim of the device. */
	SetupTrimDevice();
}
