/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_ETHERNET_ETH_XILINX_AXIENET_BD_H_
#define DRIVERS_ETHERNET_ETH_XILINX_AXIENET_BD_H_

#include <zephyr/device.h>

/**
 * @brief Set TX/RX BD counts for the next iface up cycle.
 *
 * Changes take effect after the next net iface up.
 *
 * @param dev    Ethernet device pointer.
 * @param num_tx Number of TX BDs.
 * @param num_rx Number of RX BDs.
 * @return 0 on success, negative errno on failure.
 */
int xilinx_axienet_set_bd_counts(const struct device *dev, size_t num_tx, size_t num_rx);

/**
 * @brief Read the current TX/RX BD (buffer descriptor) counts.
 *
 * Safe to call at any time (interface up or down).
 *
 * @param dev    Ethernet device pointer.
 * @param num_tx Receives the current TX BD count.
 * @param num_rx Receives the current RX BD count.
 */
void xilinx_axienet_get_bd_counts(const struct device *dev, size_t *num_tx, size_t *num_rx);

#endif /* DRIVERS_ETHERNET_ETH_XILINX_AXIENET_BD_H_ */
