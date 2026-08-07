/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_wuc

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it8xxx2.h>
#include <zephyr/dt-bindings/interrupt-controller/it8xxx2-wuc.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <soc_common.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wuc_ite_it8xxx2, CONFIG_INTC_LOG_LEVEL);

/* Driver config */
struct it8xxx2_wuc_cfg {
	/* WUC wakeup edge mode register */
	uint8_t *reg_wuemr;
	/* WUC wakeup edge sense register */
	uint8_t *reg_wuesr;
	/* WUC wakeup enable register */
	uint8_t *reg_wuenr;
	/* WUC wakeup both edge mode register */
	uint8_t *reg_wubemr;
};

void it8xxx2_wuc_enable(const struct device *dev, uint8_t mask)
{
	const struct it8xxx2_wuc_cfg *config = dev->config;
	volatile uint8_t *reg_wuenr = config->reg_wuenr;

	/*
	 * WUC group only 1, 3, and 4 have enable/disable register,
	 * others are always enabled.
	 */
	if (reg_wuenr == IT8XXX2_WUC_UNUSED_REG) {
		return;
	}

	/* Enable wakeup interrupt of the pin */
	*reg_wuenr |= mask;
}

void it8xxx2_wuc_disable(const struct device *dev, uint8_t mask)
{
	const struct it8xxx2_wuc_cfg *config = dev->config;
	volatile uint8_t *reg_wuenr = config->reg_wuenr;

	/*
	 * WUC group only 1, 3, and 4 have enable/disable register,
	 * others are always enabled.
	 */
	if (reg_wuenr == IT8XXX2_WUC_UNUSED_REG) {
		return;
	}

	/* Disable wakeup interrupt of the pin */
	*reg_wuenr &= ~mask;
}

void it8xxx2_wuc_clear_status(const struct device *dev, uint8_t mask)
{
	const struct it8xxx2_wuc_cfg *config = dev->config;
	volatile uint8_t *reg_wuesr = config->reg_wuesr;

	if (reg_wuesr == IT8XXX2_WUC_UNUSED_REG) {
		return;
	}

	/* W/C wakeup interrupt status of the pin */
	*reg_wuesr = mask;
}

void it8xxx2_wuc_set_polarity(const struct device *dev, uint8_t mask, uint32_t flags)
{
	const struct it8xxx2_wuc_cfg *config = dev->config;
	volatile uint8_t *reg_wuemr = config->reg_wuemr;
	volatile uint8_t *reg_wubemr = config->reg_wubemr;

	if (reg_wuemr == IT8XXX2_WUC_UNUSED_REG) {
		return;
	}

	/* Set wakeup interrupt edge trigger mode of the pin */
	if ((flags & WUC_TYPE_EDGE_BOTH) == WUC_TYPE_EDGE_RISING) {
		*reg_wubemr &= ~mask;
		*reg_wuemr &= ~mask;
	} else if ((flags & WUC_TYPE_EDGE_BOTH) == WUC_TYPE_EDGE_FALLING) {
		*reg_wubemr &= ~mask;
		*reg_wuemr |= mask;
	} else {
		/* Both edge trigger mode */
		*reg_wubemr |= mask;
	}
}

#define IT8XXX2_WUC_INIT(inst)						       \
									       \
	static const struct it8xxx2_wuc_cfg it8xxx2_wuc_cfg_##inst = {	       \
		.reg_wuemr = (uint8_t *) DT_INST_REG_ADDR_BY_IDX(inst, 0),     \
		.reg_wuesr = (uint8_t *) DT_INST_REG_ADDR_BY_IDX(inst, 1),     \
		.reg_wuenr = (uint8_t *) DT_INST_REG_ADDR_BY_IDX(inst, 2),     \
		.reg_wubemr = (uint8_t *) DT_INST_REG_ADDR_BY_IDX(inst, 3),    \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(inst,					       \
			      NULL,					       \
			      NULL,					       \
			      NULL,					       \
			      &it8xxx2_wuc_cfg_##inst,			       \
			      PRE_KERNEL_1,				       \
			      CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,	       \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(IT8XXX2_WUC_INIT)
