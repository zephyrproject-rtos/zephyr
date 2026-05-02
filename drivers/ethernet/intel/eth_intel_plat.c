/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/ethernet/eth_intel_plat.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intel_eth_plat, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT intel_eth_plat

/* Device id supported in igc */
enum i226_sku {
	INTEL_IGC_I226_LMVP = 0x5503,
	INTEL_IGC_I226_LM = 0x125B,
	INTEL_IGC_I226_V = 0x125C,
	INTEL_IGC_I226_IT = 0x125D,
	INTEL_IGC_I226_BLANK_NVM = 0x125F,
};

struct intel_eth_plat_cfg {
	struct pcie_dev *pcie;
};

struct intel_eth_plat_data {
	DEVICE_MMIO_RAM;
	mm_reg_t base;
};

uint32_t eth_intel_get_pcie_bdf(const struct device *dev)
{
	const struct intel_eth_plat_cfg *cfg = dev->config;

	return cfg->pcie->bdf;
}

static int eth_intel_validate_sku(const struct device *dev)
{
	const struct intel_eth_plat_cfg *cfg = dev->config;
	pcie_id_t pcie_id = cfg->pcie->id;

	switch (PCIE_ID_TO_DEV(pcie_id)) {
	case INTEL_IGC_I226_LMVP:
	case INTEL_IGC_I226_LM:
	case INTEL_IGC_I226_V:
	case INTEL_IGC_I226_IT:
		return 0;
	case INTEL_IGC_I226_BLANK_NVM:
	default:
		break;
	}

	LOG_ERR("SKU validation failed & pcie_id is %x", pcie_id);

	return -EIO;
}

static int intel_eth_plat_init(const struct device *dev)
{
	const struct intel_eth_plat_cfg *cfg = dev->config;
	struct pcie_bar mbar;
	int ret;

	ret = eth_intel_validate_sku(dev);
	if (ret < 0) {
		return ret;
	}

	if (cfg->pcie->bdf == PCIE_BDF_NONE || !pcie_probe_mbar(cfg->pcie->bdf, 0, &mbar)) {
		LOG_ERR("Cannot get mbar");
		return -ENOENT;
	}

	pcie_set_cmd(cfg->pcie->bdf, PCIE_CONF_CMDSTAT_MEM | PCIE_CONF_CMDSTAT_MASTER, true);

	device_map(DEVICE_MMIO_RAM_PTR(dev), mbar.phys_addr, mbar.size, K_MEM_CACHE_NONE);

	return 0;
}

#define INTEL_ETH_PLAT_INIT(n)                                                                     \
	DEVICE_PCIE_INST_DECLARE(n);                                                               \
	static struct intel_eth_plat_data plat_data_##n;                                           \
	static const struct intel_eth_plat_cfg plat_cfg_##n = {                                    \
		DEVICE_PCIE_INST_INIT(n, pcie),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, intel_eth_plat_init, NULL, &plat_data_##n, &plat_cfg_##n,         \
			      POST_KERNEL, CONFIG_PCIE_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INTEL_ETH_PLAT_INIT)
