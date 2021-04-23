/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
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
#include <zephyr.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <devicetree.h>
#include <storage/flash_map_phase_out.h>

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
	const struct device *fa_dev;
	off_t fa_off;
	size_t fa_size;
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
uint8_t flash_area_align(const struct flash_area *fa);

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
 * Get the value expected to be read when accessing any erased
 * flash byte.
 * This API is compatible with the MCUBoot's porting layer.
 *
 * @param fa Flash area.
 *
 * @return Byte value of erase memory.
 */
uint8_t flash_area_erased_val(const struct flash_area *fa);

#define FLASH_AREA_LABEL_EXISTS(label) \
	DT_HAS_FIXED_PARTITION_LABEL(label)

#define FLASH_AREA_LABEL_STR(lbl) \
	DT_PROP(DT_NODE_BY_FIXED_PARTITION_LABEL(lbl), label)

/**
 * Obtain pointer to flash_area objet directly from DTS node of fixed partition.
 *
 * @param[in] label Fixed partition node to use; for example:
 *	      DT_N_S_flash_controller_0_S_flash_0_S_partitions_S_partition_fc000
 *
 * @return const struct flash_area * to flash_area object will fail compilation in case when
 *	   generating object pointer will not be possible.
 */
#define FLASH_AREA_NODE(node) (&_CONCAT(_flash_map_area_, DT_FIXED_PARTITION_ID(node)))

/**
 * Obtain pointer to flash_area objet directly from DTS label of fixed partition.
 *
 * @param[in] label Fixed partition label to use; this C identifier type label, which means that
 *	      instead of, for example, image-0, image_0 should be provided.
 *
 * @return const struct flash_area * to flash_area object will fail compilation in case when
 *	   generating object pointer will not be possible.
 */
#define FLASH_AREA(label) FLASH_AREA_NODE(DT_NODE_BY_FIXED_PARTITION_LABEL(label))

/**
 * Obtain identifier of flash_area from DTS label.
 *
 * @param[in] label Fixed partition label to use; this C identifier type label, which means that
 *	      instead of, for example, image-0, image_0 should be provided.
 *
 * @return Identifier of fixed partition that has been assigned to the partition with given label.
 */
#define FLASH_AREA_ID(label) DT_FIXED_PARTITION_ID(DT_NODE_BY_FIXED_PARTITION_LABEL(label))

#define FLASH_AREA_OFFSET(label) \
	DT_REG_ADDR(DT_NODE_BY_FIXED_PARTITION_LABEL(label))

#define FLASH_AREA_SIZE(label) \
	DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(label))

/**
 * Define flash_area type object and place within _flash_map linker section.
 *
 * Usage: DEFINE_FLASH_AREA(some) = { ... };
 */
#define DEFINE_FLASH_AREA(name)									\
	Z_DECL_ALIGN(const struct flash_area) _CONCAT(_flash_map_area_, name)			\
		__in_section(_flash_map, static, _CONCAT(_flash_map_area_, name)) __used;	\
	const struct flash_area _CONCAT(_flash_map_area_, name)

/**
 * Declares flash_area type object for access within code.
 *
 * Usage: DECLARE_FLASH_AREA(some);
 */
#define DECLARE_FLASH_AREA(name) const extern struct flash_area _CONCAT(_flash_map_area_, name)


#define DT_FLASH_AREA_FOREACH_OKAY_FIXED_PARTITION(fn)				\
		    UTIL_CAT(DT_FOREACH_OKAY_INST_, fixed_partitions)(fn)

/* Declare all known partitions generated from DTS */
#define DECLARE_FLASH_AREA_FOO(part) DECLARE_FLASH_AREA(DT_FIXED_PARTITION_ID(part));
#define FOREACH_AREA(n) \
	DT_FOREACH_CHILD_STATUS_OKAY(DT_INST(n, fixed_partitions), DECLARE_FLASH_AREA_FOO)
DT_FLASH_AREA_FOREACH_OKAY_FIXED_PARTITION(FOREACH_AREA)
#undef DECLARE_FLASH_AREA_FOO
#undef FOREACH_AREA

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_H_ */
