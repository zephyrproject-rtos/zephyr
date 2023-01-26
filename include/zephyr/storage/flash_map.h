/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for flash map
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_H_
#define ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_H_

/**
 * @brief Abstraction over flash partitions/areas and their drivers
 *
 * @defgroup flash_area_api flash area Interface
 * @ingroup storage_apis
 * @{
 */

/*
 * This API makes it possible to operate on flash areas easily and
 * effectively.
 *
 * The system contains global data about flash areas. Every area
 * contains an ID number, offset, and length.
 */

/**
 *
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Provided for compatibility with MCUboot */
#define SOC_FLASH_0_ID 0
/** Provided for compatibility with MCUboot */
#define SPI_FLASH_0_ID 1

/**
 * @brief Flash partition
 *
 * This structure represents a fixed-size partition on a flash device.
 * Each partition contains one or more flash sectors.
 */
struct flash_area {
	/** ID number */
	uint8_t fa_id;
	uint16_t pad16;
	/** Start offset from the beginning of the flash device */
	off_t fa_off;
	/** Total size */
	size_t fa_size;
	/** Backing flash device */
	const struct device *fa_dev;
};

/**
 * @brief Structure for transfer flash sector boundaries
 *
 * This template is used for presentation of flash memory structure. It
 * consumes much less RAM than @ref flash_area
 */
struct flash_sector {
	/** Sector offset from the beginning of the flash device */
	off_t fs_off;
	/** Sector size in bytes */
	size_t fs_size;
};

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY)
/**
 * @brief Structure for verify flash region integrity
 *
 * This is used to pass data to be used to check flash integrity using SHA-256
 * algorithm.
 */
struct flash_area_check {
	const uint8_t *match;		/** 256 bits match vector */
	size_t clen;			/** Content len to be compared */
	size_t off;			/** Start Offset */
	uint8_t *rbuf;			/** Temporary read buffer  */
	size_t rblen;			/** Size of read buffer */
};

/**
 * Verify flash memory length bytes integrity from a flash area. The start
 * point is indicated by an offset value.
 *
 * @param[in] fa	Flash area
 * @param[in] fic	Flash area check integrity data
 *
 * @return  0 on success, negative errno code on fail
 */
int flash_area_check_int_sha256(const struct flash_area *fa,
				const struct flash_area_check *fac);
#endif

/**
 * @brief Retrieve partitions flash area from the flash_map.
 *
 * Function Retrieves flash_area from flash_map for given partition.
 *
 * @param[in]  id ID of the flash partition.
 * @param[out] fa Pointer which has to reference flash_area. If
 * @p ID is unknown, it will be NULL on output.
 *
 * @return  0 on success, -EACCES if the flash_map is not available ,
 * -ENOENT if @p ID is unknown, -ENODEV if there is no driver attached
 * to the area.
 */
int flash_area_open(uint8_t id, const struct flash_area **fa);

/**
 * @brief Close flash_area
 *
 * Reserved for future usage and external projects compatibility reason.
 * Currently is NOP.
 *
 * @param[in] fa Flash area to be closed.
 */
void flash_area_close(const struct flash_area *fa);

/**
 * @brief Read flash area data
 *
 * Read data from flash area. Area readout boundaries are asserted before read
 * request. API has the same limitation regard read-block alignment and size
 * as wrapped flash driver.
 *
 * @param[in]  fa  Flash area
 * @param[in]  off Offset relative from beginning of flash area to read
 * @param[out] dst Buffer to store read data
 * @param[in]  len Number of bytes to read
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_area_read(const struct flash_area *fa, off_t off, void *dst,
		    size_t len);

/**
 * @brief Write data to flash area
 *
 * Write data to flash area. Area write boundaries are asserted before write
 * request. API has the same limitation regard write-block alignment and size
 * as wrapped flash driver.
 *
 * @param[in]  fa  Flash area
 * @param[in]  off Offset relative from beginning of flash area to read
 * @param[out] src Buffer with data to be written
 * @param[in]  len Number of bytes to write
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_area_write(const struct flash_area *fa, off_t off, const void *src,
		     size_t len);

/**
 * @brief Erase flash area
 *
 * Erase given flash area range. Area boundaries are asserted before erase
 * request. API has the same limitation regard erase-block alignment and size
 * as wrapped flash driver.
 *
 * @param[in] fa  Flash area
 * @param[in] off Offset relative from beginning of flash area.
 * @param[in] len Number of bytes to be erase
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_area_erase(const struct flash_area *fa, off_t off, size_t len);

/**
 * @brief Get write block size of the flash area
 *
 * Currently write block size might be treated as read block size, although
 * most of drivers supports unaligned readout.
 *
 * @param[in] fa Flash area
 *
 * @return Alignment restriction for flash writes in [B].
 */
uint32_t flash_area_align(const struct flash_area *fa);

/**
 * Retrieve info about sectors within the area.
 *
 * @param[in]  fa_id    Given flash area ID
 * @param[out] sectors  buffer for sectors data
 * @param[in,out] count On input Capacity of @p sectors, on output number of
 * sectors Retrieved.
 *
 * @return  0 on success, negative errno code on fail. Especially returns
 * -ENOMEM if There are too many flash pages on the flash_area to fit in the
 * array.
 */
int flash_area_get_sectors(int fa_id, uint32_t *count,
			   struct flash_sector *sectors);

/**
 * Flash map iteration callback
 *
 * @param fa flash area
 * @param user_data User supplied data
 *
 */
typedef void (*flash_area_cb_t)(const struct flash_area *fa,
				void *user_data);

/**
 * Iterate over flash map
 *
 * @param user_cb User callback
 * @param user_data User supplied data
 */
void flash_area_foreach(flash_area_cb_t user_cb, void *user_data);

/**
 * Check whether given flash area has supporting flash driver
 * in the system.
 *
 * @param[in] fa Flash area.
 *
 * @return 1 On success. -ENODEV if no driver match.
 */
int flash_area_has_driver(const struct flash_area *fa);

/**
 * Get driver for given flash area.
 *
 * @param fa Flash area.
 *
 * @return device driver.
 */
const struct device *flash_area_get_device(const struct flash_area *fa);

/**
 * Get the value expected to be read when accessing any erased
 * flash byte.
 * This API is compatible with the MCUBoot's porting layer.
 *
 * @param fa Flash area.
 *
 * @return Byte value of erase memory.
 */
uint8_t flash_area_erased_val(const struct flash_area *fa);

#define FLASH_AREA_LABEL_EXISTS(label) __DEPRECATED_MACRO \
	DT_HAS_FIXED_PARTITION_LABEL(label)

#define FLASH_AREA_LABEL_STR(lbl) __DEPRECATED_MACRO \
	DT_PROP(DT_NODE_BY_FIXED_PARTITION_LABEL(lbl), label)

#define FLASH_AREA_ID(label) __DEPRECATED_MACRO \
	DT_FIXED_PARTITION_ID(DT_NODE_BY_FIXED_PARTITION_LABEL(label))

#define FLASH_AREA_OFFSET(label) __DEPRECATED_MACRO \
	DT_REG_ADDR(DT_NODE_BY_FIXED_PARTITION_LABEL(label))

#define FLASH_AREA_SIZE(label) __DEPRECATED_MACRO \
	DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(label))

/**
 * Returns non-0 value if fixed-partition of given DTS node label exists.
 *
 * @param label DTS node label
 *
 * @return non-0 if fixed-partition node exists and is enabled;
 *	   0 if node does not exist, is not enabled or is not fixed-partition.
 */
#define FIXED_PARTITION_EXISTS(label) DT_FIXED_PARTITION_EXISTS(DT_NODELABEL(label))

/**
 * Get flash area ID from fixed-partition DTS node label
 *
 * @param label DTS node label of a partition
 *
 * @return flash area ID
 */
#define FIXED_PARTITION_ID(label) DT_FIXED_PARTITION_ID(DT_NODELABEL(label))

/**
 * Get fixed-partition offset from DTS node label
 *
 * @param label DTS node label of a partition
 *
 * @return fixed-partition offset, as defined for the partition in DTS.
 */
#define FIXED_PARTITION_OFFSET(label) DT_REG_ADDR(DT_NODELABEL(label))

/**
 * Get fixed-partition size for DTS node label
 *
 * @param label DTS node label
 *
 * @return fixed-partition offset, as defined for the partition in DTS.
 */
#define FIXED_PARTITION_SIZE(label) DT_REG_SIZE(DT_NODELABEL(label))

/**
 * Get device pointer for device the area/partition resides on
 *
 * @param label DTS node label of a partition
 *
 * @return const struct device type pointer
 */
#define FLASH_AREA_DEVICE(label) \
	DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(DT_NODE_BY_FIXED_PARTITION_LABEL(label)))

/**
 * Get device pointer for device the area/partition resides on
 *
 * @param label DTS node label of a partition
 *
 * @return Pointer to a device.
 */
#define FIXED_PARTITION_DEVICE(label) \
	DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(DT_NODELABEL(label)))

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_H_ */
