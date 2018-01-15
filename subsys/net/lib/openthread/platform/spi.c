/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <openthread/platform/uart.h>
#include <openthread/platform/spi-slave.h>

#include "platform-zephyr.h"

/* Spi-slave stubs */

otError otPlatSpiSlaveEnable(
	otPlatSpiSlaveTransactionCompleteCallback aCompleteCallback,
	otPlatSpiSlaveTransactionProcessCallback aProcessCallback,
	void *aContext)
{
	ARG_UNUSED(aCompleteCallback);
	ARG_UNUSED(aProcessCallback);
	ARG_UNUSED(aContext);

	return OT_ERROR_NOT_IMPLEMENTED;
}

void otPlatSpiSlaveDisable(void)
{
	/* Intentionally empty */
}

otError otPlatSpiSlavePrepareTransaction(
	u8_t *anOutputBuf,
	u16_t anOutputBufLen,
	u8_t *anInputBuf,
	u16_t anInputBufLen,
	bool aRequestTransactionFlag
)
{
	ARG_UNUSED(anOutputBuf);
	ARG_UNUSED(anOutputBufLen);
	ARG_UNUSED(anInputBuf);
	ARG_UNUSED(anInputBufLen);
	ARG_UNUSED(aRequestTransactionFlag);

	return OT_ERROR_NOT_IMPLEMENTED;
}
