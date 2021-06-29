/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <sedi.h>

#if defined(CONFIG_ENABLE_SOC_NMI)
#include "soc_nmi.h"
#endif

#define PMU_WAKEUP_INTR_PRIO (2)
#define STACK_SIZE 1024
#define PRIO K_PRIO_COOP(0)
#define INIT_PRIO 99 /* lowest priority of POST_KERNEL */

#define FPEXCODIS BIT(10)

#ifdef CONFIG_PM_SERVICE
extern void sedi_pm_wake_isr(void);
extern void sedi_pm_pciedev_isr(void);
extern void sedi_pm_reset_prep_isr(void);
extern void sedi_pm_pmu2nvic_isr(void);
extern void sedi_cru_clk_ack_isr(void);
extern void sedi_dashboard_isr(void);

static K_THREAD_STACK_DEFINE(pm_handler_stack, STACK_SIZE);
struct k_thread pm_handler_thread;
static void pm_hw_event_handler(void *dummy1, void *dummy2, void *dummy3);

static K_SEM_DEFINE(pm_alert, 0, 1);

static void pm_hw_event_handler(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	k_thread_name_set(NULL, "pm_handler");
	sedi_pm_late_init();
	irq_enable(SEDI_IRQ_PMU_TO_NVIC);

	while (1) {
		k_sem_take(&pm_alert, K_FOREVER);
		sedi_pm_handle_events();
	}
}

static int pm_handler_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	k_thread_create(&pm_handler_thread, pm_handler_stack,
			K_THREAD_STACK_SIZEOF(pm_handler_stack),
			pm_hw_event_handler, NULL, NULL, NULL,
			PRIO, 0, K_NO_WAIT);

	return 0;
}
SYS_INIT(pm_handler_init, POST_KERNEL, INIT_PRIO);

void __pm_reset_prep_callback(int interrupt_type)
{
	ARG_UNUSED(interrupt_type);

	k_sem_give(&pm_alert);
}
#endif

static int config_cpu(void)
{
/* Enable coprocessor CP10 and cp 11 with Full Access */
#if defined(CONFIG_CPU_HAS_FPU)

	/* TODO Disable FPU execption. */
	SCnSCB->ACTLR |= FPEXCODIS;
#endif
	return 0;
}



/**
 *
 * @brief Perform basic hardware initialization
 *
 * @return 0
 */

static int elkhart_lake_pse_init(const struct device *arg)
{
	ARG_UNUSED(arg);
#if !defined(CONFIG_CACHE_DISABLE)
	/* Enable cortex-m7 L1 instruction and data caches */
	SCB_EnableICache();
	SCB_EnableDCache();
#else
	sedi_set_config(SEDI_CONFIG_CACHE_DISABLE, 1);
#endif
	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

#if defined(CONFIG_ENABLE_SOC_NMI)
	soc_nmi_init();
#endif
	/* On FPGA to set TCG_GBE_RGMII_PHY_RX if
	 * GBE AOB PHY daughter board is connected
	 */
#if defined(CONFIG_FPGA_GBE_RGMII_PHY_RX) || defined(CONFIG_BOARD_INTEL_EHL_CRB)
	sedi_pm_set_control(SEDI_PM_IOCTL_TCG_GBE_RGMII_PHY_RX, 1);
#endif

#ifdef CONFIG_PM_SERVICE
#if !defined(CONFIG_DISABLE_SOC_PM_INIT)
	/* register ISR */
	IRQ_CONNECT(SEDI_IRQ_PMU_WAKE, 2, sedi_pm_wake_isr, 0, 0);
	IRQ_CONNECT(SEDI_IRQ_PMU_TO_NVIC, 2, sedi_pm_pmu2nvic_isr, 0, 0);
	IRQ_CONNECT(SEDI_IRQ_PMU_PCIEDEV, 2, sedi_pm_pciedev_isr, 0, 0);
	IRQ_CONNECT(SEDI_IRQ_PMU_RESETPREP, 2, sedi_pm_reset_prep_isr, 0, 0);
	IRQ_CONNECT(SEDI_IRQ_PMU_CRU_CLK_ACK, 2, sedi_cru_clk_ack_isr, 0, 0);
	IRQ_CONNECT(SEDI_IRQ_DASHBOARD, 2, sedi_dashboard_isr, 0, 0);
	/* Power management init */
	sedi_pm_init();
	/* Enable some interrupt */
	irq_enable(SEDI_IRQ_PMU_WAKE);
	irq_enable(SEDI_IRQ_PMU_PCIEDEV);
	irq_enable(SEDI_IRQ_PMU_RESETPREP);
	irq_enable(SEDI_IRQ_PMU_CRU_CLK_ACK);
	irq_enable(SEDI_IRQ_DASHBOARD);
#endif
#endif

	config_cpu();

	return 0;
}

SYS_INIT(elkhart_lake_pse_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
