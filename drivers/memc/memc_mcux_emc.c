/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_emc

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <fsl_emc.h>

LOG_MODULE_REGISTER(memc_mcux_emc, CONFIG_MEMC_LOG_LEVEL);

/* EMC dynamic chip selects DYCS0-DYCS3. */
#define MEMC_EMC_DYCS_COUNT 4U

/*
 * Datasheet timings of one SDRAM chip from a "nxp,lpc-emc-sdram" child node.
 * The controller applies one timing set to every chip, so the worst case of
 * all chips is programmed. The write-recovery timing depends on the EMC clock
 * frequency, which is only known at runtime.
 */
struct emc_sdram_timing {
	uint32_t refresh_ns;
	uint32_t trp_ns;
	uint32_t tras_ns;
	uint32_t tsrex_ns;
	uint32_t tapr_ns;
	uint32_t twr_ns;
	uint32_t trc_ns;
	uint32_t trfc_ns;
	uint32_t txsr_ns;
	uint32_t trrd_ns;
	uint8_t tmrd_nclk;
};

struct memc_emc_config {
	EMC_Type *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	emc_basic_config_t basic;
	emc_dynamic_chip_config_t *chips;
	const struct emc_sdram_timing *timings;
	uint8_t sdram_count;
	bool has_fbclk_delay;
	uint8_t fbclk_delay;
};

static void memc_emc_worst_case_timing(const struct memc_emc_config *config, uint32_t emc_freq,
				       emc_dynamic_timing_config_t *timing)
{
	/* One EMC clock period, rounded up: the write-recovery time includes it. */
	uint32_t clk_ns = DIV_ROUND_UP(NSEC_PER_SEC, emc_freq);
	uint32_t twr_ns = 0U;

	timing->readConfig = kEMC_Cmddelay;
	timing->refreshPeriod_Nanosec = UINT32_MAX;

	for (uint8_t i = 0U; i < config->sdram_count; i++) {
		const struct emc_sdram_timing *t = &config->timings[i];

		/* The shortest refresh interval is the worst case. */
		timing->refreshPeriod_Nanosec = MIN(timing->refreshPeriod_Nanosec, t->refresh_ns);
		timing->tRp_Ns = MAX(timing->tRp_Ns, t->trp_ns);
		timing->tRas_Ns = MAX(timing->tRas_Ns, t->tras_ns);
		timing->tSrex_Ns = MAX(timing->tSrex_Ns, t->tsrex_ns);
		timing->tApr_Ns = MAX(timing->tApr_Ns, t->tapr_ns);
		twr_ns = MAX(twr_ns, t->twr_ns);
		timing->tRc_Ns = MAX(timing->tRc_Ns, t->trc_ns);
		timing->tRfc_Ns = MAX(timing->tRfc_Ns, t->trfc_ns);
		timing->tXsr_Ns = MAX(timing->tXsr_Ns, t->txsr_ns);
		timing->tRrd_Ns = MAX(timing->tRrd_Ns, t->trrd_ns);
		timing->tMrd_Nclk = MAX(timing->tMrd_Nclk, t->tmrd_nclk);
	}

	timing->tWr_Ns = clk_ns + twr_ns;
	timing->tDal_Ns = timing->tWr_Ns + timing->tRp_Ns;
}

static int memc_emc_init(const struct device *dev)
{
	const struct memc_emc_config *config = dev->config;
	emc_dynamic_timing_config_t timing = {0};
	emc_basic_config_t basic = config->basic;
	uint32_t emc_freq;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret != 0) {
		return ret;
	}

	/* EMC_Init() programs EMCCLKDIV, so the EMC clock rate is only valid afterwards. */
	EMC_Init(config->base, &basic);

	/*
	 * Align read-data capture with the SDRAM round-trip delay. The same
	 * register holds the command delay, which stays at its reset value.
	 */
	if (config->has_fbclk_delay) {
		/* Bits other than the two delay fields are reserved: write zero. */
		SYSCON->EMCDLYCTRL = (SYSCON->EMCDLYCTRL & SYSCON_EMCDLYCTRL_CMD_DELAY_MASK) |
				     SYSCON_EMCDLYCTRL_FBCLK_DELAY(config->fbclk_delay);
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &emc_freq);
	if (ret != 0) {
		return ret;
	}

	if (emc_freq == 0U) {
		LOG_ERR("EMC clock is not running");
		return -EINVAL;
	}

	memc_emc_worst_case_timing(config, emc_freq, &timing);

	EMC_DynamicMemInit(config->base, &timing, config->chips, config->sdram_count);

	LOG_DBG("EMC up: %u chip(s), EMC clock %u Hz", config->sdram_count, emc_freq);

	return 0;
}

#define MEMC_EMC_SDRAM_CHIP(node)                                                                  \
	{                                                                                          \
		.chipIndex = DT_REG_ADDR(node),                                                    \
		.dynamicDevice = kEMC_Sdram,                                                       \
		.rAS_Nclk = DT_PROP(node, nxp_ras_latency),                                        \
		.sdramModeReg = DT_PROP(node, nxp_sdram_mode_register),                            \
		.sdramExtModeReg = 0,                                                              \
		.devAddrMap = DT_PROP(node, nxp_address_mapping),                                  \
	},

#define MEMC_EMC_SDRAM_TIMING(node)                                                                \
	{                                                                                          \
		.refresh_ns = DT_PROP(node, refresh_period_ns),                                    \
		.trp_ns = DT_PROP(node, t_rp_ns),                                                  \
		.tras_ns = DT_PROP(node, t_ras_ns),                                                \
		.tsrex_ns = DT_PROP(node, t_srex_ns),                                              \
		.tapr_ns = DT_PROP(node, t_apr_ns),                                                \
		.twr_ns = DT_PROP(node, t_wr_ns),                                                  \
		.trc_ns = DT_PROP(node, t_rc_ns),                                                  \
		.trfc_ns = DT_PROP(node, t_rfc_ns),                                                \
		.txsr_ns = DT_PROP(node, t_xsr_ns),                                                \
		.trrd_ns = DT_PROP(node, t_rrd_ns),                                                \
		.tmrd_nclk = DT_PROP(node, nxp_mode_register_delay),                               \
	},

#define MEMC_EMC_SDRAM_CHECK(node)                                                                 \
	BUILD_ASSERT(DT_REG_ADDR(node) < MEMC_EMC_DYCS_COUNT,                                      \
		     "nxp,lpc-emc-sdram reg must be a dynamic chip select (0-3)");

#define MEMC_EMC_SDRAM_COUNT(inst) DT_INST_CHILD_NUM_STATUS_OKAY(inst)

#define MEMC_EMC_INIT(inst)                                                                        \
	BUILD_ASSERT(MEMC_EMC_SDRAM_COUNT(inst) >= 1,                                              \
		     "nxp,lpc-emc requires at least one nxp,lpc-emc-sdram child");                 \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, MEMC_EMC_SDRAM_CHECK)                              \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static emc_dynamic_chip_config_t memc_emc_chips_##inst[] = {                               \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, MEMC_EMC_SDRAM_CHIP)};                     \
	static const struct emc_sdram_timing memc_emc_timings_##inst[] = {                         \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, MEMC_EMC_SDRAM_TIMING)};                   \
	static const struct memc_emc_config memc_emc_config_##inst = {                             \
		.base = (EMC_Type *)DT_INST_REG_ADDR(inst),                                        \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),           \
		.basic =                                                                           \
			{                                                                          \
				.endian = DT_INST_PROP(inst, nxp_emc_big_endian)                   \
						  ? kEMC_BigEndian                                 \
						  : kEMC_LittleEndian,                             \
				.fbClkSrc = DT_INST_PROP(inst, nxp_emc_external_feedback_clock)    \
						    ? kEMC_EMCFbclkInput                           \
						    : kEMC_IntloopbackEmcclk,                      \
				.emcClkDiv = DT_INST_PROP(inst, nxp_emc_clock_div),                \
			},                                                                         \
		.chips = memc_emc_chips_##inst,                                                    \
		.timings = memc_emc_timings_##inst,                                                \
		.sdram_count = ARRAY_SIZE(memc_emc_chips_##inst),                                  \
		.has_fbclk_delay = DT_INST_NODE_HAS_PROP(inst, nxp_emc_feedback_clock_delay),      \
		.fbclk_delay = DT_INST_PROP_OR(inst, nxp_emc_feedback_clock_delay, 0),             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, memc_emc_init, NULL, NULL, &memc_emc_config_##inst,            \
			      POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_EMC_INIT)
