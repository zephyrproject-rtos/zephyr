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
	IS_ENABLED(DT_CAT3(DT_COMPAT_fixed_partitions_LABEL_, label, _EXISTS))

/**
 * @brief Test if fixed-partition compatible node exists
 *
 * @param node_id DTS node to test
 * @return 1 if node exists and is fixed-partition compatible, 0 otherwise.
 */
#define DT_FIXED_PARTITION_EXISTS(node_id)		\
	DT_NODE_HAS_COMPAT(DT_PARENT(node_id), fixed_partitions)

/**
 * @brief Get a numeric identifier for a fixed partition
 * @param node_id node identifier for a fixed-partitions child node
 * @return the partition's ID, a unique zero-based index number
 */
#define DT_FIXED_PARTITION_ID(node_id) DT_CAT(node_id, _PARTITION_ID)

/**
 * @brief Get the node identifier of the flash memory for a partition
 * @param node_id node identifier for a fixed-partitions child node
 * @return the node identifier of the internal memory that contains the
 * fixed-partitions node, or @ref DT_INVALID_NODE if it doesn't exist.
 */
#define DT_MEM_FROM_FIXED_PARTITION(node_id)                                                       \
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_GPARENT(node_id), soc_nv_flash), (DT_GPARENT(node_id)),  \
		    (DT_INVALID_NODE))

/**
 * @brief Get the node identifier of the flash controller for a partition
 * @param node_id node identifier for a fixed-partitions child node
 * @return the node identifier of the memory technology device that
 * contains the fixed-partitions node.
 */
#define DT_MTD_FROM_FIXED_PARTITION(node_id)                                                       \
	COND_CODE_1(DT_NODE_EXISTS(DT_MEM_FROM_FIXED_PARTITION(node_id)),                          \
		    (DT_PARENT(DT_MEM_FROM_FIXED_PARTITION(node_id))), (DT_GPARENT(node_id)))

/**
 * @brief Get the absolute address of a fixed partition
 *
 * Example devicetree fragment:
 *
 *     &flash_controller {
 *             flash@1000000 {
 *                     compatible = "soc-nv-flash";
 *                     partitions {
 *                             compatible = "fixed-partitions";
 *                             storage_partition: partition@3a000 {
 *                                     label = "storage";
 *                             };
 *                     };
 *             };
 *     };
 *
 * Here, the "storage" partition is seen to belong to flash memory
 * starting at address 0x1000000. The partition's unit address of
 * 0x3a000 represents an offset inside that flash memory.
 *
 * Example usage:
 *
 *     DT_FIXED_PARTITION_ADDR(DT_NODELABEL(storage_partition)) // 0x103a000
 *
 * This macro can only be used with partitions of internal memory
 * addressable by the CPU. Otherwise, it may produce a compile-time
 * error, such as: `'__REG_IDX_0_VAL_ADDRESS' undeclared`.
 *
 * @param node_id node identifier for a fixed-partitions child node
 * @return the partition's offset plus the base address of the flash
 * node containing it.
 */
#define DT_FIXED_PARTITION_ADDR(node_id)                                                           \
	(DT_REG_ADDR(DT_MEM_FROM_FIXED_PARTITION(node_id)) + DT_REG_ADDR(node_id))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_FIXED_PARTITION_H_ */
