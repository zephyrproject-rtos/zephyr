/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <xtensa/xtruntime.h>
#include <zephyr/irq_nextlevel.h>
#include <xtensa/hal.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <adsp_shim.h>
#include <cavs-idc.h>
#include <adsp_interrupt.h>
#include "soc.h"

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define CAVS_INTC_NODE(n) DT_INST(n, intel_cavs_intc)

void z_soc_irq_enable(uint32_t irq)
{
	const struct device *dev_cavs;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_IRQN(CAVS_INTC_NODE(0)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(0));
		break;
	case DT_IRQN(CAVS_INTC_NODE(1)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(1));
		break;
	case DT_IRQN(CAVS_INTC_NODE(2)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(2));
		break;
	case DT_IRQN(CAVS_INTC_NODE(3)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(3));
		break;
	default:
		/* regular interrupt */
		xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq));
		return;
	}

	if (!device_is_ready(dev_cavs)) {
		LOG_DBG("board: CAVS device is not ready");
		return;
	}

	/*
	 * The specified interrupt is in CAVS interrupt controller.
	 * So enable core interrupt first.
	 */
	xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq));

	/* Then enable the interrupt in CAVS interrupt controller */
	irq_enable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));
}

void z_soc_irq_disable(uint32_t irq)
{
	const struct device *dev_cavs;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_IRQN(CAVS_INTC_NODE(0)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(0));
		break;
	case DT_IRQN(CAVS_INTC_NODE(1)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(1));
		break;
	case DT_IRQN(CAVS_INTC_NODE(2)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(2));
		break;
	case DT_IRQN(CAVS_INTC_NODE(3)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(3));
		break;
	default:
		/* regular interrupt */
		xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
		return;
	}

	if (!device_is_ready(dev_cavs)) {
		LOG_DBG("board: CAVS device is not ready");
		return;
	}

	/*
	 * The specified interrupt is in CAVS interrupt controller.
	 * So disable the interrupt in CAVS interrupt controller.
	 */
	irq_disable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

	/* Then disable the parent IRQ if all children are disabled */
	if (!irq_is_enabled_next_level(dev_cavs)) {
		xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
	}
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	const struct device *dev_cavs;
	int ret = 0;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_IRQN(CAVS_INTC_NODE(0)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(0));
		break;
	case DT_IRQN(CAVS_INTC_NODE(1)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(1));
		break;
	case DT_IRQN(CAVS_INTC_NODE(2)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(2));
		break;
	case DT_IRQN(CAVS_INTC_NODE(3)):
		dev_cavs = DEVICE_DT_GET(CAVS_INTC_NODE(3));
		break;
	default:
		/* regular interrupt */
		ret = xtensa_irq_is_enabled(XTENSA_IRQ_NUMBER(irq));
		goto out;
	}

	if (!device_is_ready(dev_cavs)) {
		LOG_DBG("board: CAVS device is not ready");
		ret = -ENODEV;
		goto out;
	}

	/* Then check the interrupt in CAVS interrupt controller */
	ret = irq_line_is_enabled_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

out:
	return ret;
}
