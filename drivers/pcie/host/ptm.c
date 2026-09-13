/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_PCIE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pcie);

#include <errno.h>

#include <zephyr/kernel.h>

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#define DT_DRV_COMPAT ptm_root

#include <zephyr/drivers/pcie/pcie.h>
#include "ptm.h"

static void pcie_ptm_control_enable(pcie_bdf_t bdf, uint32_t base, bool root)
{
	uint32_t ctrl;

	ctrl = pcie_conf_read(bdf, base + PTM_CTRL_REG_OFFSET);
	ctrl |= PTM_CTRL_ENABLE;
	if (root) {
		ctrl |= PTM_CTRL_ROOT;
	}

	pcie_conf_write(bdf, base + PTM_CTRL_REG_OFFSET, ctrl);
}

static int pcie_ptm_root_setup(const struct device *dev, uint32_t base)
{
	const struct pcie_ptm_root_config *config = dev->config;
	uint32_t cap;

	cap = pcie_conf_read(config->pcie->bdf, base + PTM_CAP_REG_OFFSET);
	if ((cap & PTM_CAP_ROOT) == 0 || (cap & PTM_CAP_RESPONDER) == 0) {
		LOG_ERR("PTM root not supported on 0x%x", config->pcie->bdf);
		return -ENOTSUP;
	}

	pcie_ptm_control_enable(config->pcie->bdf, base, true);

	LOG_DBG("PTM root 0x%x enabled", config->pcie->bdf);

	return 0;
}

static int pcie_ptm_root_init(const struct device *dev)
{
	const struct pcie_ptm_root_config *config = dev->config;
	uint32_t reg;

	reg = pcie_get_ext_cap(config->pcie->bdf, PCIE_EXT_CAP_ID_PTM);
	if (reg == 0) {
		LOG_ERR("PTM capability not exposed on 0x%x", config->pcie->bdf);
		return -ENODEV;
	}

	return pcie_ptm_root_setup(dev, reg);
}

#define PCIE_PTM_ROOT_INIT(index)					\
	DEVICE_PCIE_INST_DECLARE(index);                                \
	static const struct pcie_ptm_root_config ptm_config_##index = {	\
		DEVICE_PCIE_INST_INIT(index, pcie),                     \
	};								\
	DEVICE_DT_INST_DEFINE(index, &pcie_ptm_root_init, NULL, NULL,	\
			      &ptm_config_##index, PRE_KERNEL_1,	\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(PCIE_PTM_ROOT_INIT)


bool pcie_ptm_enable(pcie_bdf_t bdf)
{
	uint32_t base;
	uint32_t cap;

	base = pcie_get_ext_cap(bdf, PCIE_EXT_CAP_ID_PTM);
	if (base == 0) {
		LOG_ERR("PTM capability not exposed on 0x%x", bdf);
		return false;
	}

	cap = pcie_conf_read(bdf, base + PTM_CAP_REG_OFFSET);
	if ((cap & PTM_CAP_REQUESTER) == 0) {
		LOG_ERR("PTM requester not supported on 0x%x", bdf);
		return false;
	}

	pcie_ptm_control_enable(bdf, base, false);

	LOG_DBG("PTM requester 0x%x enabled", bdf);

	return true;
}
