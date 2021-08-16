/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <pm/pm.h>
#include <pm/device_policy.h>

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

#define PM_STATES_LEN (1 + PM_STATE_SOFT_OFF - PM_STATE_ACTIVE)

static struct {
	enum pm_state system_state;
	enum pm_device_state device_state;
} states_map[PM_STATES_LEN] = {
	{PM_STATE_ACTIVE, PM_DEVICE_STATE_ACTIVE},
	{PM_STATE_RUNTIME_IDLE, PM_DEVICE_STATE_LOW_POWER},
	{PM_STATE_SUSPEND_TO_IDLE, PM_DEVICE_STATE_LOW_POWER},
	{PM_STATE_STANDBY, PM_DEVICE_STATE_LOW_POWER},
	{PM_STATE_SUSPEND_TO_RAM, PM_DEVICE_STATE_SUSPENDED},
	{PM_STATE_SUSPEND_TO_DISK, PM_DEVICE_STATE_SUSPENDED},
	{PM_STATE_SOFT_OFF, PM_DEVICE_STATE_OFF}
};

enum pm_device_state pm_device_policy_next_state(const struct device *dev,
					 const struct pm_state_info *state)
{
	size_t i;

	/* Right now the device is not used by the default policy. But it can
	 * be used in the future if the device states transition is defined
	 * using devicetree.
	 */
	ARG_UNUSED(dev);

	for (i = 0; i < ARRAY_SIZE(states_map); i++) {
		if (states_map[i].system_state == state->state) {
			return states_map[i].device_state;
		}
	}

	return PM_DEVICE_STATE_ACTIVE;
}
