/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_pinctrl_kscan_kbs

#include <pinctrl_kscan_soc.h>
#include <zephyr/drivers/pinctrl/pinctrl_kscan_it8xxx2.h>
#include <zephyr/dt-bindings/pinctrl/it8xxx2-pinctrl-kscan.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pinctrl_kscan_ite_it8xxx2, LOG_LEVEL_ERR);

struct pinctrl_kscan_it8xxx2_config {
	/* KSI[7:0]/KSO[15:8]/KSO[7:0] port gpio control register (bit mapping to pin) */
	uint8_t *reg_ksi_kso_gctrl;
	/* KSI[7:0]/KSO[15:0] control register */
	uint8_t *reg_ksi_kso_ctrl;
	/* KSO push-pull/open-drain bit of KSO[15:0] control register (one bit to all pins) */
	int pushpull_od_mask;
	/* KSI/KSO pullup bit of KSI[7:0]/KSO[15:0] control register (one bit to all pins) */
	int pullup_mask;
};

static int pinctrl_kscan_it8xxx2_set(const struct pinctrl_kscan_soc_pin *pins)
{
	const struct pinctrl_kscan_it8xxx2_config *const config = pins->pinctrls->config;
	volatile uint8_t *reg_ksi_kso_ctrl = config->reg_ksi_kso_ctrl;
	uint8_t pushpull_od_mask = config->pushpull_od_mask;
	uint8_t pullup_mask = config->pullup_mask;
	uint32_t pincfg = pins->pincfg;

	/* Enable or Disable internal pull-up */
	switch (IT8XXX2_PINCTRL_KSCAN_DT_PINCFG_PULLUP(pincfg)) {
	case IT8XXX2_KSI_KSO_NOT_PULL_UP:
		*reg_ksi_kso_ctrl &= ~pullup_mask;
		break;
	case IT8XXX2_KSI_KSO_PULL_UP:
		*reg_ksi_kso_ctrl |= pullup_mask;
		break;
	default:
		LOG_ERR("This pull level is not supported.");
		return -EINVAL;
	}

	/* Set push-pull or open-drain mode (except KSI pins) */
	if (pushpull_od_mask != IT8XXX2_PINCTRL_KSCAN_NOT_SUPPORT_PP_OD) {
		switch (IT8XXX2_PINCTRL_KSCAN_DT_PINCFG_PP_OD(pincfg)) {
		case IT8XXX2_KSI_KSO_PUSH_PULL:
			*reg_ksi_kso_ctrl &= ~pushpull_od_mask;
			break;
		case IT8XXX2_KSI_KSO_OPEN_DRAIN:
			*reg_ksi_kso_ctrl |= pushpull_od_mask;
			break;
		default:
			LOG_ERR("This pull mode is not supported.");
			return -EINVAL;
		}
	}

	return 0;
}

int pinctrl_kscan_configure_pins(const struct pinctrl_kscan_soc_pin *pins,
				 uint8_t pin_cnt)
{
	const struct pinctrl_kscan_it8xxx2_config *config;
	volatile uint8_t *reg_ksi_kso_gctrl;
	uint8_t pin_mask;

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		config = pins[i].pinctrls->config;
		reg_ksi_kso_gctrl = config->reg_ksi_kso_gctrl;
		pin_mask = BIT(pins[i].pin);

		/* Set a pin of KSI[7:0]/KSO[15:0] to pullup, push-pull/open-drain */
		if (pinctrl_kscan_it8xxx2_set(&pins[i])) {
			LOG_ERR("Pin configuration is invalid.");
			return -EINVAL;
		}

		/* Set a pin of KSI[7:0]/KSO[15:0] to kbs mode */
		*reg_ksi_kso_gctrl &= ~pin_mask;
	}

	return 0;
}

static int pinctrl_kscan_it8xxx2_init(const struct device *dev)
{
	return 0;
}

#define PINCTRL_KSCAN_ITE_INIT(inst)                                                  \
static const struct pinctrl_kscan_it8xxx2_config pinctrl_kscan_it8xxx2_cfg_##inst = { \
	.reg_ksi_kso_gctrl = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 0),             \
	.reg_ksi_kso_ctrl = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 1),              \
	.pushpull_od_mask = (uint8_t)DT_INST_PROP(inst, pushpull_od_mask),            \
	.pullup_mask = (uint8_t)DT_INST_PROP(inst, pullup_mask),                      \
};                                                                                    \
										      \
DEVICE_DT_INST_DEFINE(inst,                                                           \
		      &pinctrl_kscan_it8xxx2_init,                                    \
		      NULL,                                                           \
		      NULL,                                                           \
		      &pinctrl_kscan_it8xxx2_cfg_##inst,                              \
		      PRE_KERNEL_1,                                                   \
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                            \
		      NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_KSCAN_ITE_INIT)
