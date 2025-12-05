/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_wuu

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include <fsl_wuu.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/drivers/wuc/nxp_wuu.h>

LOG_MODULE_REGISTER(mcux_wuu, CONFIG_WUC_LOG_LEVEL);

struct mcux_wuu_config {
	WUU_Type *base;
	uint32_t irqn;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_wuu_data {
	uint32_t triggered_pin_sources;
	struct k_spinlock lock;
};

/** Union of WUU wakeup event types */
union wuu_wakeup_event {
	/** Pin event type */
	wuu_external_wakeup_pin_event_t pin_event;
	/** Module event type */
	wuu_internal_wakeup_module_event_t module_event;
};

struct wakeup_source_detail {
	int8_t type;
	uint8_t index;
	wuu_external_pin_edge_detection_t edge;
	union wuu_wakeup_event event;
} __packed;

const char *module_event_name[] = {
	"Interrupt",
	"DMA Trigger",
};

const char *pin_edge_name[] = {
	"Disabled",
	"Rising Edge",
	"Falling Edge",
	"Any Edge",
};

const char *pin_event_name[] = {
	"Interrupt",
	"DMA Request",
	"Trigger",
};

static void mcux_wuu_isr(const struct device *dev)
{
	struct mcux_wuu_config *config = (struct mcux_wuu_config *)dev->config;
	struct mcux_wuu_data *data = (struct mcux_wuu_data *)dev->data;

	data->triggered_pin_sources = WUU_GetExternalWakeUpPinsFlag(config->base);
	WUU_ClearExternalWakeUpPinsFlag(config->base, data->triggered_pin_sources);
}

static void mcux_wuu_decode_source(struct wakeup_source_detail *detail, uint32_t source)
{
	memset((void *)detail, 0, sizeof(struct wakeup_source_detail));

	detail->type = NXP_WUU_WAKEUP_SOURCE_DECODE_TYPE(source);
	detail->index = NXP_WUU_WAKEUP_SOURCE_DECODE_INDEX(source);
	if (detail->type == NXP_WUU_SOURCE_TYPE_PIN) {
		detail->edge = (wuu_external_pin_edge_detection_t)NXP_WUU_WAKEUP_SOURCE_DECODE_EDGE(
			source);
		detail->event.pin_event =
			(wuu_external_wakeup_pin_event_t)NXP_WUU_WAKEUP_SOURCE_DECODE_EVENT(source);
	} else if (detail->type == NXP_WUU_SOURCE_TYPE_MODULE) {
		detail->event.module_event =
			(wuu_internal_wakeup_module_event_t)NXP_WUU_WAKEUP_SOURCE_DECODE_EVENT(
				source);
	}
}

int mcux_wuu_init(const struct device *dev)
{
	struct mcux_wuu_config *config = (struct mcux_wuu_config *)dev->config;

	config->irq_config_func(dev);

	return 0;
}

int mcux_wuu_enable_wakeup_source(const struct device *dev, uint32_t source)
{
	struct mcux_wuu_config *config = (struct mcux_wuu_config *)dev->config;
	struct wakeup_source_detail detail;
	int ret = 0;

	mcux_wuu_decode_source(&detail, source);

	if (detail.type == NXP_WUU_SOURCE_TYPE_MODULE) {
		WUU_SetInternalWakeUpModulesConfig(
			config->base, detail.index,
			(wuu_internal_wakeup_module_event_t)detail.event.module_event);
		LOG_DBG("WUU module %d, %s enabled as wakeup source", detail.index,
			module_event_name[(uint8_t)detail.event.module_event]);
	} else if (detail.type == NXP_WUU_SOURCE_TYPE_PIN) {
		wuu_external_wakeup_pin_config_t pin_config = {
			.edge = (wuu_external_pin_edge_detection_t)detail.edge,
			.event = (wuu_external_wakeup_pin_event_t)detail.event.pin_event,
			.mode = kWUU_ExternalPinActiveDSPD,
		};
		WUU_SetExternalWakeUpPinsConfig(config->base, detail.index, &pin_config);

		if (pin_config.event == kWUU_ExternalPinInterrupt) {
			/* Pin interrupt event requires IRQ to be enabled */
			irq_enable(config->irqn);
		}

		LOG_DBG("WUU external pin %d %s %s enabled as wakeup source", detail.index,
			pin_edge_name[(uint8_t)pin_config.edge],
			pin_event_name[(uint8_t)pin_config.event]);
	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

int mcux_wuu_disable_wakeup_source(const struct device *dev, uint32_t source)
{
	struct mcux_wuu_config *config = (struct mcux_wuu_config *)dev->config;
	struct wakeup_source_detail detail;
	int ret = 0;

	mcux_wuu_decode_source(&detail, source);

	if (detail.type == NXP_WUU_SOURCE_TYPE_MODULE) {
		WUU_ClearInternalWakeUpModulesConfig(
			config->base, detail.index,
			(wuu_internal_wakeup_module_event_t)detail.event.module_event);

		LOG_DBG("WUU module %d %s disabled as wakeup source", detail.index,
			module_event_name[(uint8_t)detail.event.module_event]);
	} else if (detail.type == NXP_WUU_SOURCE_TYPE_PIN) {
		WUU_ClearExternalWakeupPinsConfig(config->base, detail.index);
		if ((detail.event.pin_event == kWUU_ExternalPinInterrupt) &&
		    ((WUU_Type *)(config->base)->PE1 == 0) &&
		    ((WUU_Type *)(config->base)->PE2 == 0)) {
			/* Disable wuu interrupt, if none of pin interrupt events are enabled */
			irq_disable(config->irqn);
		}
		LOG_DBG("WUU external pin %d %s %s disabled as wakeup source", detail.index,
			pin_edge_name[(uint8_t)detail.edge],
			pin_event_name[(uint8_t)detail.event.pin_event]);
	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

int mcux_wuu_check_wakeup_triggered(const struct device *dev, uint32_t source)
{
#if (defined(FSL_FEATURE_WUU_HAS_MF) && FSL_FEATURE_WUU_HAS_MF)
	struct mcux_wuu_config *config = (struct mcux_wuu_config *)dev->config;
#endif
	struct mcux_wuu_data *data = (struct mcux_wuu_data *)dev->data;
	struct wakeup_source_detail detail;
	int ret = 0;

	mcux_wuu_decode_source(&detail, source);
	K_SPINLOCK(&data->lock) {
		if (detail.type == NXP_WUU_SOURCE_TYPE_MODULE) {
#if (defined(FSL_FEATURE_WUU_HAS_MF) && FSL_FEATURE_WUU_HAS_MF)
			ret = WUU_GetInternalWakeupModuleFlag(config->base, detail.index) ? 1 : 0;
#else
			ret = -ENOTSUP;
#endif
		} else if (detail.type == NXP_WUU_SOURCE_TYPE_PIN) {
			ret = (data->triggered_pin_sources & BIT(detail.index)) ? 1 : 0;
		} else {
			ret = -ENOTSUP;
		}
	}

	return ret;
}

#define MCUX_WUU_DEFINE(n)                                                                         \
	static void mcux_wuu_config_func_##n(const struct device *dev);                            \
	static struct mcux_wuu_config mcux_wuu_config_##n = {                                      \
		.base = (WUU_Type *)DT_INST_REG_ADDR(n),                                           \
		.irqn = DT_INST_IRQN(n),                                                           \
		.irq_config_func = mcux_wuu_config_func_##n,                                       \
	};                                                                                         \
	static struct mcux_wuu_data mcux_wuu_data_##n = {                                          \
		.triggered_pin_sources = 0,                                                        \
	};                                                                                         \
	static struct wuc_driver_api mcux_wuu_api_##n = {                                          \
		.enable_wakeup_source = mcux_wuu_enable_wakeup_source,                             \
		.disable_wakeup_source = mcux_wuu_disable_wakeup_source,                           \
		.check_wakeup_triggered = mcux_wuu_check_wakeup_triggered,                         \
	};                                                                                         \
	static void mcux_wuu_config_func_##n(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_wuu_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, mcux_wuu_init, NULL, &mcux_wuu_data_##n, &mcux_wuu_config_##n,    \
			      PRE_KERNEL_1, 1, &mcux_wuu_api_##n);

DT_INST_FOREACH_STATUS_OKAY(MCUX_WUU_DEFINE)
