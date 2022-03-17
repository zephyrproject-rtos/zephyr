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


static uintptr_t ecs_regbase = DT_REG_ADDR(DT_NODELABEL(ecs));
static void configure_debug_interface(void)
{
	struct ecs_regs *regs = ((struct ecs_regs *)ecs_regbase);

	/* No debug support */
	regs->DEBUG_CTRL = 0;
	regs->ETM_CTRL = 0;

#ifdef CONFIG_SOC_MEC172X_DEBUG_WITHOUT_TRACING
	/* Release JTAG TDI and JTAG TDO pins so they can be
	 * controlled by their respective PCR register (UART2).
	 * For more details see table 44-1
	 */

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

	if (IS_ENABLED(CONFIG_SOC_MEC172X_TEST_CLK_OUT)) {

		struct gpio_regs * const regs =
			(struct gpio_regs * const)DT_REG_ADDR(DT_NODELABEL(pinctrl));

		regs->CTRL[MCHP_GPIO_0060_ID] = MCHP_GPIO_CTRL_MUX_F2 |
						MCHP_GPIO_CTRL_IDET_DISABLE;
	}

	configure_debug_interface();

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
