/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/firmware/scmi/clk.h>
#include <zephyr/drivers/firmware/scmi/nxp/cpu.h>
#include <zephyr/drivers/firmware/scmi/power.h>
#include <zephyr/dt-bindings/clock/imx95_clock.h>
#include <zephyr/dt-bindings/power/imx95_power.h>
#include <soc.h>

/* SCMI power domain states */
#define POWER_DOMAIN_STATE_ON  0x00000000
#define POWER_DOMAIN_STATE_OFF 0x40000000

void soc_late_init_hook(void)
{
#ifdef CONFIG_CACHE_MANAGEMENT
	sys_cache_data_enable();
	sys_cache_instr_enable();
#endif /* CONFIG_CACHE_MANAGEMENT */
}

static int soc_init(void)
{
	int ret = 0;
#if defined(CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS)
	struct scmi_cpu_sleep_mode_config cpu_cfg = {0};
#endif /* CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS */

#if defined(CONFIG_ETH_NXP_IMX_NETC) && (DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(netc)) != 0)
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(scmi_clk));
	struct scmi_protocol *proto = clk_dev->data;
	struct scmi_clock_rate_config clk_cfg = {0};
	struct scmi_power_state_config pwr_cfg = {0};
	uint32_t power_state = POWER_DOMAIN_STATE_OFF;
	uint64_t enetref_clk = 250000000; /* 250 MHz*/

	/* Power up NETCMIX */
	pwr_cfg.domain_id = IMX95_PD_NETC;
	pwr_cfg.power_state = POWER_DOMAIN_STATE_ON;

	ret = scmi_power_state_set(&pwr_cfg);
	if (ret) {
		return ret;
	}

	while (power_state != POWER_DOMAIN_STATE_ON) {
		ret = scmi_power_state_get(IMX95_PD_NETC, &power_state);
		if (ret) {
			return ret;
		}
	}

	/* ENETREF clock init */
	ret = scmi_clock_parent_set(proto, IMX95_CLK_ENETREF, IMX95_CLK_SYSPLL1_PFD0);
	if (ret) {
		return ret;
	}

	clk_cfg.flags = SCMI_CLK_RATE_SET_FLAGS_ROUNDS_AUTO;
	clk_cfg.clk_id = IMX95_CLK_ENETREF;
	clk_cfg.rate[0] = enetref_clk & 0xffffffff;
	clk_cfg.rate[1] = (enetref_clk >> 32) & 0xffffffff;

	ret = scmi_clock_rate_set(proto, &clk_cfg);
	if (ret) {
		return ret;
	}
#endif

#if defined(CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS)
	cpu_cfg.cpu_id = CPU_IDX_M7P;
	cpu_cfg.sleep_mode = CPU_SLEEP_MODE_RUN;

	ret = scmi_cpu_sleep_mode_set(&cpu_cfg);
	if (ret) {
		return ret;
	}
#endif /* CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS */

	return ret;
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	struct scmi_cpu_sleep_mode_config cpu_cfg = {0};

	/* iMX95 M7 core is based on ARMv7-M architecture. For this architecture,
	 * the current implementation of arch_irq_lock of zephyr is based on BASEPRI,
	 * which will only retain abnormal interrupts such as NMI,
	 * and all other interrupts from the CPU(including systemtick) will be masked,
	 * which makes the CORE unable to be woken up from WFI.
	 * Set PRIMASK as workaround, Shield the CPU from responding to interrupts,
	 * the CPU will not jump to the interrupt service routine (ISR).
	 */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		cpu_cfg.cpu_id = CPU_IDX_M7P;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_WAIT;
		scmi_cpu_sleep_mode_set(&cpu_cfg);
		__DSB();
		__WFI();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		cpu_cfg.cpu_id = CPU_IDX_M7P;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_STOP;
		scmi_cpu_sleep_mode_set(&cpu_cfg);
		__DSB();
		__WFI();
		break;
	case PM_STATE_STANDBY:
		cpu_cfg.cpu_id = CPU_IDX_M7P;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_SUSPEND;
		scmi_cpu_sleep_mode_set(&cpu_cfg);
		__DSB();
		__WFI();
		break;
	default:
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);

	struct scmi_cpu_sleep_mode_config cpu_cfg = {0};
	/* restore M7 core state into ACTIVE. */
	cpu_cfg.cpu_id = CPU_IDX_M7P;
	cpu_cfg.sleep_mode = CPU_SLEEP_MODE_RUN;
	scmi_cpu_sleep_mode_set(&cpu_cfg);

	/* Clear PRIMASK */
	__enable_irq();
}

/*
 * Because platform is using ARM SCMI, drivers like scmi, mbox etc. are
 * initialized during PRE_KERNEL_1. Common init hooks is not able to use.
 * SoC early init and board early init could be run during PRE_KERNEL_2 instead.
 */
SYS_INIT(soc_init, PRE_KERNEL_2, 0);
