/*
 * Copyright (c) 2023 Laczen
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
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/retained_mem.h>

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

enum nvmp_type {UNKNOWN, FLASH, EEPROM, RETAINED_MEM};

struct nvmp_info;

/**
 * @brief nvmp_info represents a nvmp item.
 */
struct nvmp_info {
	/** nvmp type */
	const enum nvmp_type type;
	/** nvmp size */
	const size_t size;
	/** nvmp read-only */
	const bool read_only;
	/** nvmp store */
	const void *store;
	/** nvmp start on store */
	const size_t store_start;
	/** nvmp routines */
	int (* const read)(const struct nvmp_info *info, size_t start,
			   void *data, size_t len);
	int (* const write)(const struct nvmp_info *info, size_t start,
			    const void *data, size_t len);
	int (* const erase)(const struct nvmp_info *info, size_t start,
			    size_t len);
};

/**
 * @brief Get the size in byte of a nvmp item.
 *
 * @param[in] info nvmp item
 * @return the size.
 */
size_t nvmp_get_size(const struct nvmp_info *info);

/**
 * @brief Get the type of a nvmp item.
 *
 * @param[in] info nvmp item
 * @return the non volatile memory type.
 */
enum nvmp_type nvmp_get_type(const struct nvmp_info *info);

/**
 * @brief Get the store of a nvmp item.
 *
 * @param[in] info nvmp item
 * @return the store.
 */
const void *nvmp_get_store(const struct nvmp_info *info);

/**
 * @brief Get the start in byte on the store of a nvmp item.
 *
 * @param[in] info nvmp item
 * @return the start on success, negative errno code on fail.
 */
ssize_t nvmp_get_store_start(const struct nvmp_info *info);

/**
 * @brief Read data from nvmp item. Read boundaries are verified before read
 * request is executed.
 *
 * REMARK: start and size are opaque: there meaning can change depending on the
 * store, e.g. byte on flash, eeprom and retained_mem but sector on disk.
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
 * REMARK: start and size are opaque: there meaning can change depending on the
 * store, e.g. byte on flash, eeprom and retained_mem but sector on disk.
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
 * REMARK: start and size are opaque: there meaning can change depending on the
 * store, e.g. byte on flash, eeprom and retained_mem but sector on disk.
 *
 * @param[in] info nvmp item
 * @param[in] start point relative from beginning of nvmp item
 * @param[in] len Size to read
 *
 * @return  0 on success, negative errno code on fail.
 */
int nvmp_erase(const struct nvmp_info *info, size_t start, size_t len);

/**
 * @brief Helper macro to find out the compatibility of a node, this can be
 * useful when aiming for minimal code size when multiple types of non
 * volatile memory are enabled.
 *
 * @param node of the non volatile memory.
 * @param compatible of the non volatile memory
 */
#define NVMP_HAS_COMPATIBLE(node, compatible) COND_CODE_1(			\
	DT_NODE_HAS_COMPAT(node, compatible), (true),				\
	(COND_CODE_1(DT_NODE_HAS_COMPAT(DT_PARENT(node),			\
					zephyr_nvmp_partitions),		\
		(DT_NODE_HAS_COMPAT(DT_GPARENT(node), compatible)), (false))))

/** @cond INTERNAL_HIDDEN */

#define NVMP_DECLARE_ITEM(n) extern const struct nvmp_info nvmp_info_##n;

#define NVMP_PARTITIONS_DECLARE(inst)						\
	DT_FOREACH_CHILD(inst, NVMP_DECLARE_ITEM)

#define NVMP_DECLARE(inst)							\
	NVMP_DECLARE_ITEM(inst)							\
	DT_FOREACH_CHILD_STATUS_OKAY(inst, NVMP_PARTITIONS_DECLARE)

#ifdef CONFIG_NVMP_EEPROM
DT_FOREACH_STATUS_OKAY(zephyr_nvmp_eeprom, NVMP_DECLARE)
#endif

#ifdef CONFIG_NVMP_FLASH
DT_FOREACH_STATUS_OKAY(zephyr_nvmp_flash, NVMP_DECLARE)
#endif

#ifdef CONFIG_NVMP_RETAINED_MEM
DT_FOREACH_STATUS_OKAY(zephyr_nvmp_retained_mem, NVMP_DECLARE)
#endif

/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NVMP_H_ */
