/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PCIE_HOST_PTM_H_
#define ZEPHYR_DRIVERS_PCIE_HOST_PTM_H_

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/cap.h>

#define PTM_CAP_REG_OFFSET 0x04U

union ptm_cap_reg {
	struct {
		uint32_t requester               : 1;
		uint32_t responder               : 1;
		uint32_t root                    : 1;
		uint32_t _reserved1              : 5;
		uint32_t local_clock_granularity : 8;
		uint32_t _reserved2              : 16;
	};
	uint32_t raw;
};

#define PTM_CTRL_REG_OFFSET 0x08U

union ptm_ctrl_reg {
	struct {
		uint32_t ptm_enable            : 1;
		uint32_t root_select           : 1;
		uint32_t _reserved1            : 6;
		uint32_t effective_granularity : 8;
		uint32_t _reserved2            : 16;
	};
	uint32_t raw;
};

struct pcie_ptm_root_config {
	pcie_bdf_t bdf;
};

#endif /* ZEPHYR_DRIVERS_PCIE_HOST_PTM_H_ */
