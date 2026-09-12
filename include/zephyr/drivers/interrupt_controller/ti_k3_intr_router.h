/* 
 *	Copyright (c) Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 *	SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_TI_K3_INTR_ROUTER_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_TI_K3_INTR_ROUTER_H_

/**
 * @brief Configure and enable an interrupt route from input to output.
 */
int ti_k3_intr_router_set_route(const struct device *dev, uint16_t output_idx, uint16_t input_idx);

/**
 * @brief Enable an interrupt output route.
 */
int ti_k3_intr_router_enable_route(const struct device *dev, uint16_t output_idx);

/**
 * @brief Disable an interrupt output route.
 */
int ti_k3_intr_router_disable_route(const struct device *dev, uint16_t output_idx);

/**
 * @brief Retrieve current route configuration for an output line.
 */
int ti_k3_intr_router_get_route(const struct device *dev, uint16_t output_idx,
				uint16_t *input_idx, bool *enabled);

/**
 * @brief Dynamically allocate an available output line and configure the route.
 */
int ti_k3_intr_router_alloc_and_route(const struct device *dev, uint16_t input_idx,
				      uint16_t *allocated_output);

/**
 * @brief Free a previously allocated output route.
 */
int ti_k3_intr_router_free_route(const struct device *dev, uint16_t output_idx);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_TI_K3_INTR_ROUTER_H_ */
