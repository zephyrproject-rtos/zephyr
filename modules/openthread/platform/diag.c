/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <openthread/platform/diag.h>

#include "platform-zephyr.h"

/**
 * Diagnostics mode variables.
 *
 */
static bool sDiagMode;

otError otPlatDiagProcess(otInstance *aInstance,
			  uint8_t argc,
			  char   *argv[],
			  char   *aOutput,
			  size_t  aOutputMaxLen)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(aInstance);

	/* Add more platform specific diagnostics features here. */
	snprintk(aOutput, aOutputMaxLen,
		 "diag feature '%s' is not supported\r\n", argv[0]);

	return OT_ERROR_NOT_IMPLEMENTED;
}

void otPlatDiagModeSet(bool aMode)
{
	sDiagMode = aMode;

	if (!sDiagMode) {
		otPlatRadioSleep(NULL);
	}
}

bool otPlatDiagModeGet(void)
{
	return sDiagMode;
}

void otPlatDiagChannelSet(uint8_t aChannel)
{
	ARG_UNUSED(aChannel);
}

void otPlatDiagTxPowerSet(int8_t aTxPower)
{
	ARG_UNUSED(aTxPower);
}

void otPlatDiagRadioReceived(otInstance *aInstance,
			     otRadioFrame *aFrame,
			     otError aError)
{
	ARG_UNUSED(aInstance);
	ARG_UNUSED(aFrame);
	ARG_UNUSED(aError);
}

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);
}
