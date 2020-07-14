/**
 * @file
 * @brief Flash Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2020, Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_FIXED_PARTITION_H_
#define ZEPHYR_INCLUDE_DEVICETREE_FIXED_PARTITION_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-fixed-partition Devicetree Fixed Partition API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a node identifier for a fixed partition with
 *        a given label property
 *
 * Example devicetree fragment:
 *
 *     flash@... {
 *              partitions {
 *                      compatible = "fixed-partitions";
 *                      boot_partition: partition@0 {
 *                              label = "mcuboot";
 *                      };
 *                      slot0_partition: partition@c000 {
 *                              label = "image-0";
 *                      };
 *                      ...
 *              };
 *     };
 *
 * Example usage:
 *
 *     DT_NODE_BY_FIXED_PARTITION_LABEL(mcuboot) // node identifier for boot_partition
 *     DT_NODE_BY_FIXED_PARTITION_LABEL(image_0) // node identifier for slot0_partition
 *
 * @param label lowercase-and-underscores label property value
 * @return a node identifier for the partition with that label property
 */
#define DT_NODE_BY_FIXED_PARTITION_LABEL(label) \
	DT_CAT(DT_COMPAT_fixed_partitions_LABEL_, label)

/**
 * @brief Test if a fixed partition with a given label property exists
 * @param label lowercase-and-underscores label property value
 * @return 1 if any "fixed-partitions" child node has the given label,
 *         0 otherwise.
 */
#define DT_HAS_FIXED_PARTITION_LABEL(label) \
	IS_ENABLED(DT_COMPAT_fixed_partitions_LABEL_##label##_EXISTS)

/**
 * @brief Get a numeric identifier for a fixed partition
 * @param node_id node identifier for a fixed-partitions child node
 * @return the partition's ID, a unique zero-based index number
 */
#define DT_FIXED_PARTITION_ID(node_id) DT_CAT(node_id, _PARTITION_ID)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_FIXED_PARTITION_H_ */
