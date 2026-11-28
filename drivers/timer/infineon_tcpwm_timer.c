/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tcpwm_system_timer

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <cy_tcpwm_counter.h>

#define TCPWM_CNT_NODE DT_INST_PARENT(0)
#define TCPWM_BLK_NODE DT_PARENT(TCPWM_CNT_NODE)
#define TCPWM_BASE     ((TCPWM_Type *)DT_REG_ADDR(TCPWM_BLK_NODE))
#define TCPWM_CNT                                                                                  \
	((DT_REG_ADDR(TCPWM_CNT_NODE) - DT_REG_ADDR(TCPWM_BLK_NODE)) / DT_REG_SIZE(TCPWM_CNT_NODE))
#define TCPWM_CLK_DST ((en_clk_dst_t)DT_PROP(TCPWM_CNT_NODE, clk_dst))
#define TCPWM_IRQN    DT_IRQN(TCPWM_CNT_NODE)

BUILD_ASSERT(DT_PROP(TCPWM_CNT_NODE, resolution) == 32, "the system timer needs a 32-bit counter");

extern unsigned int z_clock_hw_cycles_per_sec;

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TCPWM_IRQN;
#endif

#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_COUNTER_WIDTH 32

static inline uint32_t timer_driver_cycle_get(void)
{
	return Cy_TCPWM_Counter_GetCounter(TCPWM_BASE, TCPWM_CNT);
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	Cy_TCPWM_Counter_SetCompare0(TCPWM_BASE, TCPWM_CNT, (uint32_t)cycles);
}

#include "system_timer_generic.h"

static void tcpwm_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	Cy_TCPWM_ClearInterrupt(TCPWM_BASE, TCPWM_CNT, CY_TCPWM_INT_ON_CC0);
	timer_core_announce();
}

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	/* Only CPU 0 takes the timer interrupt */
}
#endif

static int sys_clock_driver_init(void)
{
	struct ifx_cat1_clock clock = {
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(
			DT_PROP_BY_IDX(DT_INST_CLOCKS_CTLR(0), peri_group, 1),
			DT_PROP(DT_INST_CLOCKS_CTLR(0), div_type)),
		.channel = DT_PROP(DT_INST_CLOCKS_CTLR(0), channel),
	};
	const cy_stc_tcpwm_counter_config_t config = {
		.period = UINT32_MAX,
		.clockPrescaler = CY_TCPWM_COUNTER_PRESCALER_DIVBY_1,
		.runMode = CY_TCPWM_COUNTER_CONTINUOUS,
		.countDirection = CY_TCPWM_COUNTER_COUNT_UP,
		.compareOrCapture = CY_TCPWM_COUNTER_MODE_COMPARE,
		.compare0 = UINT32_MAX,
		.compare1 = UINT32_MAX,
		.interruptSources = CY_TCPWM_INT_ON_CC0,
		.captureInputMode = CY_TCPWM_INPUT_LEVEL,
		.captureInput = CY_TCPWM_INPUT_0,
		.reloadInputMode = CY_TCPWM_INPUT_LEVEL,
		.reloadInput = CY_TCPWM_INPUT_0,
		.startInputMode = CY_TCPWM_INPUT_LEVEL,
		.startInput = CY_TCPWM_INPUT_0,
		.stopInputMode = CY_TCPWM_INPUT_LEVEL,
		.stopInput = CY_TCPWM_INPUT_0,
		.countInputMode = CY_TCPWM_INPUT_LEVEL,
		.countInput = CY_TCPWM_INPUT_1,
	};

	if (ifx_cat1_utils_peri_pclk_assign_divider(TCPWM_CLK_DST, &clock) != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	z_clock_hw_cycles_per_sec = ifx_cat1_utils_peri_pclk_get_frequency(TCPWM_CLK_DST, &clock);
	if (z_clock_hw_cycles_per_sec < CONFIG_SYS_CLOCK_TICKS_PER_SEC) {
		return -EINVAL;
	}

	if (Cy_TCPWM_Counter_Init(TCPWM_BASE, TCPWM_CNT, &config) != CY_TCPWM_SUCCESS) {
		return -EIO;
	}

	IRQ_CONNECT(TCPWM_IRQN, DT_IRQ(TCPWM_CNT_NODE, priority), tcpwm_timer_isr, NULL, 0);

	Cy_TCPWM_Counter_Enable(TCPWM_BASE, TCPWM_CNT);
	Cy_TCPWM_TriggerStart_Single(TCPWM_BASE, TCPWM_CNT);

	timer_core_init();

	Cy_TCPWM_ClearInterrupt(TCPWM_BASE, TCPWM_CNT, CY_TCPWM_INT_ON_CC0);
	irq_enable(TCPWM_IRQN);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
