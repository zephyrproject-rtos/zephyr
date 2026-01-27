/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/drivers/wuc/wuc_ite.h>
#include <zephyr/dt-bindings/interrupt-controller/it8xxx2-wuc.h>
#include <zephyr/dt-bindings/interrupt-controller/ite-it51xxx-wuc.h>

LOG_MODULE_REGISTER(wuc_ite, CONFIG_WUC_LOG_LEVEL);

struct ite_wuc_config {
	uint8_t *reg_wuemr;
	uint8_t *reg_wuesr;
	uint8_t *reg_wuenr;
	uint8_t *reg_wubemr;
	uint8_t *reg_wuler;
	bool wakeup_ctrl;
	bool both_edge_trigger;
	bool supports_level;
};

__weak int ite_wuc_apply_priority(uint32_t priority)
{
	ARG_UNUSED(priority);
	return -ENOTSUP;
}

static void ite_wuc_set_polarity(const struct device *dev, uint8_t mask, uint32_t flags)
{
	const struct ite_wuc_config *config = dev->config;
	volatile uint8_t *reg_wuemr = config->reg_wuemr;
	volatile uint8_t *reg_wubemr = config->reg_wubemr;
	volatile uint8_t *reg_wuler = config->reg_wuler;
	volatile uint8_t *reg_wuesr = config->reg_wuesr;

	if (!config->wakeup_ctrl) {
		LOG_ERR("Wakeup control(set polarity) is not supported.");
		return;
	}

	if (reg_wuemr == (uint8_t *)IT8XXX2_WUC_UNUSED_REG ||
	    reg_wuemr == (uint8_t *)IT51XXX_WUC_UNUSED_REG) {
		return;
	}

	if (config->supports_level && (flags & ITE_WUC_FLAG_LEVEL_TRIG)) {
		if (reg_wuler != (uint8_t *)IT51XXX_WUC_UNUSED_REG) {
			*reg_wuler |= mask;
		}
		if (flags & ITE_WUC_FLAG_LEVEL_HIGH) {
			*reg_wuemr &= ~mask;
		} else {
			*reg_wuemr |= mask;
		}
	} else {
		if (reg_wuler != (uint8_t *)IT51XXX_WUC_UNUSED_REG) {
			*reg_wuler &= ~mask;
		}

		if ((flags & ITE_WUC_FLAG_EDGE_BOTH) == ITE_WUC_FLAG_EDGE_RISING) {
			if (reg_wubemr != (uint8_t *)IT8XXX2_WUC_UNUSED_REG) {
				*reg_wubemr &= ~mask;
			}
			*reg_wuemr &= ~mask;
		} else if ((flags & ITE_WUC_FLAG_EDGE_BOTH) == ITE_WUC_FLAG_EDGE_FALLING) {
			if (reg_wubemr != (uint8_t *)IT8XXX2_WUC_UNUSED_REG) {
				*reg_wubemr &= ~mask;
			}
			*reg_wuemr |= mask;
		} else {
			if (config->supports_level && !config->both_edge_trigger) {
				LOG_WRN("Both edge trigger mode not supported by %s", dev->name);
			}
			if (reg_wubemr != (uint8_t *)IT8XXX2_WUC_UNUSED_REG) {
				*reg_wubemr |= mask;
			} else {
				*reg_wuemr |= mask;
			}
		}
	}

	if (reg_wuesr != (uint8_t *)IT8XXX2_WUC_UNUSED_REG &&
	    reg_wuesr != (uint8_t *)IT51XXX_WUC_UNUSED_REG) {
		*reg_wuesr = mask;
	}
}

static int ite_wuc_enable_wakeup_source(const struct device *dev, uint32_t id)
{
	const struct ite_wuc_config *config = dev->config;
	volatile uint8_t *reg_wuenr = config->reg_wuenr;
	uint8_t mask = ITE_WUC_ID_DECODE_MASK(id);
	uint32_t flags = ITE_WUC_ID_DECODE_FLAGS(id);
	uint32_t priority = ITE_WUC_ID_DECODE_PRIORITY(id);

	if (mask == 0U) {
		return -EINVAL;
	}

	if (!config->wakeup_ctrl) {
		LOG_ERR("Wakeup control(enable) is not supported.");
		return -ENOTSUP;
	}

	if (flags != 0U) {
		ite_wuc_set_polarity(dev, mask, flags);
	}

	if (priority != 0U) {
		(void)ite_wuc_apply_priority(priority);
	}

	if (reg_wuenr == (uint8_t *)IT8XXX2_WUC_UNUSED_REG ||
	    reg_wuenr == (uint8_t *)IT51XXX_WUC_UNUSED_REG) {
		return 0;
	}

	*reg_wuenr |= mask;
	return 0;
}

static int ite_wuc_disable_wakeup_source(const struct device *dev, uint32_t id)
{
	const struct ite_wuc_config *config = dev->config;
	volatile uint8_t *reg_wuenr = config->reg_wuenr;
	uint8_t mask = ITE_WUC_ID_DECODE_MASK(id);

	if (mask == 0U) {
		return -EINVAL;
	}

	if (!config->wakeup_ctrl) {
		LOG_ERR("Wakeup control(disable) is not supported.");
		return -ENOTSUP;
	}

	if (reg_wuenr == (uint8_t *)IT8XXX2_WUC_UNUSED_REG ||
	    reg_wuenr == (uint8_t *)IT51XXX_WUC_UNUSED_REG) {
		return 0;
	}

	*reg_wuenr &= ~mask;
	return 0;
}

static int ite_wuc_check_wakeup_source_triggered(const struct device *dev, uint32_t id)
{
	const struct ite_wuc_config *config = dev->config;
	volatile uint8_t *reg_wuesr = config->reg_wuesr;
	uint8_t mask = ITE_WUC_ID_DECODE_MASK(id);

	if (mask == 0U) {
		return -EINVAL;
	}

	if (!config->wakeup_ctrl) {
		return -ENOTSUP;
	}

	if (reg_wuesr == (uint8_t *)IT8XXX2_WUC_UNUSED_REG ||
	    reg_wuesr == (uint8_t *)IT51XXX_WUC_UNUSED_REG) {
		return 0;
	}

	return ((*reg_wuesr & mask) != 0U) ? 1 : 0;
}

static int ite_wuc_clear_wakeup_source_triggered(const struct device *dev, uint32_t id)
{
	const struct ite_wuc_config *config = dev->config;
	volatile uint8_t *reg_wuesr = config->reg_wuesr;
	uint8_t mask = ITE_WUC_ID_DECODE_MASK(id);

	if (mask == 0U) {
		return -EINVAL;
	}

	if (!config->wakeup_ctrl) {
		return -ENOTSUP;
	}

	if (reg_wuesr == (uint8_t *)IT8XXX2_WUC_UNUSED_REG ||
	    reg_wuesr == (uint8_t *)IT51XXX_WUC_UNUSED_REG) {
		return 0;
	}

	*reg_wuesr = mask;
	return 0;
}

static int ite_wuc_record_wakeup_source_triggered(const struct device *dev, uint32_t id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	return -ENOTSUP;
}

static const struct wuc_driver_api ite_wuc_driver_api = {
	.enable_wakeup_source = ite_wuc_enable_wakeup_source,
	.disable_wakeup_source = ite_wuc_disable_wakeup_source,
	.check_wakeup_source_triggered = ite_wuc_check_wakeup_source_triggered,
	.clear_wakeup_source_triggered = ite_wuc_clear_wakeup_source_triggered,
	.record_wakeup_source_triggered = ite_wuc_record_wakeup_source_triggered,
};

#define ITE_WUC_IT8XXX2_INIT(inst)                                                             \
	static const struct ite_wuc_config ite_wuc_it8xxx2_cfg_##inst = {                         \
		.reg_wuemr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 0),                           \
		.reg_wuesr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 1),                           \
		.reg_wuenr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 2),                           \
		.reg_wubemr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 3),                          \
		.reg_wuler = (uint8_t *)IT8XXX2_WUC_UNUSED_REG,                                     \
		.wakeup_ctrl = true,                                                                \
		.both_edge_trigger = true,                                                          \
		.supports_level = false,                                                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &ite_wuc_it8xxx2_cfg_##inst,                \
			      PRE_KERNEL_1, CONFIG_WUC_INIT_PRIORITY, &ite_wuc_driver_api);

#define DT_DRV_COMPAT ite_it8xxx2_wuc
DT_INST_FOREACH_STATUS_OKAY(ITE_WUC_IT8XXX2_INIT)
#undef DT_DRV_COMPAT

#define ITE_WUC_IT51XXX_INIT(inst)                                                             \
	static const struct ite_wuc_config ite_wuc_it51xxx_cfg_##inst = {                         \
		.reg_wuemr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 0),                           \
		.reg_wuesr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 1),                           \
		.reg_wuenr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 2),                           \
		.reg_wubemr = (uint8_t *)IT51XXX_WUC_UNUSED_REG,                                     \
		.reg_wuler = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 3),                           \
		.wakeup_ctrl = DT_INST_PROP_OR(inst, wakeup_controller, false),                      \
		.both_edge_trigger = DT_INST_PROP_OR(inst, both_edge_trigger, false),                \
		.supports_level = true,                                                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &ite_wuc_it51xxx_cfg_##inst,                \
			      PRE_KERNEL_1, CONFIG_WUC_INIT_PRIORITY, &ite_wuc_driver_api);

#define DT_DRV_COMPAT ite_it51xxx_wuc
DT_INST_FOREACH_STATUS_OKAY(ITE_WUC_IT51XXX_INIT)
#undef DT_DRV_COMPAT
