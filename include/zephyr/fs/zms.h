/*  ZMS: Zephyr Memory Storage
 *
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_FS_ZMS_H_
#define ZEPHYR_INCLUDE_FS_ZMS_H_

#include <zephyr/drivers/flash.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zephyr Memory Storage (ZMS)
 * @defgroup zms Zephyr Memory Storage (ZMS)
 * @ingroup file_system_storage
 * @{
 * @}
 */

/**
 * @brief Zephyr Memory Storage Data Structures
 * @defgroup zms_data_structures Zephyr Memory Storage Data Structures
 * @ingroup zms
 * @{
 */

/**
 * @brief Zephyr Memory Storage File system structure
 */
struct zms_fs {
	/** File system offset in flash **/
	off_t offset;
	/** Allocation table entry write address.
	 * Addresses are stored as uint64_t:
	 * - high 4 bytes correspond to the sector
	 * - low 4 bytes are the offset in the sector
	 */
	uint64_t ate_wra;
	/** Data write address */
	uint64_t data_wra;
	/** Storage system is split into sectors, each sector size must be multiple of erase-blocks
	 *  if the device has erase capabilities
	 */
	uint32_t sector_size;
	/** Number of sectors in the file system */
	uint32_t sector_count;
	/** Current cycle counter of the active sector (pointed by ate_wra)*/
	uint8_t sector_cycle;
	/** Flag indicating if the file system is initialized */
	bool ready;
	/** Mutex */
	struct k_mutex zms_lock;
	/** Flash device runtime structure */
	const struct device *flash_device;
	/** Flash memory parameters structure */
	const struct flash_parameters *flash_parameters;
	/** Size of an Allocation Table Entry */
	size_t ate_size;
#if CONFIG_ZMS_LOOKUP_CACHE
	/** Lookup table used to cache ATE address of a written ID */
	uint64_t lookup_cache[CONFIG_ZMS_LOOKUP_CACHE_SIZE];
#endif
};

/**
 * @}
 */

/**
 * @brief Zephyr Memory Storage APIs
 * @defgroup zms_high_level_api Zephyr Memory Storage APIs
 * @ingroup zms
 * @{
 */

/**
 * @brief Mount a ZMS file system onto the device specified in @p fs.
 *
 * @param fs Pointer to file system
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zms_mount(struct zms_fs *fs);

/**
 * @brief Clear the ZMS file system from device.
 *
 * @param fs Pointer to file system
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zms_clear(struct zms_fs *fs);

/**
 * @brief Write an entry to the file system.
 *
 * @note  When @p len parameter is equal to @p 0 then entry is effectively removed (it is
 * equivalent to calling of zms_delete). It is not possible to distinguish between a deleted
 * entry and an entry with data of length 0.
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be written
 * @param data Pointer to the data to be written
 * @param len Number of bytes to be written (maximum 64 KB)
 *
 * @return Number of bytes written. On success, it will be equal to the number of bytes requested
 * to be written. When a rewrite of the same data already stored is attempted, nothing is written
 * to flash, thus 0 is returned. On error, returns negative value of errno.h defined error codes.
 */
ssize_t zms_write(struct zms_fs *fs, uint32_t id, const void *data, size_t len);

/**
 * @brief Delete an entry from the file system
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be deleted
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zms_delete(struct zms_fs *fs, uint32_t id);

/**
 * @brief Read an entry from the file system.
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be read
 * @param data Pointer to data buffer
 * @param len Number of bytes to be read (or size of the allocated read buffer)
 *
 * @return Number of bytes read. On success, it will be equal to the number of bytes requested
 * to be read. When the return value is less than the number of bytes requested to read this
 * indicates that ATE contain less data than requested. On error, returns negative value of
 * errno.h defined error codes.
 */
ssize_t zms_read(struct zms_fs *fs, uint32_t id, void *data, size_t len);

/**
 * @brief Read a history entry from the file system.
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be read
 * @param data Pointer to data buffer
 * @param len Number of bytes to be read
 * @param cnt History counter: 0: latest entry, 1: one before latest ...
 *
 * @return Number of bytes read. On success, it will be equal to the number of bytes requested
 * to be read. When the return value is larger than the number of bytes requested to read this
 * indicates not all bytes were read, and more data is available. On error, returns negative
 * value of errno.h defined error codes.
 */
ssize_t zms_read_hist(struct zms_fs *fs, uint32_t id, void *data, size_t len, uint32_t cnt);

/**
 * @brief Gets the data size that is stored in an entry with a given id
 *
 * @param fs Pointer to file system
 * @param id Id of the entry that we want to get its data length
 *
 * @return Data length contained in the ATE. On success, it will be equal to the number of bytes
 * in the ATE. On error, returns negative value of errno.h defined error codes.
 */
ssize_t zms_get_data_length(struct zms_fs *fs, uint32_t id);
/**
 * @brief Calculate the available free space in the file system.
 *
 * @param fs Pointer to file system
 *
 * @return Number of bytes free. On success, it will be equal to the number of bytes that can
 * still be written to the file system.
 * Calculating the free space is a time consuming operation, especially on spi flash.
 * On error, returns negative value of errno.h defined error codes.
 */
ssize_t zms_calc_free_space(struct zms_fs *fs);

/**
 * @brief Tell how many contiguous free space remains in the currently active ZMS sector.
 *
 * @param fs Pointer to the file system.
 *
 * @return Number of free bytes.
 */
size_t zms_sector_max_data_size(struct zms_fs *fs);

/**
 * @brief Close the currently active sector and switch to the next one.
 *
 * @note The garbage collector is called on the new sector.
 *
 * @warning This routine is made available for specific use cases.
 * It collides with the ZMS goal of avoiding any unnecessary flash erase operations.
 * Using this routine extensively can result in premature failure of the flash device.
 *
 * @param fs Pointer to the file system.
 *
 * @return 0 on success. On error, returns negative value of errno.h defined error codes.
 */
int zms_sector_use_next(struct zms_fs *fs);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_ZMS_H_ */
