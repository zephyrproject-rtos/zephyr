/**
 * @file
 * @brief PCIE devicetree macro public API header file.
 */

/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_PCIE_H_
#define ZEPHYR_INCLUDE_DEVICETREE_PCIE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-pcie Devicetree PCIE API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get PCIE BDF for a DT_DRV_COMPAT device
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return instance's PCIE BDF
 */
#define DT_INST_PCIE_BDF(inst) DT_INST_REG_ADDR(inst)

/**
 * @brief Get PCIE ID for a DT_DRV_COMPAT device
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return instance's PCIE ID
 */
#define DT_INST_PCIE_ID(inst) DT_INST_REG_SIZE(inst)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_PCIE_H_ */
