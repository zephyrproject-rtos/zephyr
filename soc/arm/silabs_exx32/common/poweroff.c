/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_SOC_GECKO_PM_BACKEND_EMU)
#include <em_emu.h>
#elif defined(CONFIG_SOC_GECKO_PM_BACKEND_PMGR)
#include <sl_power_manager.h>
#endif

void z_sys_poweroff(void)
{
#if defined(CONFIG_SOC_GECKO_PM_BACKEND_EMU)
	EMU_EnterEM4();
#elif defined(CONFIG_SOC_GECKO_PM_BACKEND_PMGR)
	sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM4);
	sl_power_manager_sleep();
#else
	#error "No valid power management backend selected"
#endif

	CODE_UNREACHABLE;
}
