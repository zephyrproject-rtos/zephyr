/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/toolchain.h>

#include <soc_sysctl.h>

LOG_MODULE_REGISTER(mspm_reboot, CONFIG_LOG_DEFAULT_LEVEL);

#define MSPM_SYSCTL_BASE DT_REG_ADDR(DT_NODELABEL(sysctl))

void sys_arch_reboot(int type)
{
	uint32_t level;

	switch (type) {
	case SYS_REBOOT_WARM:
		level = SYSCTL_RESETLEVEL_CPU;
		break;
	case SYS_REBOOT_COLD:
		level = SYSCTL_RESETLEVEL_POR;
		break;
	default:
		LOG_ERR("Unsupported reboot type: %d", type);
		return;
	}

	barrier_dsync_fence_full();
	sys_write32(level, MSPM_SYSCTL_BASE + SYSCTL_RESETLEVEL_OFFSET);
	sys_write32(SYSCTL_RESETCMD_KEY | SYSCTL_RESETCMD_GO,
		    MSPM_SYSCTL_BASE + SYSCTL_RESETCMD_OFFSET);
	barrier_dsync_fence_full();
}
