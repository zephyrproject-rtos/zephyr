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
#include <cmsis_core.h>

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

void soc_early_init_hook(void)
{
	configure_debug_interface();
	soc_ecia_init(MCHP_MEC_ECIA_GIRQ_AGGR_ONLY_BM, MCHP_MEC_ECIA_GIRQ_DIRECT_CAP_BM, 0);
}
