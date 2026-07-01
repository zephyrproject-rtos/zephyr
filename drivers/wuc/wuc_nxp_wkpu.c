/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_wuc_wkpu

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include <fsl_wkpu.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/dt-bindings/wuc/wuc_nxp_wkpu.h>

LOG_MODULE_REGISTER(mcux_wkpu, CONFIG_WUC_LOG_LEVEL);

struct mcux_wkpu_config {
	WKPU_Type *base;
	uint32_t irqn;
};

struct mcux_wkpu_data {
	uint64_t triggered_sources;
	struct k_spinlock lock;
};

static void mcux_wkpu_isr(const struct device *dev)
{
	const struct mcux_wkpu_config *config = dev->config;
	struct mcux_wkpu_data *data = dev->data;
	uint64_t triggered;

	triggered = WKPU_GetExternalWakeUpSourceFlag(config->base);
	WKPU_ClearExternalWakeUpSourceFlag(config->base, triggered);

	K_SPINLOCK(&data->lock) {
		data->triggered_sources |= triggered;
	}
}

static int mcux_wkpu_enable_wakeup_source(const struct device *dev, uint32_t source)
{
	const struct mcux_wkpu_config *config = dev->config;
	uint8_t index = NXP_WKPU_WAKEUP_SOURCE_DECODE_INDEX(source);
	uint8_t edge = NXP_WKPU_WAKEUP_SOURCE_DECODE_EDGE(source);
	uint8_t event = NXP_WKPU_WAKEUP_SOURCE_DECODE_EVENT(source);
	wkpu_external_wakeup_source_config_t cfg = {
		.event = (event == NXP_WKPU_EVENT_INT_WAKEUP) ? kWKPU_InterruptAndWakeUp
							      : kWKPU_Interrupt,
		.edge = (wkpu_pin_edge_detection_t)edge,
		.enableFilter = false,
	};

	WKPU_SetExternalWakeUpSourceConfig(config->base, (uint64_t)1ULL << index, &cfg);

	/* A source delivering an interrupt event needs the controller IRQ. */
	irq_enable(config->irqn);

	LOG_DBG("WKPU source %u enabled (edge %u, event %u)", index, edge, event);

	return 0;
}

static int mcux_wkpu_disable_wakeup_source(const struct device *dev, uint32_t source)
{
	const struct mcux_wkpu_config *config = dev->config;
	uint8_t index = NXP_WKPU_WAKEUP_SOURCE_DECODE_INDEX(source);

	WKPU_ClearExternalWakeupSourceConfig(config->base, (uint64_t)1ULL << index);

	/* Disable the controller IRQ once no source requests an interrupt. */
	if ((config->base->IRER == 0U) && (config->base->IRER_64 == 0U)) {
		irq_disable(config->irqn);
	}

	LOG_DBG("WKPU source %u disabled", index);

	return 0;
}

static int mcux_wkpu_check_wakeup_source_triggered(const struct device *dev, uint32_t source)
{
	struct mcux_wkpu_data *data = dev->data;
	uint8_t index = NXP_WKPU_WAKEUP_SOURCE_DECODE_INDEX(source);
	int ret;

	K_SPINLOCK(&data->lock) {
		ret = (data->triggered_sources & ((uint64_t)1ULL << index)) ? 1 : 0;
	}

	return ret;
}

static int mcux_wkpu_clear_wakeup_source_triggered(const struct device *dev, uint32_t source)
{
	struct mcux_wkpu_data *data = dev->data;
	uint8_t index = NXP_WKPU_WAKEUP_SOURCE_DECODE_INDEX(source);

	K_SPINLOCK(&data->lock) {
		data->triggered_sources &= ~((uint64_t)1ULL << index);
	}

	return 0;
}

static int mcux_wkpu_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

static int mcux_wkpu_init(const struct device *dev)
{
	struct mcux_wkpu_data *data = dev->data;

	IRQ_CONNECT(DT_IRQN(DT_DRV_INST(0)), DT_IRQ(DT_DRV_INST(0), priority), mcux_wkpu_isr,
		    DEVICE_DT_INST_GET(0), 0);

	data->triggered_sources = 0;

	return pm_device_driver_init(dev, mcux_wkpu_pm_action);
}

static const struct mcux_wkpu_config wkpu_config = {
	.base = (WKPU_Type *)DT_REG_ADDR(DT_DRV_INST(0)),
	.irqn = DT_IRQN(DT_DRV_INST(0)),
};

static struct mcux_wkpu_data wkpu_data;

static const DEVICE_API(wuc, wkpu_api) = {
	.enable = mcux_wkpu_enable_wakeup_source,
	.disable = mcux_wkpu_disable_wakeup_source,
	.triggered = mcux_wkpu_check_wakeup_source_triggered,
	.clear = mcux_wkpu_clear_wakeup_source_triggered,
};

DEVICE_DT_INST_DEFINE(0, mcux_wkpu_init, NULL, &wkpu_data, &wkpu_config, PRE_KERNEL_1,
		      WUC_INIT_PRIORITY, &wkpu_api);
