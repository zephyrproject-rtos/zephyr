/*
 * Copyright (c) 2025 Xtooltech
 * SPDX-License-Identifier: Apache-2.0
 *
 * NXP LPC External Memory Controller (EMC) driver
 * Supports SDRAM configuration and initialization
 */

#define DT_DRV_COMPAT nxp_lpc_emc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <fsl_emc.h>

LOG_MODULE_REGISTER(memc_nxp_emc, CONFIG_MEMC_LOG_LEVEL);

struct memc_nxp_emc_config {
	EMC_Type *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint8_t emc_clock_div;
	uint8_t command_delay;
};

struct memc_nxp_emc_data {
	bool initialized;
};

/* Convert nanoseconds to EMC clock cycles */
static uint32_t ns_to_clocks(uint32_t ns, uint32_t emc_freq)
{
	uint32_t clocks;
	
	clocks = (ns * (emc_freq / 1000000)) / 1000;
	if ((ns * (emc_freq / 1000000) % 1000) != 0) {
		clocks++;
	}
	
	return clocks;
}

static int memc_nxp_emc_init_sdram(const struct device *dev)
{
	const struct memc_nxp_emc_config *config = dev->config;
	EMC_Type *base = config->base;
	uint32_t emc_freq;
	int ret;
	
	/* Get EMC clock frequency */
	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &emc_freq);
	if (ret < 0) {
		LOG_ERR("Failed to get EMC clock rate: %d", ret);
		return ret;
	}
	
	LOG_DBG("EMC clock frequency: %u Hz", emc_freq);
	
	/* Initialize each SDRAM chip */
	for (int i = 0; i < DT_INST_CHILD_NUM(0); i++) {
		const struct device_node *child = DT_INST_CHILD(0, i);
		
		if (!DT_NODE_HAS_COMPAT(child, nxp_lpc_emc_sdram)) {
			continue;
		}
		
		/* Get SDRAM configuration */
		uint32_t sdram_config[5];
		ret = DT_PROP_BY_IDX(child, nxp_sdram_config, 0);
		uint8_t device_type = DT_PROP_BY_IDX(child, nxp_sdram_config, 0);
		uint8_t bus_width = DT_PROP_BY_IDX(child, nxp_sdram_config, 1);
		uint8_t banks = DT_PROP_BY_IDX(child, nxp_sdram_config, 2);
		uint8_t row_bits = DT_PROP_BY_IDX(child, nxp_sdram_config, 3);
		uint8_t col_bits = DT_PROP_BY_IDX(child, nxp_sdram_config, 4);
		
		/* Get timing parameters */
		uint32_t timing[11];
		for (int j = 0; j < 11; j++) {
			timing[j] = DT_PROP_BY_IDX(child, nxp_sdram_timing, j);
		}
		
		uint32_t refresh_period = DT_PROP(child, nxp_refresh_period);
		uint8_t cas_latency = DT_PROP(child, nxp_cas_latency);
		uint32_t mode_reg = DT_PROP(child, nxp_mode_register);
		
		/* Configure EMC basic settings */
		emc_basic_config_t basic_config;
		EMC_GetDefaultBasicConfig(&basic_config);
		basic_config.endian = kEMC_LittleEndian;
		basic_config.fbClkSrc = kEMC_IntloopbackEmcclk;
		basic_config.emcClkDiv = config->emc_clock_div;
		
		/* Configure dynamic timing */
		emc_dynamic_timing_config_t dyn_timing;
		dyn_timing.readConfig = kEMC_Cmddelay;
		dyn_timing.refreshPeriod_Nanosec = refresh_period * 1000 / 64; /* Per row */
		dyn_timing.tRp_Ns = timing[0];
		dyn_timing.tRas_Ns = timing[1];
		dyn_timing.tSrex_Ns = timing[2];
		dyn_timing.tApr_Ns = timing[3];
		dyn_timing.tDal_Ns = timing[4];
		dyn_timing.tWr_Ns = timing[5];
		dyn_timing.tRc_Ns = timing[6];
		dyn_timing.tRfc_Ns = timing[7];
		dyn_timing.tXsr_Ns = timing[8];
		dyn_timing.tRrd_Ns = timing[9];
		dyn_timing.tMrd_Nclk = timing[10];
		
		/* Configure chip specific settings */
		emc_dynamic_chip_config_t chip_config;
		chip_config.chipIndex = i;
		chip_config.dynamicDevice = device_type == 0 ? kEMC_Sdram : kEMC_Sdram;
		chip_config.rAS_Nclk = cas_latency;
		chip_config.sdramModeReg = mode_reg;
		chip_config.sdramExtModeReg = 0; /* Not used for standard SDRAM */
		
		/* Calculate device address map based on configuration */
		uint32_t addr_map = 0;
		
		/* Bus width */
		if (bus_width == 1) { /* 32-bit */
			addr_map |= 0x00004000;
		}
		
		/* Banks */
		if (banks == 1) { /* 4 banks */
			addr_map |= 0x00000080;
		}
		
		/* Row address bits */
		addr_map |= ((row_bits - 11) & 0x3) << 8;
		
		/* Column address bits */
		if (col_bits >= 8) {
			addr_map |= (col_bits - 8) & 0x7;
		}
		
		chip_config.devAddrMap = addr_map;
		
		/* Initialize EMC if first chip */
		if (i == 0) {
			EMC_Init(base, &basic_config);
		}
		
		/* Initialize dynamic memory for this chip */
		EMC_DynamicMemInit(base, &dyn_timing, &chip_config, 1);
		
		LOG_INF("Initialized SDRAM chip %d: %d-bit bus, %d banks, %dx%d bits",
			i, bus_width == 0 ? 16 : 32, banks == 0 ? 2 : 4, row_bits, col_bits);
	}
	
	return 0;
}

static int memc_nxp_emc_init(const struct device *dev)
{
	const struct memc_nxp_emc_config *config = dev->config;
	struct memc_nxp_emc_data *data = dev->data;
	int ret;
	
	/* Configure pins */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure EMC pins: %d", ret);
		return ret;
	}
	
	/* Enable EMC clock */
	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable EMC clock: %d", ret);
		return ret;
	}
	
	/* Initialize SDRAM if any configured */
	if (DT_INST_CHILD_NUM(0) > 0) {
		ret = memc_nxp_emc_init_sdram(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize SDRAM: %d", ret);
			return ret;
		}
	}
	
	data->initialized = true;
	
	LOG_INF("EMC initialized successfully");
	
	return 0;
}

static const struct memc_nxp_emc_config memc_nxp_emc_config_0 = {
	.base = (EMC_Type *)DT_INST_REG_ADDR(0),
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, name),
	.emc_clock_div = DT_INST_PROP(0, nxp_emc_clock_div),
	.command_delay = DT_INST_PROP(0, nxp_command_delay),
};

static struct memc_nxp_emc_data memc_nxp_emc_data_0;

PINCTRL_DT_INST_DEFINE(0);

DEVICE_DT_INST_DEFINE(0, memc_nxp_emc_init, NULL,
		      &memc_nxp_emc_data_0, &memc_nxp_emc_config_0,
		      POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY,
		      NULL);