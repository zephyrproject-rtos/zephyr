/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_PTM_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_PTM_H_

#include <stddef.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable PTM on endpoint
 *
 * @param bdf the PCI(e) endpoint
 * @return true if that was successful, false otherwise
 */
bool pcie_ptm_enable(pcie_bdf_t bdf);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_PTM_H_ */
