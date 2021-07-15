/*
 * Copyright (c) 2021 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for fixed partitions on flash devices
 */

#ifndef ZEPHYR_INCLUDE_FIXED_PARTITION_H_
#define ZEPHYR_INCLUDE_FIXED_PARTITION_H_

/**
 * @brief Abstraction over fixed partitions on flash devices
 *
 * @defgroup fixed_partition_api fixed partition interface
 * @{
 */

/*
 * This API makes it possible to operate on fixed partitions easily and
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
#include <drivers/flash.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The fixed partition subsystem provide an abstraction layer between
 * hardware-specific device drivers and higher-level applications for fixed
 * partitions on flash devices. Fixed partition provides 3 routines for
 * operation: read, write and erase. Fixed partition erases typically operate on
 * blocks and the erase can only be peformed on a complete block.
 */

/**
 * @brief Retrieve a pointer to a fixed partition.
 *
 * A partition labeled "image-1" is retrieved as:
 * const struct fixed_partition *fxp = FIXED_PARTITION_GET(image_1)
 *
 * @param label Label of the partition.
 */
#define FIXED_PARTITION_GET(label) &UTIL_CAT(fxp_, \
		DT_NODELABEL(DT_NODE_BY_FIXED_PARTITION_LABEL(label)))

struct fxp_info;

/**
 * @brief fxp_info represents a partition. A partition can only be the child of
 * a device (subpartitions are not supported).
 */

struct fxp_info {
	const struct device *device;
	const off_t off;
	const size_t size;
	int (*read)(const struct fxp_info *fxp, off_t off, void *dst,
		    size_t len);
	int (*write)(const struct fxp_info *fxp, off_t off, const void *src,
		     size_t len);
	int (*erase)(const struct fxp_info *fxp, off_t off, size_t len);
};

/**
 * @brief Read data from fixed partition. Read boundaries are verified before
 * read request is executed.
 *
 * @param[in]  fxp Fixed partition
 * @param[in]  off Read offset from partition start
 * @param[out] dst Buffer to store read data
 * @param[in]  len Number of bytes to read
 *
 * @return  0 on success, negative errno code on fail.
 */
int fxp_read(const struct fxp_info *fxp, off_t off, void *dst, size_t len);

/**
 * @brief Write data to fixed partition. Write boundaries are verified before
 * write request is executed.
 *
 * @param[in]  fxp Fixed partition
 * @param[in]  off Write offset from partition start
 * @param[out] src Buffer with data to be written
 * @param[in]  len Number of bytes to write
 *
 * @return  0 on success, negative errno code on fail.
 */
int fxp_write(const struct fxp_info *fxp, off_t off, const void *src,
	      size_t len);

/**
 * @brief Erase range on fixed partition. Erase boundaries are verified before
 * erase is executed.
 *
 * @param[in] fxp Fixed partition
 * @param[in] off Erase offset from partition start
 * @param[in] len Number of bytes to be erase (should be a multiple of
 *                erase-block-size)
 *
 * @return  0 on success, negative errno code on fail.
 */
int fxp_erase(const struct fxp_info *fxp, off_t off, size_t len);

/**
 * @brief Get flash parameters (write-block-size and erase-value) of the device
 * the partition is on.
 *
 * @param[in] fxp Fixed partition
 *
 * @return 0 on success, negative errno code on fail.
 */
const struct flash_parameters *fxp_get_parameters(const struct fxp_info *fxp);

/**
 * @brief Iterate over all pages on a fixed partition
 *
 * This routine iterates over all pages on the given partition, ordered by
 * increasing offset. For each page, it invokes the given callback, passing it
 * the page information and a private data object. The iteration stops when
 * the callback returns false.
 *
 * @param[in] fxp Fixed partition to iterate over
 * @param[in] cb Callback to invoke for each page
 * @param[in,out] data Private data for callback function
 */
void fxp_page_foreach(const struct fxp_info *fxp, flash_page_cb cb, void *data);

/**
 * @brief Get fixed partition page info by offset.
 *
 * @param[in] fxp Fixed partition
 * @param[in] offset Offset in partition
 * @param[out] info Fixed partition page info (start_offset, size and index)
 *
 * @return 0 on success, negative errno code on fail.
 */
int fxp_get_page_info_by_offs(const struct fxp_info *fxp, off_t offset,
			      struct flash_pages_info *info);

/**
 * @brief Get fixed partition page info by index.
 *
 * @param[in] fxp Fixed partition
 * @param[in] idx Index of page in partition
 * @param[out] info Fixed partition page info (start_offset, size and index)
 *
 * @return 0 on success, negative errno code on fail.
 */
int fxp_get_page_info_by_idx(const struct fxp_info *fxp, uint32_t idx,
			     struct flash_pages_info *info);

/**
 *  @brief Get fixed partition page count.
 *
 *  @param[in] fxp Fixed partition
 *
 *  @return  Number of pages.
 */
size_t fxp_get_page_count(const struct fxp_info *fxp);

/** @cond INTERNAL_HIDDEN */
#define DT_DRV_COMPAT fixed_partitions

#define FIXED_PARTITION_DECLARE(n) \
	extern const struct fxp_info UTIL_CAT(fxp_, DT_NODELABEL(n));

#define FOREACH_FIXED_PARTITION_DECLARE(n) \
	DT_FOREACH_CHILD(DT_DRV_INST(n), FIXED_PARTITION_DECLARE)

DT_INST_FOREACH_STATUS_OKAY(FOREACH_FIXED_PARTITION_DECLARE)

/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_FIXED_PARTITION_H_ */
