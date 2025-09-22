/*
 * Copyright (c) 2017-2024 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2023 Sensorfy B.V.
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
 * @since 1.11
 * @version 1.0.0
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
#if CONFIG_FLASH_MAP_LABELS
	/** Partition label if defined in DTS. Otherwise nullptr; */
	const char *fa_label;
#endif
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
 * @brief Verify that a device assigned to flash area is ready for use.
 *
 * Indicates whether the provided flash area has a device known to be
 * in a state where it can be used with Flash Map API.
 *
 * This can be used with struct flash_area pointers captured from FIXED_PARTITION().
 * At minimum this means that the device has been successfully initialized.
 *
 * @param fa pointer to flash_area object to check.
 *
 * @retval true If the device is ready for use.
 * @retval false If the device is not ready for use or if a NULL pointer is
 * passed as flash area pointer or device pointer within flash area object
 * is NULL.
 */
static ALWAYS_INLINE bool flash_area_device_is_ready(const struct flash_area *fa)
{
	return (fa != NULL && device_is_ready(fa->fa_dev));
}

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
 * @param[in]  off Offset relative from beginning of flash area to write
 * @param[in]  src Buffer with data to be written
 * @param[in]  len Number of bytes to write
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_area_write(const struct flash_area *fa, off_t off, const void *src,
		     size_t len);

/**
 * @brief Copy flash memory from one flash area to another.
 *
 * Copy data to flash area. Area boundaries are asserted before copy
 * request.
 *
 * For more information, see flash_copy().
 *
 * @param[in]  src_fa  Source Flash area
 * @param[in]  src_off Offset relative from beginning of source flash area.
 * @param[in]  dst_fa  Destination Flash area
 * @param[in]  dst_off Offset relative from beginning of destination flash area.
 * @param[in]  len Number of bytes to copy, in bytes.
 * @param[out] buf Pointer to a buffer of size @a buf_size.
 * @param[in]  buf_size Size of the buffer pointed to by @a buf.
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_area_copy(const struct flash_area *src_fa, off_t src_off,
		    const struct flash_area *dst_fa, off_t dst_off,
		    off_t len, uint8_t *buf, size_t buf_size);

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
 * @brief Erase flash area or fill with erase-value
 *
 * On program-erase devices this function behaves exactly like flash_area_erase.
 * On RAM non-volatile device it will call erase, if driver provides such
 * callback, or will fill given range with erase-value defined by driver.
 * This function should be only used by code that has not been written
 * to directly support devices that do not require erase and rely on
 * device being erased prior to some operations.
 * Note that emulated erase, on devices that do not require, is done
 * via write, which affects endurance of device.
 *
 * @see flash_area_erase()
 * @see flash_flatten()
 *
 * @param[in] fa  Flash area
 * @param[in] off Offset relative from beginning of flash area.
 * @param[in] len Number of bytes to be erase
 *
 * @return  0 on success, negative errno code on fail.
 */
int flash_area_flatten(const struct flash_area *fa, off_t off, size_t len);

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
 * @param[in,out] count On input Capacity of @p sectors, on output number of
 * sectors Retrieved.
 * @param[out] sectors  buffer for sectors data
 *
 * @return  0 on success, negative errno code on fail. Especially returns
 * -ENOMEM if There are too many flash pages on the flash_area to fit in the
 * array.
 */
int flash_area_get_sectors(int fa_id, uint32_t *count,
			   struct flash_sector *sectors);

/**
 * Retrieve info about sectors within the area.
 *
 * @param[in]  fa       pointer to flash area object.
 * @param[in,out] count On input Capacity of @p sectors, on output number of
 * sectors retrieved.
 * @param[out] sectors  buffer for sectors data
 *
 * @return  0 on success, negative errno code on fail. Especially returns
 * -ENOMEM if There are too many flash pages on the flash_area to fit in the
 * array.
 */
int flash_area_sectors(const struct flash_area *fa, uint32_t *count, struct flash_sector *sectors);

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
 * @param[in] fa Flash area.
 *
 * @return device driver.
 */
const struct device *flash_area_get_device(const struct flash_area *fa);

#if CONFIG_FLASH_MAP_LABELS
/**
 * Get the label property from the device tree
 *
 * @param[in] fa Flash area.
 *
 * @return The label property if it is defined, otherwise NULL
 */
const char *flash_area_label(const struct flash_area *fa);
#endif

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
 * Get fixed-partition offset from DTS node
 *
 * @param node DTS node of a partition
 *
 * @return fixed-partition offset, as defined for the partition in DTS.
 */
#define FIXED_PARTITION_NODE_OFFSET(node) DT_REG_ADDR(node)

/**
 * Get fixed-partition size for DTS node label
 *
 * @param label DTS node label
 *
 * @return fixed-partition offset, as defined for the partition in DTS.
 */
#define FIXED_PARTITION_SIZE(label) DT_REG_SIZE(DT_NODELABEL(label))

/**
 * Get fixed-partition size for DTS node
 *
 * @param node DTS node of a partition
 *
 * @return fixed-partition size, as defined for the partition in DTS.
 */
#define FIXED_PARTITION_NODE_SIZE(node) DT_REG_SIZE(node)

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

/**
 * Get device pointer for device the area/partition resides on
 *
 * @param node DTS node of a partition
 *
 * @return Pointer to a device.
 */
#define FIXED_PARTITION_NODE_DEVICE(node) \
	DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(node))

/**
 * Get pointer to flash_area object by partition label
 *
 * @param label DTS node label of a partition
 *
 * @return Pointer to flash_area type object representing partition
 */
#define FIXED_PARTITION(label)	FIXED_PARTITION_1(DT_NODELABEL(label))

/**
 * Get pointer to flash_area object by partition node in DTS
 *
 * @param node DTS node of a partition
 *
 * @return Pointer to flash_area type object representing partition
 */
#define FIXED_PARTITION_BY_NODE(node)	FIXED_PARTITION_1(node)

/** @cond INTERNAL_HIDDEN */
#define FIXED_PARTITION_1(node)	FIXED_PARTITION_0(DT_DEP_ORD(node))
#define FIXED_PARTITION_0(ord)							\
	((const struct flash_area *)&DT_CAT(global_fixed_partition_ORD_, ord))

#define DECLARE_PARTITION(node) DECLARE_PARTITION_0(DT_DEP_ORD(node))
#define DECLARE_PARTITION_0(ord)						\
	extern const struct flash_area DT_CAT(global_fixed_partition_ORD_, ord);
#define FOR_EACH_PARTITION_TABLE(table) DT_FOREACH_CHILD(table, DECLARE_PARTITION)

/* Generate declarations */
DT_FOREACH_STATUS_OKAY(fixed_partitions, FOR_EACH_PARTITION_TABLE)

#undef DECLARE_PARTITION
#undef DECLARE_PARTITION_0
#undef FOR_EACH_PARTITION_TABLE
/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_H_ */
