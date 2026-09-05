/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PCIE_HOST_PTM_H_
#define ZEPHYR_DRIVERS_PCIE_HOST_PTM_H_

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/cap.h>

/* pcie_conf_read()/pcie_conf_write() use DWORD indices, not byte offsets. */
#define PTM_REG_OFFSET(_offset) ((_offset) / sizeof(uint32_t))

#define PTM_CAP_REG_OFFSET PTM_REG_OFFSET(0x04U)
#define PTM_CAP_REQUESTER  BIT(0)
#define PTM_CAP_RESPONDER  BIT(1)
#define PTM_CAP_ROOT                    BIT(2)
#define PTM_CAP_LOCAL_CLOCK_GRANULARITY GENMASK(15, 8)

#define PTM_CTRL_REG_OFFSET PTM_REG_OFFSET(0x08U)
#define PTM_CTRL_ENABLE     BIT(0)
#define PTM_CTRL_ROOT                  BIT(1)
#define PTM_CTRL_EFFECTIVE_GRANULARITY GENMASK(15, 8)

struct pcie_ptm_root_config {
	struct pcie_dev *pcie;
};

#endif /* ZEPHYR_DRIVERS_PCIE_HOST_PTM_H_ */
