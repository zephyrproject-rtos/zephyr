/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/firmware/scmi/clk.h>
#include <zephyr/drivers/firmware/scmi/nxp/cpu.h>
#include <zephyr/dt-bindings/clock/imx943_clock.h>
#include <soc.h>

void soc_early_init_hook(void)
{
#ifdef CONFIG_CACHE_MANAGEMENT
	sys_cache_data_enable();
	sys_cache_instr_enable();
#endif
}

static int soc_init(void)
{
#if defined(CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS)
	struct scmi_cpu_sleep_mode_config cpu_cfg = {0};
#endif /* CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS */
	int ret = 0;

#if defined(CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS)
	cpu_cfg.cpu_id = CPU_IDX_M7P_1;
	cpu_cfg.sleep_mode = CPU_SLEEP_MODE_RUN;

	ret = scmi_cpu_sleep_mode_set(&cpu_cfg);
	if (ret) {
		return ret;
	}
#endif /* CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS */
	return ret;
}

/*
 * Because platform is using ARM SCMI, drivers like scmi, mbox etc. are
 * initialized during PRE_KERNEL_1. Common init hooks is not able to use.
 * SoC early init and board early init could be run during PRE_KERNEL_2 instead.
 */
SYS_INIT(soc_init, PRE_KERNEL_2, 0);
