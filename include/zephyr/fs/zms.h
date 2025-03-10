/* Copyright (c) 2018 Laczen
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ZMS: Zephyr Memory Storage
 */
#ifndef ZEPHYR_INCLUDE_FS_ZMS_H_
#define ZEPHYR_INCLUDE_FS_ZMS_H_

#include <sys/types.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup zms Zephyr Memory Storage (ZMS)
 * @ingroup file_system_storage
 * @{
 * @}
 */

/**
 * @defgroup zms_data_structures ZMS data structures
 * @ingroup zms
 * @{
 */

/** Zephyr Memory Storage file system structure */
struct zms_fs {
	/** File system offset in flash */
	off_t offset;
	/** Allocation Table Entry (ATE) write address.
	 * Addresses are stored as `uint64_t`:
	 * - high 4 bytes correspond to the sector
	 * - low 4 bytes are the offset in the sector
	 */
	uint64_t ate_wra;
	/** Data write address */
	uint64_t data_wra;
	/** Storage system is split into sectors. The sector size must be a multiple of
	 *  `erase-block-size` if the device has erase capabilities
	 */
	uint32_t sector_size;
	/** Number of sectors in the file system */
	uint32_t sector_count;
	/** Current cycle counter of the active sector (pointed to by `ate_wra`) */
	uint8_t sector_cycle;
	/** Flag indicating if the file system is initialized */
	bool ready;
	/** Mutex used to lock flash writes */
	struct k_mutex zms_lock;
	/** Flash device runtime structure */
	const struct device *flash_device;
	/** Flash memory parameters structure */
	const struct flash_parameters *flash_parameters;
	/** Size of an Allocation Table Entry */
	size_t ate_size;
#if CONFIG_ZMS_LOOKUP_CACHE
	/** Lookup table used to cache ATE addresses of written IDs */
	uint64_t lookup_cache[CONFIG_ZMS_LOOKUP_CACHE_SIZE];
#endif
};

/**
 * @}
 */

/**
 * @defgroup zms_high_level_api ZMS API
 * @ingroup zms
 * @{
 */

/**
 * @brief Mount a ZMS file system onto the device specified in `fs`.
 *
 * @param fs Pointer to the file system.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the detected file system is not ZMS.
 * @retval -EPROTONOSUPPORT if the ZMS version is not supported.
 * @retval -EINVAL if any of the flash parameters or the sector layout is invalid.
 * @retval -ENXIO if there is a device error.
 * @retval -EIO if there is a memory read/write error.
 */
int zms_mount(struct zms_fs *fs);

/**
 * @brief Clear the ZMS file system from device.
 *
 * @param fs Pointer to the file system.
 *
 * @retval 0 on success.
 * @retval -EACCES if `fs` is not mounted.
 * @retval -ENXIO if there is a device error.
 * @retval -EIO if there is a memory read/write error.
 */
int zms_clear(struct zms_fs *fs);

/**
 * @brief Write an entry to the file system.
 *
 * @note  When the `len` parameter is equal to `0` the entry is effectively removed (it is
 * equivalent to calling @ref zms_delete()). It is not possible to distinguish between a deleted
 * entry and an entry with data of length 0.
 *
 * @param fs Pointer to the file system.
 * @param id ID of the entry to be written.
 * @param data Pointer to the data to be written.
 * @param len Number of bytes to be written (maximum 64 KiB).
 *
 * @return Number of bytes written. On success, it will be equal to the number of bytes requested
 * to be written or 0.
 * When a rewrite of the same data already stored is attempted, nothing is written to flash,
 * thus 0 is returned. On error, returns negative value of error codes defined in `errno.h`.
 * @retval Number of bytes written (`len` or 0) on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -ENXIO if there is a device error.
 * @retval -EIO if there is a memory read/write error.
 * @retval -EINVAL if `len` is invalid.
 * @retval -ENOSPC if no space is left on the device.
 */
ssize_t zms_write(struct zms_fs *fs, uint32_t id, const void *data, size_t len);

/**
 * @brief Delete an entry from the file system
 *
 * @param fs Pointer to the file system.
 * @param id ID of the entry to be deleted.
 *
 * @retval 0 on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -ENXIO if there is a device error.
 * @retval -EIO if there is a memory read/write error.
 */
int zms_delete(struct zms_fs *fs, uint32_t id);

/**
 * @brief Read an entry from the file system.
 *
 * @param fs Pointer to the file system.
 * @param id ID of the entry to be read.
 * @param data Pointer to data buffer.
 * @param len Number of bytes to read at most.
 *
 * @return Number of bytes read. On success, it will be equal to the number of bytes requested
 * to be read or less than that if the stored data has a smaller size than the requested one.
 * On error, returns negative value of error codes defined in `errno.h`.
 * @retval Number of bytes read (> 0) on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -EIO if there is a memory read/write error.
 * @retval -ENOENT if there is no entry with the given `id`.
 */
ssize_t zms_read(struct zms_fs *fs, uint32_t id, void *data, size_t len);

/**
 * @brief Read a history entry from the file system.
 *
 * @param fs Pointer to the file system.
 * @param id ID of the entry to be read.
 * @param data Pointer to data buffer.
 * @param len Number of bytes to be read.
 * @param cnt History counter: 0: latest entry, 1: one before latest ...
 *
 * @return Number of bytes read. On success, it will be equal to the number of bytes requested
 * to be read. When the return value is larger than the number of bytes requested to read this
 * indicates not all bytes were read, and more data is available. On error, returns negative
 * value of error codes defined in `errno.h`.
 * @retval Number of bytes read (> 0) on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -EIO if there is a memory read/write error.
 * @retval -ENOENT if there is no entry with the given `id` and history counter.
 */
ssize_t zms_read_hist(struct zms_fs *fs, uint32_t id, void *data, size_t len, uint32_t cnt);

/**
 * @brief Gets the length of the data that is stored in an entry with a given `id`
 *
 * @param fs Pointer to the file system.
 * @param id ID of the entry whose data length to retrieve.
 *
 * @return Data length contained in the ATE. On success, it will be equal to the number of bytes
 * in the ATE. On error, returns negative value of error codes defined in `errno.h`.
 * @retval Length of the entry with the given `id` (> 0) on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -EIO if there is a memory read/write error.
 * @retval -ENOENT if there is no entry with the given id and history counter.
 */
ssize_t zms_get_data_length(struct zms_fs *fs, uint32_t id);

/**
 * @brief Calculate the available free space in the file system.
 *
 * @param fs Pointer to the file system.
 *
 * @return Number of free bytes. On success, it will be equal to the number of bytes that can
 * still be written to the file system.
 * Calculating the free space is a time-consuming operation, especially on SPI flash.
 * On error, returns negative value of error codes defined in `errno.h`.
 * @retval Number of free bytes (>= 0) on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -EIO if there is a memory read/write error.
 */
ssize_t zms_calc_free_space(struct zms_fs *fs);

/**
 * @brief Tell how much contiguous free space remains in the currently active ZMS sector.
 *
 * @param fs Pointer to the file system.
 *
 * @retval >=0 Number of free bytes in the currently active sector
 * @retval -EACCES if ZMS is still not initialized.
 */
size_t zms_active_sector_free_space(struct zms_fs *fs);

/**
 * @brief Close the currently active sector and switch to the next one.
 *
 * @note The garbage collector is called on the new sector.
 *
 * @warning This routine is made available for specific use cases.
 * It collides with ZMS's goal of avoiding any unnecessary flash erase operations.
 * Using this routine extensively can result in premature failure of the flash device.
 *
 * @param fs Pointer to the file system.
 *
 * @retval 0 on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -EIO if there is a memory read/write error.
 */
int zms_sector_use_next(struct zms_fs *fs);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_ZMS_H_ */
