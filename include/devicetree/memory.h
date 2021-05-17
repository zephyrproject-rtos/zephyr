/**
 * @file
 * @brief reserved-memory Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MEMORY_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MEMORY_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-reserved Devicetree reserved-memory API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the pointer to the reserved-memory region
 *
 * Example devicetree fragment:
 *
 *     reserved: reserved-memory {
 *         compatible = "reserved-memory";
 *         ...
 *         n: node {
 *             reg = <0x42000000 0x1000>;
 *         };
 *      };
 *
 * Example usage:
 *
 *     DT_RESERVED_MEM_GET_PTR(DT_NODELABEL(n)) // (uint8_t *) 0x42000000
 *
 * @param node_id node identifier
 * @return pointer to the beginning of the reserved-memory region
 */
#define DT_RESERVED_MEM_GET_PTR(node_id) _DT_RESERVED_START(node_id)

/**
 * @brief Get the size of the reserved-memory region
 *
 * Example devicetree fragment:
 *
 *     reserved: reserved-memory {
 *         compatible = "reserved-memory";
 *         ...
 *         n: node {
 *             reg = <0x42000000 0x1000>;
 *         };
 *     };
 *
 * Example usage:
 *
 *     DT_RESERVED_MEM_GET_SIZE(DT_NODELABEL(n)) // 0x1000
 *
 * @param node_id node identifier
 * @return the size of the reserved-memory region
 */
#define DT_RESERVED_MEM_GET_SIZE(node_id) DT_REG_SIZE(node_id)

/**
 * @brief Get the pointer to the reserved-memory region from a memory-reserved
 *        phandle
 *
 * Example devicetree fragment:
 *
 *     reserved: reserved-memory {
 *         compatible = "reserved-memory";
 *         ...
 *         res0: res {
 *             reg = <0x42000000 0x1000>;
 *             label = "res0";
 *         };
 *     };
 *
 *     n: node {
 *         memory-region = <&res0>;
 *     };
 *
 * Example usage:
 *
 *     DT_RESERVED_MEM_GET_PTR_BY_PHANDLE(DT_NODELABEL(n), memory_region) // (uint8_t *) 0x42000000
 *
 * @param node_id node identifier
 * @param ph phandle to reserved-memory region
 *
 * @return pointer to the beginning of the reserved-memory region
 */
#define DT_RESERVED_MEM_GET_PTR_BY_PHANDLE(node_id, ph) \
	DT_RESERVED_MEM_GET_PTR(DT_PHANDLE(node_id, ph))

/**
 * @brief Get the size of the reserved-memory region from a memory-reserved
 *        phandle
 *
 * Example devicetree fragment:
 *
 *     reserved: reserved-memory {
 *         compatible = "reserved-memory";
 *         ...
 *         res0: res {
 *             reg = <0x42000000 0x1000>;
 *             label = "res0";
 *         };
 *     };
 *
 *     n: node {
 *         memory-region = <&res0>;
 *     };
 *
 * Example usage:
 *
 *     DT_RESERVED_MEM_GET_SIZE_BY_PHANDLE(DT_NODELABEL(n), memory_region) // (uint8_t *) 0x42000000
 *
 * @param node_id node identifier
 * @param ph phandle to reserved-memory region
 *
 * @return size of the reserved-memory region
 */
#define DT_RESERVED_MEM_GET_SIZE_BY_PHANDLE(node_id, ph) \
	DT_RESERVED_MEM_GET_SIZE(DT_PHANDLE(node_id, ph))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_MEMORY_H_ */
