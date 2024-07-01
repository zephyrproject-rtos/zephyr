/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for flash partitions
 */

#ifndef ZEPHYR_INCLUDE_FLASH_PARTITIONS_H_
#define ZEPHYR_INCLUDE_FLASH_PARTITIONS_H_

/**
 * @brief Abstraction over flash memory and its partitions
 *
 * @defgroup flash_partitions_api flash_partitions interface
 * @{
 */

/*
 * This API makes it possible to operate on flash partitions easily and
 * effectively.
 */

#include <errno.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The flash_partitions subsystem provide an abstraction layer between flash
 * memory specific drivers and higher-level applications for flash memory
 * partitions. On flash partitions the following routines for operation are
 * provided: open, read, write, erase, clear and close.
 *
 */

/**
 * @brief Retrieve a pointer to the flash partition info struct.
 *
 * A flash partition with nodelabel "nodelabel" is retrieved as:
 * const struct flash_partition *fp = FLASH_PARTITION_GET(nodelabel)
 *
 * @param nodelabel of the flash partition.
 */
#define FLASH_PARTITION_GET(nodelabel)                                          \
	&UTIL_CAT(flash_partition_, DT_NODELABEL(nodelabel))

/**
 * @brief flash_partition represents a flash partition.
 */
struct flash_partition {
	/** flash device */
	const struct device *fldev;
	/** flash partition size */
	const size_t size;
	/** flash partition offset on flash device */
	const size_t offset;
	/** flash partition erase-block-size */
	const size_t erase_block_size;
	/** flash partition routines */
	int (*const open)(const struct flash_partition *partition);
	int (*const read)(const struct flash_partition *partition, size_t start,
			  void *data, size_t len);
	int (*const write)(const struct flash_partition *partition, size_t start,
			   const void *data, size_t len);
	int (*const erase)(const struct flash_partition *partition, size_t start,
			   size_t len);
	int (*const close)(const struct flash_partition *partition);
#if  CONFIG_FLASH_PARTITIONS_LABELS
	const char *label;
#endif
};

/**
 * @brief Get the size in byte of a flash partition.
 *
 * @param[in] partition flash partition
 * @return the size.
 */
size_t flash_partition_get_size(const struct flash_partition *partition);

/**
 * @brief Get the erase-block-size in byte of a flash partition.
 *
 * @param[in] partition flash partition
 * @return the size.
 */
size_t flash_partition_get_ebs(const struct flash_partition *partition);

/**
 * @brief Get the label a flash partition.
 *
 * @param[in] partition flash partition
 * @param[out] label
 * @return the size.
 */
const char *flash_partition_get_label(const struct flash_partition *partition);

/**
 * @brief Open a flash partition.
 *
 * @param[in] partition flash partition
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_partition_open(const struct flash_partition *partition);

/**
 * @brief Read data from flash partition. Read boundaries are verified before
 * read request is executed.
 *
 *
 * @param[in] partition flash partition
 * @param[in] start point relative from beginning of flash partition
 * @param[out] data Buffer to store read data
 * @param[in] len Size to read
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_partition_read(const struct flash_partition *partition, size_t start,
			 void *data, size_t len);

/**
 * @brief Write data to flash partition. Write boundaries are verified before
 * write request is executed.
 *
 * @param[in] partition flash partition
 * @param[in] start point relative from beginning of flash partition
 * @param[out] data Buffer with data to be written
 * @param[in] len Size to read
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_partition_write(const struct flash_partition *partition, size_t start,
			  const void *data, size_t len);

/**
 * @brief Erase range on flash partition. Erase boundaries are verified before
 * erase is executed.
 *
 * @param[in] partition flash partition
 * @param[in] start point relative from beginning of flash partition
 * @param[in] len Size to erase
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_partition_erase(const struct flash_partition *partition, size_t start,
			  size_t len);

/**
 * @brief Close a flash partition.
 *
 * @param[in] partition flash partition
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_partition_close(const struct flash_partition *partition);

/** @cond INTERNAL_HIDDEN */

#define FLASH_PARTITION_DECLARE(n)                                              \
	extern const struct flash_partition flash_partition_##n;

#define FLASH_PARTITIONS_DECLARE(inst)                                          \
	DT_FOREACH_CHILD_STATUS_OKAY(inst, FLASH_PARTITION_DECLARE)

DT_FOREACH_STATUS_OKAY(zephyr_flash_partitions, FLASH_PARTITIONS_DECLARE)
/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_FLASH_PARTITIONS_H_ */
