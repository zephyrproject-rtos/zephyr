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
 * @brief Get fixed partition with "label" property that matches label
 * @param label lowercase-and-underscores value for "label" property
 * @return a node identifier for the partition that matches label
 */
#define DT_NODE_BY_FIXED_PARTITION_LABEL(label) \
	DT_CAT(DT_COMPAT_fixed_partitions_LABEL_, label)

/**
 * @brief Test if a fixed partition with the "label" property exists
 * @param label lowercase-and-underscores value for "label" property
 * @return 1 if the label exists across all 'fixed-partitions' nodes
 *         0 otherwise.
 */
#define DT_HAS_FIXED_PARTITION_LABEL(label) \
	IS_ENABLED(DT_COMPAT_fixed_partitions_LABEL_##label##_EXISTS)

/**
 * @brief 'fixed-partitions' ID
 * @param node_id node identifier
 * @return unique fixed partition id for given partition referenced by node_id
 */
#define DT_FIXED_PARTITION_ID(node_id) DT_CAT(node_id, _PARTITION_ID)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_FIXED_PARTITION_H_ */
