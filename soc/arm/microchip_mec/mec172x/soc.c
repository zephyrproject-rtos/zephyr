/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

/* Enable SWD and ETM debug interface and pins.
 * NOTE: ETM TRACE pins exposed on MEC172x EVB J30 12,14,16,18,20.
 */
static void configure_debug_interface(void)
{
	struct ecs_regs *ecs = (struct ecs_regs *)(DT_REG_ADDR(DT_NODELABEL(ecs)));

#ifdef CONFIG_SOC_MEC172X_DEBUG_DISABLED
	ecs->ETM_CTRL = 0;
	ecs->DEBUG_CTRL = 0;
#elif defined(CONFIG_SOC_MEC172X_DEBUG_WITHOUT_TRACING)
	ecs->ETM_CTRL = 0;
	ecs->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN | MCHP_ECS_DCTRL_MODE_SWD);
#elif defined(CONFIG_SOC_MEC172X_DEBUG_AND_TRACING)

	#if defined(CONFIG_SOC_MEC172X_DEBUG_AND_ETM_TRACING)
	ecs->ETM_CTRL = 1u;
	ecs->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN | MCHP_ECS_DCTRL_MODE_SWD);
	#elif defined(CONFIG_SOC_MEC172X_DEBUG_AND_SWV_TRACING)
	ecs->ETM_CTRL = 0;
	ecs->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN | MCHP_ECS_DCTRL_MODE_SWD_SWV);
	#endif /* CONFIG_SOC_MEC172X_DEBUG_AND_ETM_TRACING */

#endif /* CONFIG_SOC_MEC172X_DEBUG_DISABLED */
}

static int soc_init(void)
{

	configure_debug_interface();

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
