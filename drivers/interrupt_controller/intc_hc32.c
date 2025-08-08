/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for interrupt/event controller in HC32 MCUs
 */

#define DT_DRV_COMPAT           xhsc_hc32_intc

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <errno.h>

#define LOG_LEVEL CONFIG_INTC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intc_hc32);


int hc32_intc_irq_signin(int irqn, int intsrc)
{
	stc_irq_signin_config_t stcIrqSignConfig;

	stcIrqSignConfig.enIRQn      = (IRQn_Type)irqn;
	stcIrqSignConfig.enIntSrc    = (en_int_src_t)intsrc;
	stcIrqSignConfig.pfnCallback = NULL;
	if (LL_OK != INTC_IrqSignIn(&stcIrqSignConfig)) {
		LOG_ERR("intc signin failed!");
		return -EACCES;
	}

	return 0;
}

int hc32_intc_irq_signout(int irqn)
{
	if (LL_OK != INTC_IrqSignOut((IRQn_Type)irqn)) {
		LOG_ERR("intc signout failed!");
		return -EACCES;
	}

	return 0;
}
