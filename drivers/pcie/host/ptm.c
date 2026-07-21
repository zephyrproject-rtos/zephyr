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

static int pcie_ptm_root_setup(const struct device *dev, uint32_t base)
{
	const struct pcie_ptm_root_config *config = dev->config;
	union ptm_cap_reg cap;
	union ptm_ctrl_reg ctrl;

	cap.raw = pcie_conf_read(config->pcie->bdf, base + PTM_CAP_REG_OFFSET);
	if ((cap.root == 0) || ((cap.root == 1) && (cap.responder == 0))) {
		LOG_ERR("PTM root not supported on 0x%x", config->pcie->bdf);
		return -ENOTSUP;
	}

	ctrl.ptm_enable = 1;
	ctrl.root_select = 1;

	pcie_conf_write(config->pcie->bdf, base + PTM_CTRL_REG_OFFSET, ctrl.raw);

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
	union ptm_cap_reg cap;
	union ptm_ctrl_reg ctrl;

	base = pcie_get_ext_cap(bdf, PCIE_EXT_CAP_ID_PTM);
	if (base == 0) {
		LOG_ERR("PTM capability not exposed on 0x%x", bdf);
		return false;
	}

	cap.raw = pcie_conf_read(bdf, base + PTM_CAP_REG_OFFSET);
	if (cap.requester == 0) {
		LOG_ERR("PTM requester not supported on 0x%x", bdf);
		return false;
	}

	ctrl.ptm_enable = 1;

	pcie_conf_write(bdf, base + PTM_CTRL_REG_OFFSET, ctrl.raw);

	LOG_DBG("PTM requester 0x%x enabled", bdf);

	return true;
}
