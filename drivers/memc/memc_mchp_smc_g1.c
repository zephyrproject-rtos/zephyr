/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_smc_g1_memc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/drivers/memc/mchp_smc_g1.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_mchp_smc_g1, CONFIG_MEMC_LOG_LEVEL);

struct memc_smc_config {
	smc_registers_t *regs;
	struct sam_clk_cfg clk_cfg;
};

struct memc_smc_data {
	clock_control_subsys_t mck_cfg;
};

static int encode_ncycles(uint32_t ncycles,
			  uint32_t msbpos,
			  uint32_t msbmask,
			  uint32_t msbfactor,
			  uint32_t *val)
{
	uint32_t lsbmask = (1 << msbpos) - 1;
	uint32_t msb = ncycles / msbfactor;
	uint32_t lsb = ncycles % msbfactor;

	if (lsb > lsbmask) {
		lsb = 0;
		msb++;
	}

	if (msb > msbmask) {
		LOG_ERR("SMC: ncycles %d is out of range, msb=0x%x msbmask=0x%x",
			ncycles, msb, msbmask);
		return -ERANGE;
	}

	*val = (msb << msbpos) | lsb;

	return 0;
}

void smc_cs_conf_init(struct cs_config *cs)
{
	memset(cs, 0, sizeof(*cs));
}

int smc_cs_conf_set_setup(struct cs_config *cs, uint32_t shift, uint32_t ncycles)
{
	uint32_t val;
	int ret;

	if ((shift != SMC(SETUP_NWE_SETUP_Pos)) &&
	    (shift != SMC(SETUP_NCS_WR_SETUP_Pos)) &&
	    (shift != SMC(SETUP_NRD_SETUP_Pos)) &&
	    (shift != SMC(SETUP_NCS_RD_SETUP_Pos))) {
		LOG_ERR("SMC: Invalid shift %d in setup register", shift);
		return -EINVAL;
	}

	ret = encode_ncycles(ncycles, 5, 1, 128, &val);
	if (ret) {
		LOG_ERR("SMC: Failed to set setup ret=%d, shift=%d ncycles=%d",
			ret, shift, ncycles);
		return ret;
	}

	cs->setup &= ~(SMC(SETUP_NWE_SETUP_Msk) << shift);
	cs->setup |= val << shift;

	return 0;
}

int smc_cs_conf_set_pulse(struct cs_config *cs, uint32_t shift, uint32_t ncycles)
{
	uint32_t val;
	int ret;

	if ((shift != SMC(PULSE_NWE_PULSE_Pos)) &&
	    (shift != SMC(PULSE_NCS_WR_PULSE_Pos)) &&
	    (shift != SMC(PULSE_NRD_PULSE_Pos)) &&
	    (shift != SMC(PULSE_NCS_RD_PULSE_Pos))) {
		LOG_ERR("SMC: Invalid shift %d in pulse register", shift);
		return -EINVAL;
	}

	ret = encode_ncycles(ncycles, 6, 1, 256, &val);
	if (ret) {
		LOG_ERR("SMC: Failed to set pulse ret=%d, shift=%d ncycles=%d",
			ret, shift, ncycles);
		return ret;
	}

	cs->pulse &= ~(SMC(PULSE_NWE_PULSE_Msk) << shift);
	cs->pulse |= val << shift;

	return 0;
}

int smc_cs_conf_set_cycle(struct cs_config *cs, uint32_t shift, uint32_t ncycles)
{
	uint32_t val;
	int ret;

	if ((shift != SMC(CYCLE_NWE_CYCLE_Pos)) &&
	    (shift != SMC(CYCLE_NRD_CYCLE_Pos))) {
		LOG_ERR("SMC: Invalid shift %d in cycle register", shift);
		return -EINVAL;
	}

	ret = encode_ncycles(ncycles, 7, 3, 256, &val);
	if (ret) {
		LOG_ERR("SMC: Failed to set cycle ret=%d, shift=%d ncycles=%d",
			ret, shift, ncycles);
		return ret;
	}

	cs->cycle &= ~(SMC(CYCLE_NWE_CYCLE_Msk) << shift);
	cs->cycle |= val << shift;

	return 0;
}

#ifdef CONFIG_MEMC_MCHP_HSMC_G1
int smc_cs_conf_set_timing(struct cs_config *cs, uint32_t shift, uint32_t ncycles)
{
	uint32_t val;
	int ret;

	if ((shift != SMC(TIMINGS_TCLR_Pos)) &&
	    (shift != SMC(TIMINGS_TADL_Pos)) &&
	    (shift != SMC(TIMINGS_TAR_Pos)) &&
	    (shift != SMC(TIMINGS_TRR_Pos)) &&
	    (shift != SMC(TIMINGS_TWB_Pos))) {
		LOG_ERR("SMC: Invalid shift %d in timing register", shift);
		return -EINVAL;
	}

	ret = encode_ncycles(ncycles, 3, 1, 64, &val);
	if (ret) {
		LOG_ERR("SMC: Failed to set timing ret=%d, shift=%d ncycles=%d",
			ret, shift, ncycles);
		return ret;
	}

	cs->timings &= ~(SMC(TIMINGS_TCLR_Msk) << shift);
	cs->timings |= val << shift;

	return 0;
}
#endif

int smc_cs_conf_apply(const struct device *dev, const struct cs_config *cs)
{
	const struct memc_smc_config *cfg = dev->config;
	smc_registers_t *smc = cfg->regs;

	if (cs->cs >= SMC_CS_NUMBER_NUMBER) {
		return -EINVAL;
	}

	LOG_DBG("SMC: Apply CS %d config:", cs->cs);
	LOG_DBG("SMC: setup   = 0x%08x", cs->setup);
	LOG_DBG("SMC: pulse   = 0x%08x", cs->pulse);
	LOG_DBG("SMC: cycle   = 0x%08x", cs->cycle);
	LOG_DBG("SMC: timings = 0x%08x", cs->timings);
	LOG_DBG("SMC: mode    = 0x%08x", cs->mode);

	smc->SMC_CS_NUMBER[cs->cs].SMC(SETUP)   = cs->setup;
	smc->SMC_CS_NUMBER[cs->cs].SMC(PULSE)   = cs->pulse;
	smc->SMC_CS_NUMBER[cs->cs].SMC(CYCLE)   = cs->cycle;
#ifdef CONFIG_MEMC_MCHP_HSMC_G1
	smc->SMC_CS_NUMBER[cs->cs].SMC(TIMINGS) = cs->timings;
#else
	if (cs->timings) {
		LOG_WRN("SMC: CS timings could not be applied to SMC.");
	}
#endif
	smc->SMC_CS_NUMBER[cs->cs].SMC(MODE)    = cs->mode;

	return 0;
}

int smc_set_mck_cfg(const struct device *dev, clock_control_subsys_t cfg)
{
	struct memc_smc_data *data = dev->data;

	if (!cfg) {
		return -EINVAL;
	}

	data->mck_cfg = cfg;

	return 0;
}

int smc_get_mck_rate(const struct device *dev, uint32_t *rate)
{
	struct memc_smc_data *data = dev->data;
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	int ret;

	ret = clock_control_get_rate(pmc, data->mck_cfg, rate);
	if (ret) {
		return ret;
	}

	return 0;
}

static int memc_smc_init(const struct device *dev)
{
#ifdef CONFIG_MEMC_MCHP_HSMC_G1
	const struct memc_smc_config *cfg = dev->config;
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	int ret;

	if (!device_is_ready(pmc)) {
		LOG_ERR("SMC: Power Management Controller device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(pmc, (clock_control_subsys_t)(uintptr_t)&cfg->clk_cfg);
	if (ret) {
		LOG_ERR("SMC: Clock op failed");
		return ret;
	}
#endif

	return 0;
}

#ifdef CONFIG_MEMC_MCHP_HSMC_G1
#define SMC_DT_INST_CLOCK_GET(inst) SAM_DT_INST_CLOCK_PMC_CFG(inst)
#else
#define SMC_DT_INST_CLOCK_GET(inst) {0}
#endif

#define MEMC_SMC_DEFINE(inst)						\
	static const struct memc_smc_config smc_config_##inst = {	\
		.regs    = (smc_registers_t *)DT_INST_REG_ADDR(inst),	\
		.clk_cfg = SMC_DT_INST_CLOCK_GET(inst),			\
	};								\
									\
	static struct memc_smc_data smc_data_##inst = {0};		\
									\
	DEVICE_DT_INST_DEFINE(inst, memc_smc_init, NULL,		\
			      &smc_data_##inst,				\
			      &smc_config_##inst, POST_KERNEL,		\
			      CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_SMC_DEFINE)
