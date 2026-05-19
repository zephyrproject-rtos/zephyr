/**
 * @file
 * @brief Flash Devicetree macro public API header file, for partitions.
 */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_PARTITIONS_H_
#define ZEPHYR_INCLUDE_DEVICETREE_PARTITIONS_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/devicetree/mapped-partition.h>
#include <zephyr/devicetree/fixed-partitions.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-partition Devicetree Partition API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a node identifier for a partition with a given label property
 *
 * @param label lowercase-and-underscores label property value
 * @return a node identifier for the partition with that label property
 */
#define DT_NODE_BY_PARTITION_LABEL(label)			\
	COND_CODE_1(DT_HAS_MAPPED_PARTITION_LABEL(label),	\
		    (DT_NODE_BY_MAPPED_PARTITION_LABEL(label)),	\
		    (DT_NODE_BY_FIXED_PARTITION_LABEL(label)))

/**
 * @brief Test if a partition with a given label property exists
 * @param label lowercase-and-underscores label property value
 * @return 1 if the device has a "zephyr,mapped-partition" or "fixed-subpartitions" compatible,
 *         or parent has a "fixed-partitions" compatible, 0 otherwise.
 */
#define DT_HAS_PARTITION_LABEL(label)								\
	UTIL_OR(DT_HAS_MAPPED_PARTITION_LABEL(label), DT_HAS_FIXED_PARTITION_LABEL(label))

/**
 * @brief Test if zephyr,mapped-partition, fixed-partitions or fixed-subpartitions compatible
 *        node exists
 *
 * @param node_id DTS node to test
 * @return 1 if node exists and has a "zephyr,mapped-partition" or "fixed-subpartitions"
 *         compatible, or if parent has a "fixed-partitions" compatible, 0 otherwise.
 */
#define DT_PARTITION_EXISTS(node_id)				\
	UTIL_OR(DT_MAPPED_PARTITION_EXISTS(node_id),		\
		UTIL_OR(DT_FIXED_PARTITION_EXISTS(node_id),	\
			DT_FIXED_SUBPARTITION_EXISTS(node_id)))

/**
 * @brief Get a numeric identifier for a partition
 * @param node_id node identifier for a partition node
 * @return the partition's ID, a unique zero-based index number
 */
#define DT_PARTITION_ID(node_id)						\
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, zephyr_mapped_partition),	\
		    (DT_MAPPED_PARTITION_ID(node_id)),				\
		    (DT_FIXED_PARTITION_ID(node_id)))

/**
 * @brief Get the node identifier of the NVM memory for a partition
 * @param node_id node identifier for a partition node
 * @return the node identifier of the internal memory that contains the partition node, or
 *         @ref DT_INVALID_NODE if it doesn't exist.
 */
#define DT_MEM_FROM_PARTITION(node_id)						\
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, zephyr_mapped_partition),	\
		    (DT_MEM_FROM_MAPPED_PARTITION(node_id)),			\
		    (DT_MEM_FROM_FIXED_PARTITION(node_id)))

/**
 * @brief Get the node identifier of the NVM controller for a partition
 * @param node_id node identifier for a partition node
 * @return the node identifier of the memory technology device that contains the partition node.
 */
#define DT_MTD_FROM_PARTITION(node_id)						\
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, zephyr_mapped_partition),	\
		    (DT_MTD_FROM_MAPPED_PARTITION(node_id)),			\
		    (DT_MTD_FROM_FIXED_PARTITION(node_id)))

/**
 * @brief Get the absolute address of a partition
 * This macro can only be used with partitions of internal memory
 * addressable by the CPU. Otherwise, it may produce a compile-time
 * error, such as: `'__REG_IDX_0_VAL_ADDRESS' undeclared`.
 *
 * @param node_id node identifier for a partition node
 * @return the partition's unit address.
 */
#define DT_PARTITION_ADDR(node_id)						\
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, zephyr_mapped_partition),	\
		    (DT_MAPPED_PARTITION_ADDR(node_id)),			\
		    (DT_FIXED_PARTITION_ADDR(node_id)))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_PARTITIONS_H_ */
