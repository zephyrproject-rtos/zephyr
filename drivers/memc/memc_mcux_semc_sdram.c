/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SDRAM device driver for the NXP SEMC controller.
 *
 * Initialization ordering: the SEMC controller driver initializes at
 * CONFIG_MEMC_MCUX_SEMC_INIT_PRIORITY and this driver at
 * CONFIG_MEMC_INIT_PRIORITY (which is >= the controller priority), so the
 * controller is ready before SDRAM configuration is attempted. The
 * controller reference is still validated with device_is_ready() as a
 * defensive check.
 */

#define DT_DRV_COMPAT nxp_mcux_semc_sdram

#include <zephyr/device.h>
#include <zephyr/drivers/memc/memc_mcux_semc.h>
#include <zephyr/drivers/memc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(memc_mcux_semc_sdram, CONFIG_MEMC_LOG_LEVEL);

struct memc_mcux_semc_sdram_config {
	const struct device *controller;
	semc_sdram_cs_t cs;
	uint32_t clk_hz;
	uint8_t delay_chain;
	uint8_t port_size;
	uint8_t burst_length;
	/* SDRAM timing template filled in from devicetree. */
	semc_sdram_config_t sdram_cfg;
};

struct memc_mcux_semc_sdram_data {
	semc_sdram_config_t sdram_cfg;
};

static void *memc_mcux_semc_sdram_get_mem_base(const struct device *dev)
{
	const struct memc_mcux_semc_sdram_config *cfg = dev->config;

	return (void *)(uintptr_t)cfg->sdram_cfg.address;
}

static int memc_mcux_semc_sdram_get_size(const struct device *dev, uint64_t *size)
{
	const struct memc_mcux_semc_sdram_config *cfg = dev->config;

	*size = (uint64_t)cfg->sdram_cfg.memsize_kbytes * 1024U;

	return 0;
}

/*
 * The generic memc_read()/memc_write() default to a memcpy through
 * get_mem_base(), which is exactly right for memory-mapped SDRAM.
 */
static DEVICE_API(memc, memc_mcux_semc_sdram_api) = {
	.get_mem_base = memc_mcux_semc_sdram_get_mem_base,
	.get_size = memc_mcux_semc_sdram_get_size,
};

static int memc_mcux_semc_sdram_init(const struct device *dev)
{
	const struct memc_mcux_semc_sdram_config *cfg = dev->config;
	struct memc_mcux_semc_sdram_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->controller)) {
		LOG_ERR("SEMC controller is not ready; check MEMC_MCUX_SEMC_INIT_PRIORITY");
		return -ENODEV;
	}

	data->sdram_cfg = cfg->sdram_cfg;

	/*
	 * kSEMC_PortSize8Bit/16Bit/32Bit are 0/1/2; the 32-bit enum member is
	 * compiled out on SoCs without 32-bit SEMC support, so derive the value
	 * arithmetically instead of referencing the enum directly.
	 */
	if ((cfg->port_size != 8U) && (cfg->port_size != 16U) && (cfg->port_size != 32U)) {
		LOG_ERR("Invalid SDRAM port size: %u", cfg->port_size);
		return -EINVAL;
	}
	data->sdram_cfg.portSize = (smec_port_size_t)(cfg->port_size / 16U);

	switch (cfg->burst_length) {
	case 1:
		data->sdram_cfg.burstLen = kSEMC_Sdram_BurstLen1;
		break;
	case 2:
		data->sdram_cfg.burstLen = kSEMC_Sdram_BurstLen2;
		break;
	case 4:
		data->sdram_cfg.burstLen = kSEMC_Sdram_BurstLen4;
		break;
	case 8:
		data->sdram_cfg.burstLen = kSEMC_Sdram_BurstLen8;
		break;
	default:
		LOG_ERR("Invalid SDRAM burst length: %u", cfg->burst_length);
		return -EINVAL;
	}

	/*
	 * Derive the prescale period and refresh urgent threshold when they
	 * are not provided, matching the MCUX SDK example values.
	 */
	if (data->sdram_cfg.tPrescalePeriod_Ns == 0U) {
		data->sdram_cfg.tPrescalePeriod_Ns = 160U * (1000000000U / cfg->clk_hz);
	}
	if (data->sdram_cfg.refreshUrgThreshold == 0U) {
		data->sdram_cfg.refreshUrgThreshold = data->sdram_cfg.refreshPeriod_nsPerRow;
	}
#if defined(FSL_FEATURE_SEMC_HAS_DELAY_CHAIN_CONTROL) && FSL_FEATURE_SEMC_HAS_DELAY_CHAIN_CONTROL
	data->sdram_cfg.delayChain = cfg->delay_chain;
#endif

	ret = memc_mcux_semc_configure_sdram(cfg->controller, cfg->cs, &data->sdram_cfg,
					     cfg->clk_hz);
	if (ret != 0) {
		LOG_ERR("Failed to configure SDRAM on SEMC: %d", ret);
		return ret;
	}

	return 0;
}

#define MEMC_MCUX_SEMC_SDRAM_INIT(n)                                                               \
	static const struct memc_mcux_semc_sdram_config memc_mcux_semc_sdram_config_##n = {        \
		.controller = DEVICE_DT_GET(DT_INST_PARENT(n)),                                    \
		.cs = DT_INST_PROP(n, cs),                                                         \
		.clk_hz = DT_INST_PROP(n, clk_frequency),                                          \
		.delay_chain = DT_INST_PROP(n, delay_chain),                                       \
		.port_size = DT_INST_PROP(n, port_size),                                           \
		.burst_length = DT_INST_PROP(n, burst_length),                                     \
		.sdram_cfg =                                                                       \
			{                                                                          \
				.csxPinMux =                                                       \
					(semc_iomux_pin)(kSEMC_MUXCSX0 + DT_INST_PROP(n, cs)),     \
				.address = DT_INST_REG_ADDR(n),                                    \
				.memsize_kbytes = DT_INST_REG_SIZE(n) / 1024U,                     \
				.columnAddrBitNum =                                                \
					(semc_sdram_column_bit_num_t)(kSEMC_SdramColunm_12bit +    \
								      (12 -                        \
								       DT_INST_PROP(               \
									       n, column_bits))),  \
				.casLatency = (semc_caslatency_t)DT_INST_PROP(n, cas_latency),     \
				.tPrecharge2Act_Ns = DT_INST_PROP(n, t_precharge_to_act_ns),       \
				.tAct2ReadWrite_Ns = DT_INST_PROP(n, t_act_to_readwrite_ns),       \
				.tRefreshRecovery_Ns = DT_INST_PROP(n, t_refresh_recovery_ns),     \
				.tWriteRecovery_Ns = DT_INST_PROP(n, t_write_recovery_ns),         \
				.tCkeOff_Ns = DT_INST_PROP(n, t_cke_off_ns),                       \
				.tAct2Prechage_Ns = DT_INST_PROP(n, t_act_to_precharge_ns),        \
				.tSelfRefRecovery_Ns = DT_INST_PROP(n, t_self_ref_recovery_ns),    \
				.tRefresh2Refresh_Ns = DT_INST_PROP(n, t_refresh_to_refresh_ns),   \
				.tAct2Act_Ns = DT_INST_PROP(n, t_act_to_act_ns),                   \
				.tPrescalePeriod_Ns = DT_INST_PROP(n, t_prescale_period_ns),       \
				.refreshPeriod_nsPerRow =                                          \
					DT_INST_PROP(n, refresh_period_ns_per_row),                \
				.refreshUrgThreshold = DT_INST_PROP(n, refresh_urgent_threshold),  \
				.refreshBurstLen = DT_INST_PROP(n, refresh_burst_length),          \
				.autofreshTimes = DT_INST_PROP(n, auto_refresh_times),             \
			},                                                                         \
	};                                                                                         \
	static struct memc_mcux_semc_sdram_data memc_mcux_semc_sdram_data_##n;                     \
	DEVICE_DT_INST_DEFINE(n, memc_mcux_semc_sdram_init, NULL, &memc_mcux_semc_sdram_data_##n,  \
			      &memc_mcux_semc_sdram_config_##n, POST_KERNEL,                       \
			      CONFIG_MEMC_INIT_PRIORITY, &memc_mcux_semc_sdram_api);

DT_INST_FOREACH_STATUS_OKAY(MEMC_MCUX_SEMC_SDRAM_INIT)
