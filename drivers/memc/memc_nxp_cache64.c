/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <fsl_cache.h>

LOG_MODULE_REGISTER(memc_nxp_cache64, CONFIG_MEMC_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_cache64

#ifndef CACHE64_REGION_NUM
#define CACHE64_REGION_NUM (3U)
#endif

#ifndef CACHE64_REGION_ALIGNMENT
#define CACHE64_REGION_ALIGNMENT 0x400U /* 1KB alignment for boundaries */
#endif

/* Region tuple size in devicetree: <boundary, policy> */
#define CACHE64_REGION_TUPLE_SIZE 2

/* Region configuration: boundary address and policy */
struct cache64_region {
	uint32_t boundary; /* Top address of region in bytes (FlexSPI domain) */
	uint8_t policy;    /* 0=NC, 1=WT, 2=WB */
};

struct cache64_config {
	CACHE64_CTRL_Type *ctrl;
	CACHE64_POLSEL_Type *polsel;
	/* Raw DT regions array (like MEMC_FLEXSPI buf_cfg): */
	const uint32_t *regions_raw;
	bool enable_write_buffer;
};

/**
 * @brief Program CACHE64_POLSEL region boundaries and policies
 */
static int cache64_program_polsel(const struct cache64_config *cfg)
{
	cache64_config_t hw_cfg;
	(void)memset(&hw_cfg, 0, sizeof(hw_cfg));

	/* regions_raw is tuples of <boundary, policy>; count tuples */
	size_t raw_len = 0U;

	for (uint8_t i = 0U; i < (CACHE64_REGION_NUM - 1U); i++) {
		uint32_t boundary = cfg->regions_raw[i * CACHE64_REGION_TUPLE_SIZE + 0];
		uint32_t policy = cfg->regions_raw[i * CACHE64_REGION_TUPLE_SIZE + 1];

		if ((boundary == 0U) && (policy == 0U)) {
			break;
		}
		if ((boundary & (CACHE64_REGION_ALIGNMENT - 1U)) != 0U) {
			LOG_WRN("Region %u boundary 0x%x not 1KB aligned; aligning down", i,
				boundary);
			boundary &= ~(CACHE64_REGION_ALIGNMENT - 1U);
		}
		hw_cfg.boundaryAddr[i] = boundary;
		hw_cfg.policy[i] = (cache64_policy_t)policy;
		raw_len += CACHE64_REGION_TUPLE_SIZE;
	}

	/* Set last region policy if provided; else default NC */
	uint8_t last_idx =
		(uint8_t)((raw_len / CACHE64_REGION_TUPLE_SIZE) < (CACHE64_REGION_NUM - 1U)
				  ? (raw_len / CACHE64_REGION_TUPLE_SIZE)
				  : (CACHE64_REGION_NUM - 1U));
	if (last_idx < CACHE64_REGION_NUM) {
		uint32_t last_pol = cfg->regions_raw[last_idx * CACHE64_REGION_TUPLE_SIZE + 1];

		if (last_pol <= (uint32_t)kCACHE64_PolicyWriteBack) {
			hw_cfg.policy[last_idx] = (cache64_policy_t)last_pol;
		} else {
			hw_cfg.policy[last_idx] = kCACHE64_PolicyNonCacheable;
		}
	}

	status_t st = CACHE64_Init(cfg->polsel, &hw_cfg);

	if (st != kStatus_Success) {
		LOG_ERR("CACHE64_Init failed (%d)", (int)st);
		return -EIO;
	}

	return 0;
}

static int cache64polsel_init(const struct device *dev)
{

	const struct cache64_config *cfg = dev->config;
	int ret;

	/* Program policy selector regions */
	ret = cache64_program_polsel(cfg);
	if (ret < 0) {
		return ret;
	}

	/* Enable write buffer if configured */
#if !(defined(FSL_FEATURE_CACHE64_CTRL_HAS_NO_WRITE_BUF) && \
		FSL_FEATURE_CACHE64_CTRL_HAS_NO_WRITE_BUF)
	if (cfg->enable_write_buffer) {
		CACHE64_EnableWriteBuffer(cfg->ctrl, true);
		LOG_DBG("Write buffer enabled");
	}
#endif

	CACHE64_InvalidateCache(cfg->ctrl);
	CACHE64_EnableCache(cfg->ctrl);

	return 0;
}

/* Main device instantiation macro */
#define CACHE64_INIT(inst)                                          \
	static const uint32_t cache64_regions_raw_##inst[] =            \
		DT_INST_PROP(inst, regions);                                \
	static const struct cache64_config cache64_config_##inst = {	\
		.ctrl = (CACHE64_CTRL_Type *)DT_INST_REG_ADDR(inst),        \
		.polsel = (CACHE64_POLSEL_Type *)DT_INST_REG_ADDR(inst),    \
		.regions_raw = cache64_regions_raw_##inst,                  \
		.enable_write_buffer =                                      \
			DT_INST_PROP_OR(inst, enable_write_buffer, false),      \
	};                                                              \
	DEVICE_DT_INST_DEFINE(inst,                                     \
				cache64polsel_init,                                 \
				NULL,                                               \
				NULL,                                               \
				&cache64_config_##inst,                             \
				PRE_KERNEL_1,                                       \
				CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                \
				NULL)

DT_INST_FOREACH_STATUS_OKAY(CACHE64_INIT)
