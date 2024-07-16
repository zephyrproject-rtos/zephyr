/*
 * Copyright (c) 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief
 *   This file includes the platform-specific initializers.
 */

#include <stdlib.h>

#include <openthread/instance.h>
#include <openthread/tasklet.h>

#include "platform-zephyr.h"
#include "platform.h"


void otSysInit(int argc, char *argv[])
{
	otPlatRadioInit();
	platformAlarmInit();
}

void otSysDeinit(void)
{
	otPlatRadioDeinit();
}

void otSysProcessDrivers(otInstance *aInstance)
{
	otPlatRadioProcess(aInstance);
	platformAlarmProcess(aInstance);
}
