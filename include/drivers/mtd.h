/*
 * Copyright (c) 2021 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for memory technology devices (mtd) partitions
 */

#ifndef ZEPHYR_INCLUDE_MTD_PART_H_
#define ZEPHYR_INCLUDE_MTD_PART_H_

/**
 * @brief Abstraction over mtd partitions and their drivers
 *
 * @defgroup mtd_part_api mtd partition interface
 * @{
 */

/*
 * This API makes it possible to operate on mtd partitions easily and
 * effectively.
 */

/**
 *
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The MTD subsystem provide an abstraction layer between hardware-specific
 * device drivers and higher-level applications for flash memory. Flash memory
 * typically provides 3 routines for operation: read, write and erase.
 *
 * Both partitions on a MTD as the MTD itself (the MTD master) can be used by
 * the subsystem.
 *
 * Both MTD partitions and MTD masters are designated as MTD items.
 *
 */

/**
 * @brief Retrieve a pointer to the MTD info struct of a MTD partition.
 *
 * A partition labeled "image-1" is retrieved as:
 * const struct mtd_info *mtd = MTD_PART_GET(image_1)
 *
 * @param label Label of the partition.
 */
#define MTD_PARTITION_GET(label) &UTIL_CAT(mtd_, \
		DT_NODELABEL(DT_NODE_BY_FIXED_PARTITION_LABEL(label)))

/**
 * @brief Retrieve a pointer to the MTD info struct of a MTD master.
 *
 * A master with nodelabel "flash0" is retrieved as:
 * const struct mtd_info *mtd = MTD_MASTER_GET(flash0)
 *
 * @param nodelabel Nodelabel of the MTD master.
 */
#define MTD_MASTER_GET(nodelabel) &UTIL_CAT(mtd_, DT_NODELABEL(nodelabel))

struct mtd_info;

/**
 * @brief mtd_info_cfg represents the configuration of a mtd item (partition or
 * master).
 */
struct mtd_info_cfg {
	const struct device *device;
	const struct mtd_info *parent;
	const off_t off;
	const size_t size;
	const size_t erase_block_size;
	const bool read_only;
};

/**
 * @brief mtd_info_state represent the state of a mtd item (partition or master)
 */
struct mtd_info_state {};

/**
 * @brief mtd_info represents a mtd item (partition or master).
 */
struct mtd_info {
	const struct mtd_info_cfg *cfg;
	const struct mtd_info_state *state;
};

/**
 * @brief mtd_block represents a erasable unit (block) on a mtd item.
 */
struct mtd_block {
	/** offset from the mtd item start */
	off_t offset;
	/** block size */
	size_t size;
};

/**
 * @brief Read data from mtd item (partition or master). Read boundaries are
 * verified before read request is executed.
 *
 * @param[in]  mtd MTD item
 * @param[in]  off Offset relative from beginning of mtd item to read
 * @param[out] dst Buffer to store read data
 * @param[in]  len Number of bytes to read
 *
 * @return  0 on success, negative errno code on fail.
 */
int mtd_read(const struct mtd_info *mtd, off_t off, void *dst, size_t len);

/**
 * @brief Write data to mtd item (partition or master). Write boundaries are
 * verified before write request is executed.
 *
 * @param[in]  mtd MTD item
 * @param[in]  off Offset relative from beginning of mtd item to write
 * @param[out] src Buffer with data to be written
 * @param[in]  len Number of bytes to write
 *
 * @return  0 on success, negative errno code on fail.
 */
int mtd_write(const struct mtd_info *mtd, off_t off, const void *src,
	      size_t len);

/**
 * @brief Erase range on mtd item. Erase boundaries are verified before erase
 * is executed.
 *
 * @param[in] mtd MTD item
 * @param[in] off Offset relative from beginning of mtd item.
 * @param[in] len Number of bytes to be erase
 *
 * @return  0 on success, negative errno code on fail.
 */
int mtd_erase(const struct mtd_info *mtd, off_t off, size_t len);

/**
 * @brief Get write block size of a mtd item
 *
 * @param[in] mtd MTD item
 * @param[out] wbs Write block size
 *
 * @return 0 on success, negative errno code on fail.
 */
int mtd_get_wbs(const struct mtd_info *mtd, uint8_t *wbs);

/**
 * @brief Get erase block size of a mtd item
 *
 * @param[in] mtd MTD item
 * @param[out] ebs Erase block size
 *
 * @return 0 on success, negative errno code on fail. Fails on mtd with
 * non-constant erase-block-size.
 */
int mtd_get_ebs(const struct mtd_info *mtd, size_t *ebs);

/**
 * @brief Get the mtd item block at offset
 *
 * @param[in] mtd MTD item
 * @param[in] off Offset in mtd item
 * @param[out] block The mtd item block (offset and size)
 *
 * @return 0 on success, negative errno code on fail.
 */
int mtd_get_block_at(const struct mtd_info *mtd, off_t off,
		     struct mtd_block *block);

/**
 * @brief Get erase default value (byte) of the mtd item
 *
 * @param[in] mtd MTD item.
 * @param[out] edv erase default value [B]
 *
 * @return 0 on success, negative errno code on fail.*
 */
int mtd_get_edv(const struct mtd_info *mtd, uint8_t *edv);

/** @cond INTERNAL_HIDDEN */
#define DT_DRV_COMPAT fixed_partitions

#define MTD_PARTITION_DECLARE(n) \
	extern const struct mtd_info UTIL_CAT(mtd_, DT_NODELABEL(n));

#define FOREACH_MTD_PARTITION_DECLARE(n) \
	DT_FOREACH_CHILD(DT_DRV_INST(n), MTD_PARTITION_DECLARE)

DT_INST_FOREACH_STATUS_OKAY(FOREACH_MTD_PARTITION_DECLARE)

#define MTD_MASTER_FILTER_DECLARE(n)  \
	extern const struct mtd_info UTIL_CAT(mtd_, DT_PARENT(n));

#define FOREACH_MASTER_DECLARE(n) \
	MTD_MASTER_FILTER_DECLARE(DT_INST(n, fixed_partitions))

DT_INST_FOREACH_STATUS_OKAY(FOREACH_MASTER_DECLARE)

/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MTD_PART_H_ */
