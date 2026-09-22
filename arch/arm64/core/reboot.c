/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/pm_cpu_ops.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_PM_CPU_OPS_PSCI
void __weak sys_arch_reboot(int type)
{
	unsigned char reset_type;

	if (type == SYS_REBOOT_COLD) {
		reset_type = SYS_COLD_RESET;
	} else if (type == SYS_REBOOT_WARM) {
		reset_type = SYS_WARM_RESET;
	} else {
		LOG_ERR("Invalid reboot type");
		return;
	}
	pm_system_reset(reset_type);
}
#else
void __weak sys_arch_reboot(int type)
{
	LOG_WRN("%s is not implemented", __func__);
	ARG_UNUSED(type);
}
#endif
