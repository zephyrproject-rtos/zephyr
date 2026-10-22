/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the PCIe Virtual Channel support APIs.
 * @ingroup pcie_vc_host_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_VC_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_VC_H_

/**
 * @brief PCIe Virtual Channel Host Interface
 * @defgroup pcie_vc_host_interface PCIe Virtual Channel Host Interface
 * @ingroup pcie_host_interface
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stdbool.h>

#include <zephyr/drivers/pcie/pcie.h>

/**
 * Maximum number of Virtual Channels: 1 default VC + 7 extended VCs
 */
#define PCIE_VC_MAX_COUNT 8U

#define PCIE_VC_SET_TC0 BIT(0) /**< Map Traffic Class 0 to a VC */
#define PCIE_VC_SET_TC1 BIT(1) /**< Map Traffic Class 1 to a VC */
#define PCIE_VC_SET_TC2 BIT(2) /**< Map Traffic Class 2 to a VC */
#define PCIE_VC_SET_TC3 BIT(3) /**< Map Traffic Class 3 to a VC */
#define PCIE_VC_SET_TC4 BIT(4) /**< Map Traffic Class 4 to a VC */
#define PCIE_VC_SET_TC5 BIT(5) /**< Map Traffic Class 5 to a VC */
#define PCIE_VC_SET_TC6 BIT(6) /**< Map Traffic Class 6 to a VC */
#define PCIE_VC_SET_TC7 BIT(7) /**< Map Traffic Class 7 to a VC */

/** @brief Traffic Class (TC) to Virtual Channel (VC) map */
struct pcie_vctc_map {
	/**
	 * Map the TCs for each VC by setting bits relevantly
	 * Note: one bit cannot be set more than once among the VCs
	 */
	uint8_t vc_tc[PCIE_VC_MAX_COUNT];
	/**
	 * Number of VCs being addressed
	 */
	int vc_count;
};

/**
 * @brief Enable PCIe Virtual Channel handling
 * @param bdf the target PCI endpoint
 * @return 0 on success, a negative error code otherwise
 *
 * Note: Not being able to enable such feature is a non-fatal error
 * and any code using it should behave accordingly (displaying some info,
 * and ignoring it for instance).
 */
int pcie_vc_enable(pcie_bdf_t bdf);

/**
 * @brief Disable PCIe Virtual Channel handling
 * @param bdf the target PCI endpoint
 * @return 0 on success, a negative error code otherwise
 */
int pcie_vc_disable(pcie_bdf_t bdf);

/**
 * @brief Map PCIe TC/VC
 * @param bdf the target PCI endpoint
 * @param map the tc/vc map to apply
 * @return 0 on success, a negative error code otherwise
 *
 * Note: VC must be disabled prior to call this function and
 * enabled afterward in order for the endpoint to take advandage of the map.
 *
 * Note: Not being able to enable such feature is a non-fatal error
 * and any code using it should behave accordingly (displaying some info,
 * and ignoring it for instance).
 */
int pcie_vc_map_tc(pcie_bdf_t bdf, struct pcie_vctc_map *map);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_VC_H_ */
