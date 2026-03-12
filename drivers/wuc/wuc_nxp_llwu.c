/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_llwu

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/wuc.h>

#include <fsl_llwu.h>
#include <zephyr/dt-bindings/wuc/wuc_nxp_llwu.h>

LOG_MODULE_REGISTER(mcux_llwu, CONFIG_WUC_LOG_LEVEL);

struct mcux_llwu_config {
	LLWU_Type *base;
	uint32_t irqn;
};

struct mcux_llwu_data {
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN
	/** Bitmask of pin sources that have triggered a wakeup event */
	uint32_t triggered_pin_sources;
	/** Bitmask of currently enabled wakeup pins */
	uint32_t enabled_pins;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
	struct k_spinlock lock;
};

#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN
static void mcux_llwu_isr(const struct device *dev)
{
	const struct mcux_llwu_config *config = dev->config;
	struct mcux_llwu_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		for (int i = 0; i < 16; i++) {
			if (IS_BIT_SET(data->enabled_pins, i)) {
				if (LLWU_GetExternalWakeupPinFlag(config->base, i + 1) == true) {
					data->triggered_pin_sources |= BIT(i);
					LLWU_ClearExternalWakeupPinFlag(config->base, i + 1);
				}
			}
		}
	}
}
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */

static int mcux_llwu_enable_wakeup_source(const struct device *dev, uint32_t id)
{
	const struct mcux_llwu_config *config = dev->config;
	struct mcux_llwu_data *data = dev->data;
	int ret = -EINVAL;

	K_SPINLOCK(&data->lock) {
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN
		if (NXP_LLWU_IS_PIN_SOURCE(id)) {
			ret = 0;
			if (IS_BIT_SET(data->enabled_pins, (NXP_LLWU_GET_PIN_INDEX(id) - 1)) ==
			    false) {
				LLWU_SetExternalWakeupPinMode(
					config->base, NXP_LLWU_GET_PIN_INDEX(id),
					(llwu_external_pin_mode_t)NXP_LLWU_GET_PIN_EDGE(id));
				data->enabled_pins |= BIT((NXP_LLWU_GET_PIN_INDEX(id) - 1));
			}

			if (data->enabled_pins != 0) {
				irq_enable(config->irqn);
			}
		}
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */

#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE
		if (NXP_LLWU_IS_MODULE_SOURCE(id)) {
			ret = 0;
			LLWU_EnableInternalModuleInterruptWakup(
				config->base, NXP_LLWU_GET_MODULE_INDEX(id), true);
		}
#endif /* FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE */
	}

	return ret;
}

static int mcux_llwu_disable_wakeup_source(const struct device *dev, uint32_t id)
{
	const struct mcux_llwu_config *config = dev->config;
	struct mcux_llwu_data *data = dev->data;
	int ret = -EINVAL;

	K_SPINLOCK(&data->lock) {
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN
		if (NXP_LLWU_IS_PIN_SOURCE(id)) {
			ret = 0;
			if (IS_BIT_SET(data->enabled_pins, (NXP_LLWU_GET_PIN_INDEX(id) - 1)) ==
			    true) {
				LLWU_SetExternalWakeupPinMode(config->base,
							      NXP_LLWU_GET_PIN_INDEX(id),
							      kLLWU_ExternalPinDisable);
				data->enabled_pins &= ~BIT((NXP_LLWU_GET_PIN_INDEX(id) - 1));
			}

			if (data->enabled_pins == 0) {
				irq_disable(config->irqn);
			}
		}
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */

#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE
		if (NXP_LLWU_IS_MODULE_SOURCE(id)) {
			ret = 0;
			LLWU_EnableInternalModuleInterruptWakup(
				config->base, NXP_LLWU_GET_MODULE_INDEX(id), false);
		}
#endif /* FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE */
	}

	return ret;
}

static int mcux_llwu_check_wakeup_source_triggered(const struct device *dev, uint32_t id)
{
	struct mcux_llwu_data *data = dev->data;
	int ret = 0;

	K_SPINLOCK(&data->lock) {
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN
		if (NXP_LLWU_IS_PIN_SOURCE(id) &&
		    IS_BIT_SET(data->triggered_pin_sources, (NXP_LLWU_GET_PIN_INDEX(id) - 1))) {
			ret = 1;
		}
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
	}

	return ret;
}

static int mcux_llwu_clear_wakeup_source_triggered(const struct device *dev, uint32_t id)
{
	struct mcux_llwu_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN
		if (NXP_LLWU_IS_PIN_SOURCE(id)) {
			data->triggered_pin_sources &= ~BIT(NXP_LLWU_GET_PIN_INDEX(id) - 1);
		}
#endif
	}

	return 0;
}

static int mcux_llwu_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

static int mcux_llwu_init(const struct device *dev)
{
	struct mcux_llwu_data *data = dev->data;

#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN
	IRQ_CONNECT(DT_IRQN(DT_DRV_INST(0)), DT_IRQ(DT_DRV_INST(0), priority), mcux_llwu_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	data->triggered_pin_sources = 0;
	data->enabled_pins = 0;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */

	return pm_device_driver_init(dev, mcux_llwu_pm_action);
}

static const struct mcux_llwu_config llwu_config = {
	.base = (LLWU_Type *)DT_REG_ADDR(DT_DRV_INST(0)),
	.irqn = DT_IRQN(DT_DRV_INST(0)),
};

static struct mcux_llwu_data llwu_data;

static const DEVICE_API(wuc, mcux_llwu_api) = {
	.enable = mcux_llwu_enable_wakeup_source,
	.disable = mcux_llwu_disable_wakeup_source,
	.triggered = mcux_llwu_check_wakeup_source_triggered,
	.clear = mcux_llwu_clear_wakeup_source_triggered,
};

DEVICE_DT_INST_DEFINE(0, mcux_llwu_init, NULL, &llwu_data, &llwu_config, POST_KERNEL,
		      WUC_INIT_PRIORITY, &mcux_llwu_api);
