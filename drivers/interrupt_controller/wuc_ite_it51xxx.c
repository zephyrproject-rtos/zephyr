/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_wuc

#include <soc.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it51xxx.h>
#include <zephyr/dt-bindings/interrupt-controller/ite-it51xxx-wuc.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(wuc_ite_it51xxx, CONFIG_INTC_LOG_LEVEL);

/* Driver config */
struct it51xxx_wuc_cfg {
	/* WUC wakeup edge mode register */
	uint8_t *reg_wuemr;
	/* WUC wakeup edge sense register */
	uint8_t *reg_wuesr;
	/* WUC wakeup enable register */
	uint8_t *reg_wuenr;
	/* WUC level or edge mode register */
	uint8_t *reg_wuler;
#if CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW
	uint8_t *reg_wuedr;
#else
	bool both_edge_trigger;
#endif /* CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW */
	bool wakeup_ctrl;
};

void it51xxx_wuc_enable(const struct device *dev, uint8_t mask)
{
	const struct it51xxx_wuc_cfg *config = dev->config;
	volatile uint8_t *reg_wuenr = config->reg_wuenr;

	if (!config->wakeup_ctrl) {
		LOG_ERR("Wakeup control(enable) is not supported.");
		return;
	}
	/*
	 * WUC group only 1, 3, and 4 have enable/disable register,
	 * others are always enabled.
	 */
	if (reg_wuenr == IT51XXX_WUC_UNUSED_REG) {
		return;
	}

	/* Enable wakeup interrupt of the pin */
	*reg_wuenr |= mask;
}

void it51xxx_wuc_disable(const struct device *dev, uint8_t mask)
{
	const struct it51xxx_wuc_cfg *config = dev->config;
	volatile uint8_t *reg_wuenr = config->reg_wuenr;

	if (!config->wakeup_ctrl) {
		LOG_ERR("Wakeup control(disable) is not supported.");
		return;
	}
	/*
	 * WUC group only 1, 3, and 4 have enable/disable register,
	 * others are always enabled.
	 */
	if (reg_wuenr == IT51XXX_WUC_UNUSED_REG) {
		return;
	}

	/* Disable wakeup interrupt of the pin */
	*reg_wuenr &= ~mask;
}

void it51xxx_wuc_clear_status(const struct device *dev, uint8_t mask)
{
	const struct it51xxx_wuc_cfg *config = dev->config;
	volatile uint8_t *reg_wuesr = config->reg_wuesr;

	if (!config->wakeup_ctrl) {
		LOG_ERR("Wakeup control of clear status is not supported.");
		return;
	}

	if (reg_wuesr == IT51XXX_WUC_UNUSED_REG) {
		return;
	}

	/* W/C wakeup interrupt status of the pin */
	*reg_wuesr = mask;
}

void it51xxx_wuc_set_polarity(const struct device *dev, uint8_t mask, uint32_t flags)
{
	const struct it51xxx_wuc_cfg *config = dev->config;
	volatile uint8_t *reg_wuemr = config->reg_wuemr;
	volatile uint8_t *reg_wuler = config->reg_wuler;
	volatile uint8_t *reg_wuesr = config->reg_wuesr;
#if CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW
	volatile uint8_t *reg_wuedr = config->reg_wuedr;
#endif /* CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW */

	if (!config->wakeup_ctrl) {
		LOG_ERR("Wakeup control of set polarity is not supported.");
		return;
	}

	if (reg_wuemr == IT51XXX_WUC_UNUSED_REG) {
		return;
	}

	if (flags & WUC_TYPE_LEVEL_TRIG) {
		*reg_wuler |= mask;
		if (flags & WUC_TYPE_LEVEL_HIGH) {
			*reg_wuemr &= ~mask;
		} else {
			*reg_wuemr |= mask;
		}
	} else {
		*reg_wuler &= ~mask;

		switch (flags & WUC_TYPE_EDGE_BOTH) {
		case WUC_TYPE_EDGE_RISING:
#if CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW
			*reg_wuedr |= mask;
#endif /* CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW */
			*reg_wuemr &= ~mask;
			break;
		case WUC_TYPE_EDGE_FALLING:
#if CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW
			*reg_wuedr |= mask;
#else
			if (config->both_edge_trigger) {
				LOG_WRN("Group 7, 10, 12 do not support falling edge mode.");
			}
#endif /* CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW */
			*reg_wuemr |= mask;
			break;
		case WUC_TYPE_EDGE_BOTH:
#if CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW
			*reg_wuedr &= ~mask;
#else
			if (!config->both_edge_trigger) {
				LOG_WRN("Group 7, 10, 12 do not support falling edge mode.");
			}
			*reg_wuemr |= mask;
#endif /* CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW */
			break;
		default:
			LOG_ERR("unknown trigger mode 0x%x", flags);
			break;
		}
	}
	/* W/C wakeup interrupt status of the pin */
	*reg_wuesr = mask;
}

/* clang-format off */
#define INST_HAS_EITHER_EDGE(n) \
	+DT_INST_PROP(n, hw_either_edge_supported)

#define NUM_EITHER_EDGE (0 DT_INST_FOREACH_STATUS_OKAY(INST_HAS_EITHER_EDGE))

BUILD_ASSERT((NUM_EITHER_EDGE == 0) || (NUM_EITHER_EDGE == DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)),
	     "Inconsistent hw-either-edge-supported configuration");

#define IT51XXX_EDGE_TRIGGER_CFG_INIT(inst)                                               \
	COND_CODE_1(CONFIG_ITE_IT51XXX_WUC_EITHER_EDGE_HW,                                 \
		(.reg_wuedr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 4),),               \
		(.both_edge_trigger = DT_INST_PROP(inst, both_edge_trigger),))

#define IT51XXX_WUC_INIT(inst)                                                                     \
	static const struct it51xxx_wuc_cfg it51xxx_wuc_cfg_##inst = {                             \
		.reg_wuemr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 0),                          \
		.reg_wuesr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 1),                          \
		.reg_wuenr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 2),                          \
		.reg_wuler = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 3),                          \
		.wakeup_ctrl = DT_INST_PROP(inst, wakeup_controller),                              \
		IT51XXX_EDGE_TRIGGER_CFG_INIT(inst)                                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &it51xxx_wuc_cfg_##inst, PRE_KERNEL_1,       \
			      CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, NULL);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(IT51XXX_WUC_INIT)
