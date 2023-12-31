/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include <stm32_ll_cortex.h>
#include <stm32_ll_pwr.h>

#ifdef CONFIG_WAKEUP_SOURCE
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_pwr)
#define DT_DRV_COMPAT st_stm32_pwr
#define PWR_STM32_WKUP_PINS_PULL_CFG DT_INST_PROP_OR(0, wkup_pins_pull, 0)
#endif
#endif /* CONFIG_WAKEUP_SOURCE */

void z_sys_poweroff(void)
{
#ifdef CONFIG_WAKEUP_SOURCE
#if PWR_STM32_WKUP_PINS_PULL_CFG
	LL_PWR_EnablePUPDConfig();
#else
	LL_PWR_DisablePUPDConfig();
#endif /* PWR_STM32_WKUP_PINS_PULL_CFG */
	LL_PWR_ClearFlag_WU();
#endif /* CONFIG_WAKEUP_SOURCE */

	LL_PWR_SetPowerMode(LL_PWR_SHUTDOWN_MODE);
	LL_LPM_EnableDeepSleep();

	k_cpu_idle();

	CODE_UNREACHABLE;
}
