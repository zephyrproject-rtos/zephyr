/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief
 *   This file includes the platform-specific initializers.
 */

#include <kernel.h>
#include <openthread/instance.h>
#include <openthread/tasklet.h>

#include "platform-zephyr.h"

void otSysInit(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	platformAlarmInit();
	platformRadioInit();
}

void otSysProcessDrivers(otInstance *aInstance)
{
	platformRadioProcess(aInstance);
	platformAlarmProcess(aInstance);

	if (IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR)) {
		platformUartProcess(aInstance);
	}
}
