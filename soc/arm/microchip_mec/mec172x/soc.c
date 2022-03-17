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


#if defined(CONFIG_SOC_MEC172X_DEBUG_WITHOUT_TRACING) || \
	defined(CONFIG_SOC_MEC172X_DEBUG_AND_TRACING)
static uintptr_t ecs_regbase = DT_REG_ADDR(DT_NODELABEL(ecs));
#endif

static void configure_debug_interface(void)
{
#ifdef CONFIG_SOC_MEC172X_DEBUG_WITHOUT_TRACING
	struct ecs_regs *regs = ((struct ecs_regs *)ecs_regbase);

	regs->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN |
				MCHP_ECS_DCTRL_MODE_SWD);
#elif defined(CONFIG_SOC_MEC172X_DEBUG_AND_TRACING)
	struct ecs_regs *regs = ((struct ecs_regs *)ecs_regbase);

	#if defined(CONFIG_SOC_MEC172X_DEBUG_AND_ETM_TRACING)
		regs->ETM_CTRL = MCHP_ECS_ETM_CTRL_EN;
		regs->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN |
				MCHP_ECS_DCTRL_MODE_SWD);
	#elif defined(CONFIG_SOC_MEC172X_DEBUG_AND_SWV_TRACING)
		regs->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN |
				MCHP_ECS_DCTRL_MODE_SWD_SWV);
	#endif /* CONFIG_SOC_MEC172X_DEBUG_AND_TRACING */

#endif /* CONFIG_SOC_MEC172X_DEBUG_WITHOUT_TRACING */
}

static int soc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	configure_debug_interface();

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
