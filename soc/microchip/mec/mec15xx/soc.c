/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>

/*
 * Initialize MEC1501 EC Interrupt Aggregator (ECIA) and external NVIC
 * inputs.
 */
static int soc_ecia_init(void)
{
	GIRQ_Type *pg;
	uint32_t n;

	mchp_pcr_periph_slp_ctrl(PCR_ECIA, MCHP_PCR_SLEEP_DIS);

	ECS_REGS->INTR_CTRL |= MCHP_ECS_ICTRL_DIRECT_EN;

	/* gate off all aggregated outputs */
	ECIA_REGS->BLK_EN_CLR = 0xFFFFFFFFul;
	/* gate on GIRQ's that are aggregated only */
	ECIA_REGS->BLK_EN_SET = MCHP_ECIA_AGGR_BITMAP;

	/* Clear all GIRQn source enables and source status */
	pg = &ECIA_REGS->GIRQ08;
	for (n = MCHP_FIRST_GIRQ; n <= MCHP_LAST_GIRQ; n++) {
		pg->EN_CLR = 0xFFFFFFFFul;
		pg->SRC = 0xFFFFFFFFul;
		pg++;
	}

	/* Clear all external NVIC enables and pending status */
	for (n = 0u; n < MCHP_NUM_NVIC_REGS; n++) {
		NVIC->ICER[n] = 0xFFFFFFFFul;
		NVIC->ICPR[n] = 0xFFFFFFFFul;
	}

	return 0;
}

static void configure_debug_interface(void)
{
	/* No debug support */
	ECS_REGS->DEBUG_CTRL = 0;
	ECS_REGS->ETM_CTRL = 0;

#ifdef CONFIG_SOC_MEC1501_DEBUG_WITHOUT_TRACING
	/* Release JTAG TDI and JTAG TDO pins so they can be
	 * controlled by their respective PCR register (UART2).
	 * For more details see table 44-1
	 */
	ECS_REGS->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN |
				MCHP_ECS_DCTRL_MODE_SWD);
#elif defined(CONFIG_SOC_MEC1501_DEBUG_AND_TRACING)
	#if defined(CONFIG_SOC_MEC1501_DEBUG_AND_ETM_TRACING)
	#pragma error "TRACE DATA are not exposed in HW connector"
	#elif defined(CONFIG_SOC_MEC1501_DEBUG_AND_SWV_TRACING)
		ECS_REGS->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN |
				MCHP_ECS_DCTRL_MODE_SWD_SWV);
	#endif /* CONFIG_SOC_MEC1501_DEBUG_AND_TRACING */

#endif /* CONFIG_SOC_MEC1501_DEBUG_WITHOUT_TRACING */
}

void soc_early_init_hook(void)
{
	uint32_t isave;


	isave = __get_PRIMASK();
	__disable_irq();

	soc_ecia_init();

	/* Configure GPIO bank before usage
	 * VTR1 is not configurable
	 * VTR2 doesn't need configuration if setting VTR2_STRAP
	 */
#ifdef CONFIG_SOC_MEC1501_VTR3_1_8V
	ECS_REGS->GPIO_BANK_PWR |= MCHP_ECS_VTR3_LVL_18;
#endif

	configure_debug_interface();

	if (!isave) {
		__enable_irq();
	}
}
