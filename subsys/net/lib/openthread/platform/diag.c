/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <openthread-config.h>
#include <openthread/openthread.h>
#include <openthread/platform/diag.h>

#include "platform-zephyr.h"

/**
 * Diagnostics mode variables.
 *
 */
static bool sDiagMode;

void otPlatDiagProcess(otInstance *aInstance,
		       int argc,
		       char *argv[],
		       char *aOutput,
		       size_t aOutputMaxLen)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(aInstance);

	/* Add more plarform specific diagnostics features here. */
	snprintf(aOutput, aOutputMaxLen,
		 "diag feature '%s' is not supported\r\n", argv[0]);
}

void otPlatDiagModeSet(bool aMode)
{
	sDiagMode = aMode;
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
			     RadioPacket *aFrame,
			     ThreadError aError)
{
	ARG_UNUSED(aInstance);
	ARG_UNUSED(aFrame);
	ARG_UNUSED(aError);
}

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);
}
