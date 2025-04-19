/*
 * Ethernet Platform Utilities for Intel Devices
 *
 * This module provides utility functions to interact with the PCIe features
 * of Intel Ethernet devices, facilitating the retrieval of device-specific
 * PCIe configuration details such as BDF (Bus/Device/Function) and device ID.
 *
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETH_INTEL_PLAT_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETH_INTEL_PLAT_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Retrieve the PCIe Bus/Device/Function of the device.
 *
 * @param dev Pointer to the device structure.
 * @return PCIe BDF address as a uint32_t.
 */
extern uint32_t eth_intel_get_pcie_bdf(const struct device *dev);

/**
 * @brief Retrieve the PCIe ID of the device.
 *
 * @param dev Pointer to the device structure.
 * @return PCIe ID as a uint32_t.
 */
extern uint32_t eth_intel_get_pcie_id(const struct device *dev);
#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_INTEL_PLAT_H__ */
