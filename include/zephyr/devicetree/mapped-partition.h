/**
 * @file
 * @brief Flash Devicetree macro public API header file, for memory-mapped partitions.
 */

/*
 * Copyright (c) 2020, Linaro Ltd.
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MAPPED_PARTITION_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MAPPED_PARTITION_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-mapped-partition Devicetree Mapped Partition API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a node identifier for a mapped partition with a given label property
 *
 * Example devicetree fragment:
 *
 *	flash@... {
 *		reg = <0x0 0x40000>;
 *		ranges = <0x0 0x0 0x40000>;
 *
 *		partitions {
 *			ranges;
 *
 *			boot_partition: partition@0 {
 *				compatible = "zephyr,mapped-partition";
 *				reg = <0x0 0xc000>;
 *				label = "mcuboot";
 *			};
 *
 *			slot0_partition: partition@c000 {
 *				compatible = "zephyr,mapped-partition";
 *				reg = <0xc000 0x18000>;
 *				label = "image-0";
 *			};
 *
 *			...
 *		};
 *	};
 *
 * Example usage:
 *
 *	DT_NODE_BY_MAPPED_PARTITION_LABEL(mcuboot) // Node identifier for boot_partition
 *	DT_NODE_BY_MAPPED_PARTITION_LABEL(image_0) // Node identifier for slot0_partition
 *
 * @param label lowercase-and-underscores label property value.
 * @return a node identifier for the partition with that label property.
 */
#define DT_NODE_BY_MAPPED_PARTITION_LABEL(label)		\
	DT_CAT(DT_COMPAT_zephyr_mapped_partition_LABEL_, label)

/**
 * @brief Test if a mapped partition with a given label property exists.
 * @param label lowercase-and-underscores label property value.
 * @return 1 if the device has a "zephyr,mapped-partition" compatible, 0 otherwise.
 */
#define DT_HAS_MAPPED_PARTITION_LABEL(label)						\
	IS_ENABLED(DT_CAT3(DT_COMPAT_zephyr_mapped_partition_LABEL_, label, _EXISTS))

/**
 * @brief Test if zephyr,mapped-partition compatible node exists.
 *
 * @param node_id DTS node to test.
 * @return 1 if node exists and has a zephyr,mapped-partition compatible, 0 otherwise.
 */
#define DT_MAPPED_PARTITION_EXISTS(node_id) DT_NODE_HAS_COMPAT(node_id, zephyr_mapped_partition)

/**
 * @brief Get a numeric identifier for a mapped partition.
 * @param node_id node identifier for a zephyr,mapped-partition node.
 * @return the partition's ID, a unique zero-based index number.
 */
#define DT_MAPPED_PARTITION_ID(node_id) DT_CAT(node_id, _PARTITION_ID)

/**
 * @brief Get the node identifier of the NVM memory for a partition.
 * @param node_id node identifier for a zephyr,mapped-partition node.
 * @return the node identifier of the internal memory that contains the zephyr,mapped-partition
 *         node, or @ref DT_INVALID_NODE if it doesn't exist.
 */
#define DT_MEM_FROM_MAPPED_PARTITION(node_id)						\
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_CAT(node_id, _NVM_DEVICE), soc_nv_flash),	\
				       (DT_CAT(node_id, _NVM_DEVICE)),			\
				       (DT_INVALID_NODE))

/**
 * @brief Get the node identifier of the NVM controller for a partition.
 * @param node_id node identifier for a zephyr,mapped-partition node.
 * @return the node identifier of the memory technology device that contains the
 *         zephyr,mapped-partition node.
 */
#define DT_MTD_FROM_MAPPED_PARTITION(node_id) DT_PARENT(DT_MEM_FROM_MAPPED_PARTITION(node_id))

/**
 * @brief Get the absolute address of a mapped partition
 *
 * Example devicetree fragment:
 *
 *	flash@1000000 {
 *		compatible = "soc-nv-flash";
 *		reg = <0x1000000 0x30000>;
 *		ranges = <0x0 0x1000000 0x30000>;
 *
 *		partitions {
 *			ranges;
 *
 *			storage_partition: partition@3a000 {
 *				compatible = "zephyr,mapped-partition";
 *				label = "storage";
 *				reg = <0x3a000 0x8000>;
 *			};
 *		};
 *	};
 *
 * Here, the "storage" partition is seen to belong to flash memory starting at address 0x1000000.
 * The partition's address of 0x3a000 represents an offset inside that flash memory.
 *
 * Example usage:
 *
 *     DT_MAPPED_PARTITION_ADDR(DT_NODELABEL(storage_partition)) // 0x103a000
 *
 * This macro can only be used with partitions of internal memory addressable by the CPU.
 * Otherwise, it may produce a compile-time error, such as: `'__REG_IDX_0_VAL_ADDRESS' undeclared`.
 *
 * @param node_id node identifier for a zephyr,mapped-partition node
 * @return the partition's unit address.
 */
#define DT_MAPPED_PARTITION_ADDR(node_id) DT_REG_ADDR(node_id)

/**
 * @brief Get the offset address of a mapped partition from the NVM node
 *
 * Example devicetree fragment:
 *
 *	flash@1000000 {
 *		compatible = "soc-nv-flash";
 *		reg = <0x1000000 0x30000>;
 *		ranges = <0x0 0x1000000 0x30000>;
 *
 *		partitions {
 *			ranges;
 *
 *			storage_partition: partition@3a000 {
 *				compatible = "zephyr,mapped-partition";
 *				label = "storage";
 *				reg = <0x3a000 0x8000>;
 *			};
 *		};
 *	};
 *
 * Here, the "storage" partition is seen to belong to flash memory starting at address 0x1000000.
 * The partition's address of 0x3a000 represents an offset inside that flash memory.
 *
 * Example usage:
 *
 *     DT_MAPPED_PARTITION_OFFSET(DT_NODELABEL(storage_partition)) // 0x3a000
 *
 * This macro can only be used with partitions of internal memory addressable by the CPU.
 * Otherwise, it may produce a compile-time error, such as: `'__REG_IDX_0_VAL_ADDRESS' undeclared`.
 *
 * @param node_id node identifier for a zephyr,mapped-partition node
 * @return the partition's offset from the memory device.
 */
#define DT_MAPPED_PARTITION_OFFSET(node_id)						\
	(DT_REG_ADDR(node_id) - DT_REG_ADDR(DT_MEM_FROM_MAPPED_PARTITION(node_id)))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_MAPPED_PARTITION_H_ */
