/*
 * Copyright (c) 2022 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_smc

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_sam, CONFIG_MEMC_LOG_LEVEL);

struct memc_smc_bank_config {
	uint32_t cs;
	uint32_t mode;
	uint32_t setup_timing;
	uint32_t pulse_timing;
	uint32_t cycle_timing;
};

struct memc_smc_config {
	Smc *regs;
	uint32_t periph_id;

	size_t banks_len;
	const struct memc_smc_bank_config *banks;

	const struct pinctrl_dev_config *pcfg;
};

static int memc_smc_init(const struct device *dev)
{
	int ret;
	const struct memc_smc_config *cfg = dev->config;
	SmcCs_number *bank;

	soc_pmc_peripheral_enable(cfg->periph_id);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0U; i < cfg->banks_len; i++) {
		if (cfg->banks[i].cs >= SMCCS_NUMBER_NUMBER) {
			return -EINVAL;
		}

		bank = &cfg->regs->SMC_CS_NUMBER[cfg->banks[i].cs];

		bank->SMC_SETUP = cfg->banks[i].setup_timing;
		bank->SMC_PULSE = cfg->banks[i].pulse_timing;
		bank->SMC_CYCLE = cfg->banks[i].cycle_timing;
		bank->SMC_MODE = cfg->banks[i].mode;
	}

	return 0;
}

#define SETUP_TIMING(node_id)								\
	SMC_SETUP_NWE_SETUP(DT_PROP_BY_IDX(node_id, atmel_smc_setup_timing, 0))		\
	| SMC_SETUP_NCS_WR_SETUP(DT_PROP_BY_IDX(node_id, atmel_smc_setup_timing, 1))	\
	| SMC_SETUP_NRD_SETUP(DT_PROP_BY_IDX(node_id, atmel_smc_setup_timing, 2))	\
	| SMC_SETUP_NCS_RD_SETUP(DT_PROP_BY_IDX(node_id, atmel_smc_setup_timing, 3))
#define PULSE_TIMING(node_id)								\
	SMC_PULSE_NWE_PULSE(DT_PROP_BY_IDX(node_id, atmel_smc_pulse_timing, 0))		\
	| SMC_PULSE_NCS_WR_PULSE(DT_PROP_BY_IDX(node_id, atmel_smc_pulse_timing, 1))	\
	| SMC_PULSE_NRD_PULSE(DT_PROP_BY_IDX(node_id, atmel_smc_pulse_timing, 2))	\
	| SMC_PULSE_NCS_RD_PULSE(DT_PROP_BY_IDX(node_id, atmel_smc_pulse_timing, 3))
#define CYCLE_TIMING(node_id)								\
	SMC_CYCLE_NWE_CYCLE(DT_PROP_BY_IDX(node_id, atmel_smc_cycle_timing, 0))		\
	| SMC_CYCLE_NRD_CYCLE(DT_PROP_BY_IDX(node_id, atmel_smc_cycle_timing, 1))

#define BANK_CONFIG(node_id)								\
	{										\
		.cs = DT_REG_ADDR(node_id),						\
		.mode = COND_CODE_1(DT_ENUM_IDX(node_id, atmel_smc_write_mode),		\
				    (SMC_MODE_WRITE_MODE), (0))				\
			| COND_CODE_1(DT_ENUM_IDX(node_id, atmel_smc_read_mode),	\
				      (SMC_MODE_READ_MODE), (0)),			\
		.setup_timing = SETUP_TIMING(node_id),					\
		.pulse_timing = PULSE_TIMING(node_id),					\
		.cycle_timing = CYCLE_TIMING(node_id),					\
	},

#define MEMC_SMC_DEFINE(inst)								\
	static const struct memc_smc_bank_config smc_bank_config_##inst[] = {		\
		DT_INST_FOREACH_CHILD(inst, BANK_CONFIG)				\
	};										\
	PINCTRL_DT_INST_DEFINE(inst);							\
	static const struct memc_smc_config smc_config_##inst = {			\
		.regs = (Smc *)DT_INST_REG_ADDR(inst),					\
		.periph_id = DT_INST_PROP(inst, peripheral_id),				\
		.banks_len = ARRAY_SIZE(smc_bank_config_##inst),			\
		.banks = smc_bank_config_##inst,					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
	};										\
	DEVICE_DT_INST_DEFINE(inst, memc_smc_init, NULL, NULL,				\
			      &smc_config_##inst, POST_KERNEL,				\
			      CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_SMC_DEFINE)
