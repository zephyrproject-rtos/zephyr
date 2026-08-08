/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xilinx AXI/XPS Interrupt Controller driver API
 *
 * Helper APIs for accessing interrupt controller status and pending
 * information.
 *
 * All APIs operate on a specific interrupt controller instance and
 * therefore support systems containing multiple Xilinx interrupt
 * controller devices.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_XLNX_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_XLNX_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Acknowledge interrupt sources.
 *
 * Clears the pending state of interrupt lines identified by @p mask.
 *
 * @param dev Interrupt controller device instance.
 * @param mask Interrupt lines to acknowledge.
 */
void xlnx_intc_irq_acknowledge(const struct device *dev,
			       uint32_t mask);

/**
 * @brief Get pending interrupt lines.
 *
 * @param dev Interrupt controller device instance.
 *
 * @return Pending interrupt bitmask.
 */
uint32_t xlnx_intc_irq_pending(const struct device *dev);

/**
 * @brief Get enabled interrupt lines.
 *
 * @param dev Interrupt controller device instance.
 *
 * @return Interrupt enable register contents.
 */
uint32_t xlnx_intc_irq_get_enabled(const struct device *dev);

/**
 * @brief Get highest-priority pending interrupt.
 *
 * When an Interrupt Vector Register (IVR) is implemented, the value
 * is read directly from hardware. Otherwise the highest-priority
 * interrupt is derived from the pending interrupt bitmap.
 *
 * @param dev Interrupt controller device instance.
 *
 * @return Interrupt line index.
 */
uint32_t xlnx_intc_irq_pending_vector(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_XLNX_H_ */
