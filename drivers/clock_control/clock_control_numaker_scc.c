/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_scc

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/logging/log.h>
#include <NuMicro.h>

LOG_MODULE_REGISTER(clock_control_numaker_scc, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct numaker_scc_config {
	uint32_t clk_base;
	int hxt;
	int lxt;
	int hirc48;
	uint32_t clk_pclkdiv;
	uint32_t core_clock;
};

static inline int numaker_scc_on(const struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(dev);

	struct numaker_scc_subsys *scc_subsys = (struct numaker_scc_subsys *)subsys;

	if (scc_subsys->subsys_id == NUMAKER_SCC_SUBSYS_ID_PCC) {
		SYS_UnlockReg();
		CLK_EnableModuleClock(scc_subsys->pcc.clk_modidx);
		SYS_LockReg();
	} else {
		return -EINVAL;
	}

	return 0;
}

static inline int numaker_scc_off(const struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(dev);

	struct numaker_scc_subsys *scc_subsys = (struct numaker_scc_subsys *)subsys;

	if (scc_subsys->subsys_id == NUMAKER_SCC_SUBSYS_ID_PCC) {
		SYS_UnlockReg();
		CLK_DisableModuleClock(scc_subsys->pcc.clk_modidx);
		SYS_LockReg();
	} else {
		return -EINVAL;
	}

	return 0;
}

static inline int numaker_scc_get_rate(const struct device *dev, clock_control_subsys_t subsys,
				       uint32_t *rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(subsys);
	ARG_UNUSED(rate);
	return -ENOTSUP;
}

static inline int numaker_scc_set_rate(const struct device *dev, clock_control_subsys_t subsys,
				       clock_control_subsys_rate_t rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(subsys);
	ARG_UNUSED(rate);
	return -ENOTSUP;
}

static inline int numaker_scc_configure(const struct device *dev, clock_control_subsys_t subsys,
					void *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	struct numaker_scc_subsys *scc_subsys = (struct numaker_scc_subsys *)subsys;

	if (scc_subsys->subsys_id == NUMAKER_SCC_SUBSYS_ID_PCC) {
		SYS_UnlockReg();
		CLK_SetModuleClock(scc_subsys->pcc.clk_modidx, scc_subsys->pcc.clk_src,
				   scc_subsys->pcc.clk_div);
		SYS_LockReg();
	} else {
		return -EINVAL;
	}

	return 0;
}

/* System clock controller driver registration */
static DEVICE_API(clock_control, numaker_scc_api) = {
	.on = numaker_scc_on,
	.off = numaker_scc_off,
	.get_rate = numaker_scc_get_rate,
	.set_rate = numaker_scc_set_rate,
	.configure = numaker_scc_configure,
};

/* At most one compatible with status "okay" */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Requires at most one compatible with status \"okay\"");

#define LOG_OSC_SW(osc, sw)                                                                        \
	if (sw == NUMAKER_SCC_CLKSW_ENABLE) {                                                      \
		LOG_DBG("Enable " #osc);                                                           \
	} else if (sw == NUMAKER_SCC_CLKSW_DISABLE) {                                              \
		LOG_DBG("Disable " #osc);                                                          \
	}

static int numaker_scc_init(const struct device *dev)
{
	const struct numaker_scc_config *cfg = dev->config;

	LOG_DBG("CLK base: 0x%08x", cfg->clk_base);
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hxt)
	LOG_OSC_SW(HXT, cfg->hxt);
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), lxt)
	LOG_OSC_SW(LXT, cfg->lxt);
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hirc48)
	LOG_OSC_SW(HIRC48, cfg->hirc48);
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), clk_pclkdiv)
	LOG_DBG("CLK_PCLKDIV: 0x%08x", cfg->clk_pclkdiv);
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), core_clock)
	LOG_DBG("Core clock: %d (Hz)", cfg->core_clock);
#endif

	/*
	 * soc_reset_hook() will respect above configurations and
	 * actually take charge of system clock control initialization.
	 */

	SystemCoreClockUpdate();
	LOG_DBG("SystemCoreClock: %d (Hz)", SystemCoreClock);

	return 0;
}

#define NUMICRO_SCC_INIT(inst)                                                                     \
	static const struct numaker_scc_config numaker_scc_config_##inst = {                       \
		.clk_base = DT_INST_REG_ADDR(inst),                                                \
		.hxt = DT_INST_ENUM_IDX_OR(inst, hxt, NUMAKER_SCC_CLKSW_UNTOUCHED),                \
		.lxt = DT_INST_ENUM_IDX_OR(inst, lxt, NUMAKER_SCC_CLKSW_UNTOUCHED),                \
		.hirc48 = DT_INST_ENUM_IDX_OR(inst, hirc48, NUMAKER_SCC_CLKSW_UNTOUCHED),          \
		.clk_pclkdiv = DT_INST_PROP_OR(inst, clk_pclkdiv, 0),                              \
		.core_clock = DT_INST_PROP_OR(inst, core_clock, 0),                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, numaker_scc_init, NULL, NULL, &numaker_scc_config_##inst,     \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &numaker_scc_api);

DT_INST_FOREACH_STATUS_OKAY(NUMICRO_SCC_INIT);
