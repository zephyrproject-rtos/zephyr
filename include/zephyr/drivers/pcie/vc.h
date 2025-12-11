/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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

/*
 * 1 default VC + 7 extended VCs
 */
#define PCIE_VC_MAX_COUNT 8U

#define PCIE_VC_SET_TC0 BIT(0)
#define PCIE_VC_SET_TC1 BIT(1)
#define PCIE_VC_SET_TC2 BIT(2)
#define PCIE_VC_SET_TC3 BIT(3)
#define PCIE_VC_SET_TC4 BIT(4)
#define PCIE_VC_SET_TC5 BIT(5)
#define PCIE_VC_SET_TC6 BIT(6)
#define PCIE_VC_SET_TC7 BIT(7)

struct pcie_vctc_map {
	/*
	 * Map the TCs for each VC by setting bits relevantly
	 * Note: one bit cannot be set more than once among the VCs
	 */
	uint8_t vc_tc[PCIE_VC_MAX_COUNT];
	/*
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
