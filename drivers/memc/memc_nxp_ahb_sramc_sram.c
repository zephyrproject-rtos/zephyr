/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SRAM device driver for the NXP AHB SRAM Controller.
 *
 * Every devicetree node with the "nxp,ahb-sramc-sram" compatible is one
 * external device on one chip select of the controller. This driver programs
 * that chip select through the controller and registers the device's
 * memory-mapped window with the MEMC API.
 *
 * Initialization ordering: the controller initializes at
 * CONFIG_MEMC_NXP_AHB_SRAMC_INIT_PRIORITY, this driver at
 * CONFIG_MEMC_INIT_PRIORITY. The controller reference is validated with
 * device_is_ready().
 */

#define DT_DRV_COMPAT nxp_ahb_sramc_sram

#include <zephyr/device.h>
#include <zephyr/drivers/memc.h>
#include <zephyr/drivers/memc/memc_nxp_ahb_sramc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_sramc.h>

LOG_MODULE_REGISTER(memc_nxp_ahb_sramc_sram, CONFIG_MEMC_LOG_LEVEL);

BUILD_ASSERT(CONFIG_MEMC_NXP_AHB_SRAMC_INIT_PRIORITY <= CONFIG_MEMC_INIT_PRIORITY,
	     "AHB SRAM controller must initialize before its devices");

/* RM 30.3.2: the controller implements at most 128 KB per chip select. */
#define SRAMC_MAX_WINDOW_SIZE (128U * 1024U)

struct memc_nxp_ahb_sramc_sram_config {
	const struct device *controller;
	struct memc_nxp_ahb_sramc_target target;
	void *mem_base;
	uint64_t mem_size;
};

static void *memc_nxp_ahb_sramc_sram_get_mem_base(const struct device *dev)
{
	const struct memc_nxp_ahb_sramc_sram_config *cfg = dev->config;

	return cfg->mem_base;
}

static int memc_nxp_ahb_sramc_sram_get_size(const struct device *dev, uint64_t *size)
{
	const struct memc_nxp_ahb_sramc_sram_config *cfg = dev->config;

	*size = cfg->mem_size;

	return 0;
}

static DEVICE_API(memc, memc_nxp_ahb_sramc_sram_api) = {
	.get_mem_base = memc_nxp_ahb_sramc_sram_get_mem_base,
	.get_size = memc_nxp_ahb_sramc_sram_get_size,
};

static int memc_nxp_ahb_sramc_sram_init(const struct device *dev)
{
	const struct memc_nxp_ahb_sramc_sram_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->controller)) {
		LOG_ERR("AHB SRAM controller is not ready; check MEMC_NXP_AHB_SRAMC_INIT_PRIORITY");
		return -ENODEV;
	}

	ret = memc_nxp_ahb_sramc_configure_target(cfg->controller, &cfg->target);
	if (ret != 0) {
		LOG_ERR("cannot configure CS%u (%d)", cfg->target.chip_select, ret);
		return ret;
	}

	LOG_DBG("CS%u device at %p, %llu bytes", cfg->target.chip_select, cfg->mem_base,
		cfg->mem_size);

	return 0;
}

/* One bus timing, mapped from its devicetree property to its register field. */
#define SRAMC_TIMING(n, name, prop) [MEMC_NXP_AHB_SRAMC_TIMING_##name] = DT_INST_PROP(n, prop),

#define SRAMC_TARGET_INIT(n) \
	{ \
		.timing_ns = \
			{ \
				SRAMC_TIMING(n, CE_SETUP, ce_setup_time_ns) \
				SRAMC_TIMING(n, CE_HOLD, ce_hold_time_ns) \
				SRAMC_TIMING(n, ADDRESS_SETUP, address_setup_time_ns) \
				SRAMC_TIMING(n, ADDRESS_HOLD, address_hold_time_ns) \
				SRAMC_TIMING(n, TURNAROUND, turnaround_time_ns) \
				SRAMC_TIMING(n, WE_LOW, write_enable_low_time_ns) \
				SRAMC_TIMING(n, WE_HIGH, write_enable_high_time_ns) \
				SRAMC_TIMING(n, RE_LOW, read_enable_low_time_ns) \
				SRAMC_TIMING(n, RE_HIGH, read_enable_high_time_ns) \
			}, \
		.chip_select = DT_INST_PROP(n, cs), \
		.wait_pin = DT_INST_PROP_OR(n, wait_pin, 0), \
		.wait_mode = DT_INST_PROP_OR(n, wait_mode, 0), \
		.wait_enable = DT_INST_NODE_HAS_PROP(n, wait_pin), \
		.wait_active_high = DT_INST_PROP(n, wait_active_high), \
	}

#define MEMC_NXP_AHB_SRAMC_SRAM_INIT(n) \
	BUILD_ASSERT(DT_INST_PROP(n, cs) < AHB_SRAMC_CFG0_CS_COUNT, \
		     "chip-select index is beyond what this SoC implements"); \
	BUILD_ASSERT(DT_INST_REG_SIZE(n) <= SRAMC_MAX_WINDOW_SIZE, \
		     "a chip-select range holds at most 128 KB (RM 30.3.2)"); \
	BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(n, wait_pin) || DT_INST_NODE_HAS_PROP(n, wait_mode), \
		     "wait-mode is required on a chip select that sets wait-pin"); \
 \
	static const struct memc_nxp_ahb_sramc_sram_config \
		memc_nxp_ahb_sramc_sram_cfg_##n = { \
			.controller = DEVICE_DT_GET(DT_INST_PARENT(n)), \
			.target = SRAMC_TARGET_INIT(n), \
			.mem_base = (void *)(uintptr_t)DT_INST_REG_ADDR(n), \
			.mem_size = (uint64_t)DT_INST_REG_SIZE(n), \
		}; \
 \
	DEVICE_DT_INST_DEFINE(n, memc_nxp_ahb_sramc_sram_init, NULL, NULL, \
			      &memc_nxp_ahb_sramc_sram_cfg_##n, POST_KERNEL, \
			      CONFIG_MEMC_INIT_PRIORITY, &memc_nxp_ahb_sramc_sram_api);

DT_INST_FOREACH_STATUS_OKAY(MEMC_NXP_AHB_SRAMC_SRAM_INIT)
