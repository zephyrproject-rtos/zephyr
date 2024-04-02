/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for non volatile memory and partitions
 */

#ifndef ZEPHYR_INCLUDE_NVMP_H_
#define ZEPHYR_INCLUDE_NVMP_H_

/**
 * @brief Abstraction over non volatile memory and its partitions on non
 *	  volatile memory and their drivers
 *
 * @defgroup nvmp_part_api nvmp interface
 * @{
 */

/*
 * This API makes it possible to operate on non volatile memory and its
 * partitions easily and effectively.
 */

#include <errno.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The nvmp subsystem provide an abstraction layer between non volatile memory
 * specific drivers and higher-level applications for non volatile memory and
 * its partitions. On non volatile memory and its partitions 3 routines for
 * operation are provided: read, write and erase.
 *
 */

/**
 * @brief Retrieve a pointer to the non volatile info struct of non volatile
 *        memory.
 *
 * A non volatile memory or one of its partition with nodelabel "nodelabel" is
 * retrieved as:
 * const struct nbmp_info *nvmp = NVMP_GET(nodelabel)
 *
 * @param nodelabel of the non volatile memory or one of its partitions.
 */
#define NVMP_GET(nodelabel) &UTIL_CAT(nvmp_info_, DT_NODELABEL(nodelabel))

struct nvmp_info;
/**
 * @brief nvmp_info represents a nvmp item.
 */
struct nvmp_info {
	/** nvmp store */
	const void *store;
	/** nvmp size */
	const size_t size;
	/** nvmp block-size */
	const size_t block_size;
	/** nvmp write-block-size */
	const size_t write_block_size;
	/** nvmp routines */
	int (*const open)(const struct nvmp_info *info);
	int (*const read)(const struct nvmp_info *info, size_t start, void *data,
			  size_t len);
	int (*const write)(const struct nvmp_info *info, size_t start,
			   const void *data, size_t len);
	int (*const erase)(const struct nvmp_info *info, size_t start,
			   size_t len);
	int (*const clear)(const struct nvmp_info *info, void *data, size_t len);
	int (*const close)(const struct nvmp_info *info);
};

/**
 * @brief Get the size in byte of a nvmp item.
 *
 * @param[in] info nvmp item
 * @return the size.
 */
size_t nvmp_get_size(const struct nvmp_info *info);

/**
 * @brief Get the block-size in byte of a nvmp item.
 *
 * @param[in] info nvmp item
 * @return the size.
 */
size_t nvmp_get_block_size(const struct nvmp_info *info);

/**
 * @brief Get the write-block-size in byte of a nvmp item.
 *
 * @param[in] info nvmp item
 * @return the size.
 */
size_t nvmp_get_write_block_size(const struct nvmp_info *info);

/**
 * @brief Open a nvmp item.
 *
 * @param[in] info nvmp item
 *
 * @return  0 on success, negative errno code on fail.
 */
int nvmp_open(const struct nvmp_info *info);

/**
 * @brief Read data from nvmp item. Read boundaries are verified before read
 * request is executed.
 *
 *
 * @param[in] info nvmp item
 * @param[in] start point relative from beginning of nvmp item
 * @param[out] data Buffer to store read data
 * @param[in] len Size to read
 *
 * @return  0 on success, negative errno code on fail.
 */
int nvmp_read(const struct nvmp_info *info, size_t start, void *data,
	      size_t len);

/**
 * @brief Write data to nvmp item. Write boundaries are verified before write
 * request is executed.
 *
 * @param[in] info nvmp item
 * @param[in] start point relative from beginning of nvmp item
 * @param[out] data Buffer with data to be written
 * @param[in] len Size to read
 *
 * @return  0 on success, negative errno code on fail.
 */
int nvmp_write(const struct nvmp_info *info, size_t start, const void *data,
	       size_t len);

/**
 * @brief Erase range on nvmp item. Erase boundaries are verified before erase
 * is executed.
 *
 * @param[in] info nvmp item
 * @param[in] start point relative from beginning of nvmp item
 * @param[in] len Size to erase
 *
 * @return  0 on success, negative errno code on fail.
 */
int nvmp_erase(const struct nvmp_info *info, size_t start, size_t len);

/**
 * @brief Clear a buffer by setting the value to the nvmp item erase value.
 *
 *
 * @param[in] info nvmp item
 * @param[in] data Buffer to clear
 * @param[in] len Size to clear
 *
 * @return  0 on success, negative errno code on fail.
 */
int nvmp_clear(const struct nvmp_info *info, void *data, size_t len);

/**
 * @brief Close a nvmp item.
 *
 * @param[in] info nvmp item
 *
 * @return  0 on success, negative errno code on fail.
 */
int nvmp_close(const struct nvmp_info *info);

/** @cond INTERNAL_HIDDEN */

#define NVMP_DECLARE_ITEM(n) extern const struct nvmp_info nvmp_info_##n;
#define NVMP_DECLARE(inst) DT_FOREACH_CHILD_STATUS_OKAY(inst, NVMP_DECLARE_ITEM)

#ifdef CONFIG_NVMP_EEPROM
DT_FOREACH_STATUS_OKAY(zephyr_eeprom_nvmp_fixed_partitions, NVMP_DECLARE)
#endif

#ifdef CONFIG_NVMP_FLASH
DT_FOREACH_STATUS_OKAY(zephyr_flash_nvmp_fixed_partitions, NVMP_DECLARE)
#endif

/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NVMP_H_ */
