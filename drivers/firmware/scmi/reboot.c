/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/system.h>
#include <string.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(scmi_reboot);

static int scmi_reboot_handler(int type)
{
	struct scmi_system_power_state_config cfg;
	int ret;

	cfg.flags = SCMI_SYSTEM_POWER_FLAG_FORCEFUL;

	switch (type) {
	case SYS_REBOOT_WARM:
		cfg.system_state = SCMI_SYSTEM_POWER_STATE_WARM_RESET;
		break;

	case SYS_REBOOT_COLD:
		cfg.system_state = SCMI_SYSTEM_POWER_STATE_COLD_RESET;
		break;

	default:
		LOG_ERR("Unsupported reboot type: %d", type);
		return -EINVAL;
	}

	ret = scmi_system_power_state_set(&cfg);
	if (ret < 0) {
		LOG_ERR("Failed to request power state 0x%x: %d", cfg.system_state, ret);
	}

	return ret;
}

void sys_arch_reboot(int type)
{
	scmi_reboot_handler(type);
}
