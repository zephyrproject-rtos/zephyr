/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_OPENTHREAD_PLATFORM_POWER_H_
#define ZEPHYR_MODULES_OPENTHREAD_PLATFORM_POWER_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/instance.h>

void platformPowerInit(void);
void platformPowerProcessBegin(void);
void platformPowerProcess(otInstance *instance);
bool platformPowerHasPendingEvents(otInstance *instance);
void platformPowerSignalPending(void);
void platformPowerNotifyAlarmMilliStart(uint32_t fire_at_ms);
void platformPowerNotifyAlarmMilliStop(void);
void platformPowerNotifyAlarmMilliFired(void);
void platformPowerNotifyAlarmMicroStart(uint32_t fire_at_us);
void platformPowerNotifyAlarmMicroStop(void);
void platformPowerNotifyAlarmMicroFired(void);

#endif /* ZEPHYR_MODULES_OPENTHREAD_PLATFORM_POWER_H_ */
