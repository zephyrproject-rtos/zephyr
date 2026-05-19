/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for MAX32xxx MCUs
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/devicetree/fixed-partitions.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>

#include <wrap_max32_sys.h>

#ifdef CONFIG_MAX32_SECONDARY_RV32
#include <fcr_regs.h>

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_code_rv32_partition),
	     "Set zephyr,code-rv32-partition chosen property to launch the RV32 core");

#endif

#if defined(CONFIG_MAX32_STANDBY_DELAY) && (CONFIG_MAX32_STANDBY_DELAY > 0)
struct k_timer max32_soc_timer;
void max32_soc_timer_callback(struct k_timer *timer_id)
{
	/* Allow the system to enter standby */
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}
#endif /* defined(MAX32_STANDBY_DELAY) && (MAX32_STANDBY_DELAY > 0) */

#if defined(CONFIG_MAX32_ON_ENTER_CPU_IDLE_HOOK)
bool z_arm_on_enter_cpu_idle(void)
{
	/* Returning false prevent device goes to sleep mode */
	return false;
}
#endif

#define CPU1_CPU              DT_NODELABEL(cpu1)
#define DO_CPU1_DEBUG_PINCTRL (DT_PINCTRL_HAS_NAME(CPU1_CPU, default))

#if (CONFIG_MAX32_SECONDARY_RV32 || CONFIG_MAX32_SECONDARY_M4) && DO_CPU1_DEBUG_PINCTRL

PINCTRL_DT_DEFINE(CPU1_CPU);

static const struct pinctrl_dev_config *cpu1_pcfg = PINCTRL_DT_DEV_CONFIG_GET(CPU1_CPU);

#endif

#if (defined(CONFIG_MAX32_SECONDARY_RV32) &&                \
	defined(CONFIG_MAX32_SECONDARY_RV32_STARTUP_DELAY) &&  \
	(CONFIG_MAX32_SECONDARY_RV32_STARTUP_DELAY > 0)) || \
	(defined(CONFIG_MAX32_SECONDARY_M4) &&                \
	defined(CONFIG_MAX32_SECONDARY_M4_STARTUP_DELAY) &&  \
	(CONFIG_MAX32_SECONDARY_M4_STARTUP_DELAY > 0))

static ALWAYS_INLINE void soc_max32_secondary_delay(int n)
{
	while (n--) {
		__asm__ volatile("nop");
	}
}

#endif

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	/* Apply device related preinit configuration */
	max32xx_system_init();

	/* Temporarily prevent  the system from entering standby to prevent debug lockout */
#if defined(CONFIG_MAX32_STANDBY_DELAY) && (CONFIG_MAX32_STANDBY_DELAY > 0)
	/* Prevent the system from entering standby mode */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

	/* Start a one-shot timer to put the pm lock */
	k_timer_init(&max32_soc_timer, max32_soc_timer_callback, NULL);
	k_timer_start(&max32_soc_timer, K_MSEC(CONFIG_MAX32_STANDBY_DELAY), K_NO_WAIT);
#endif /* defined(MAX32_STANDBY_DELAY) && (MAX32_STANDBY_DELAY > 0) */

#if (CONFIG_MAX32_SECONDARY_RV32 || CONFIG_MAX32_SECONDARY_M4) && DO_CPU1_DEBUG_PINCTRL
	pinctrl_apply_state(cpu1_pcfg, PINCTRL_STATE_DEFAULT);
#endif

#ifdef CONFIG_MAX32_SECONDARY_RV32

#if defined(CONFIG_MAX32_SECONDARY_RV32_STARTUP_DELAY) && \
	(CONFIG_MAX32_SECONDARY_RV32_STARTUP_DELAY > 0)
	soc_max32_secondary_delay(CONFIG_MAX32_SECONDARY_RV32_STARTUP_DELAY);
#endif

	MXC_FCR->urvbootaddr = DT_REG_ADDR(DT_CHOSEN(zephyr_code_rv32_partition));
	MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_CPU1);
	MXC_GCR->rst1 |= MXC_F_GCR_RST1_CPU1;
#endif /* CONFIG_MAX32_SECONDARY_RV32 */


#ifdef CONFIG_MAX32_SECONDARY_M4

#if defined(CONFIG_MAX32_SECONDARY_M4_STARTUP_DELAY) && \
	(CONFIG_MAX32_SECONDARY_M4_STARTUP_DELAY > 0)
	soc_max32_secondary_delay(CONFIG_MAX32_SECONDARY_M4_STARTUP_DELAY);
#endif

	MXC_GCR->gp0 = DT_REG_ADDR(DT_CHOSEN(zephyr_code_cpu1_partition));
	MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_CPU1);
#endif /* CONFIG_MAX32_SECONDARY_RV32 */
}

#if defined(CONFIG_SOC_FAMILY_MAX32_RV32)

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	MXC_GCR->rst1 |= MXC_F_GCR_RST1_CPU1;
}

#endif
