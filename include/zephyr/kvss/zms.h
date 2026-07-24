/* Copyright (c) 2018 Laczen
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ZMS: Zephyr Memory Storage
 */
#ifndef ZEPHYR_INCLUDE_KVSS_ZMS_H_
#define ZEPHYR_INCLUDE_KVSS_ZMS_H_

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

/** Mount options for @ref zms_mount and @ref zms_mount_force. */
enum zms_mount_flags {
	/**
	 * Do not format erased media during mount.
	 *
	 * When this flag is set and the backing flash area is erased (no valid ZMS
	 * header), mount fails instead of creating a new ZMS header.
	 */
	ZMS_MOUNT_FLAG_NO_FORMAT = BIT(0),
};

/**
 * @brief ID type used in the ZMS API.
 *
 * @note The width of this type depends on @kconfig{CONFIG_ZMS_ID_64BIT}.
 */
#if CONFIG_ZMS_ID_64BIT
typedef uint64_t zms_id_t;
#else
typedef uint32_t zms_id_t;
#endif

/** Default minimum ID accepted by iterator range filtering. */
#define ZMS_ITER_ID_MIN ((zms_id_t)0)

/** Default maximum ID accepted by iterator range filtering. */
#define ZMS_ITER_ID_MAX ((zms_id_t)~(zms_id_t)0)

/** Default mask that accepts all IDs during iterator filtering. */
#define ZMS_ITER_MASK_ALL ZMS_ITER_ID_MAX

/** Iterator predicate callback type used by @ref zms_iter_init_with_config. */
typedef bool (*zms_iter_predicate_t)(zms_id_t id);

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
	/** Mount behavior flags from @ref zms_mount_flags */
	uint32_t mount_flags;
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
#if CONFIG_ZMS_LOOKUP_CACHE_MANUAL
	/** Lookup table used to cache ATE addresses of written IDs */
	uint64_t *lookup_cache;
	/** Size of the lookup cache */
	size_t lookup_cache_size;
#else
	/** Lookup table used to cache ATE addresses of written IDs */
	uint64_t lookup_cache[CONFIG_ZMS_LOOKUP_CACHE_SIZE];
#endif
#endif
};

/**
 * @brief Iterator configuration for @ref zms_iter_init_with_config.
 *
 * Leave @ref use_mask, @ref use_range, and @ref use_predicate disabled to use
 * the default behavior:
 * all IDs in the full @ref zms_id_t range are accepted.
 */
struct zms_iter_config {
	/** Mask applied when @ref use_mask is true. */
	zms_id_t mask_id;
	/** Minimum accepted ID when @ref use_range is true. */
	zms_id_t min_id;
	/** Maximum accepted ID when @ref use_range is true. */
	zms_id_t max_id;
	/** Enable mask-based filtering. */
	bool use_mask;
	/** Enable inclusive range-based filtering. */
	bool use_range;
	/** Enable callback-based filtering. */
	bool use_predicate;
	/** Predicate callback used when @ref use_predicate is true. */
	zms_iter_predicate_t predicate_func;
};

/** Default initializer for @ref zms_iter_config. */
#define ZMS_ITER_CONFIG_DEFAULT \
	{ \
		.mask_id = ZMS_ITER_MASK_ALL, \
		.min_id = ZMS_ITER_ID_MIN, \
		.max_id = ZMS_ITER_ID_MAX, \
		.use_mask = false, \
		.use_range = false, \
		.use_predicate = false, \
		.predicate_func = NULL, \
	}

/**
 * @brief Opaque iterator state for @ref zms_iter_next and @ref zms_iter_next_all.
 *
 * Must be initialised with @ref zms_iter_init or @ref zms_iter_init_with_config
 * before use.
 * Do not access fields directly.
 */
struct zms_iter {
	/**  Current walk address in the ATE ring */
	uint64_t walk_addr;
	/**  Snapshot of ate_wra taken at iterator initialization time */
	uint64_t end_addr;
	/**  ID mask used to filter returned entries */
	zms_id_t mask_id;
	/**  Inclusive minimum ID accepted by the iterator */
	zms_id_t min_id;
	/**  Inclusive maximum ID accepted by the iterator */
	zms_id_t max_id;
	/**  Optional callback used to filter IDs */
	zms_iter_predicate_t predicate_func;
	/**  True once the walk has completed the full ring */
	bool exhausted;
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
 * @details If the flash area is erased and no valid ZMS header is found,
 * mount will format the area and create a valid header by default.
 * Set @ref ZMS_MOUNT_FLAG_NO_FORMAT in `fs->mount_flags` to disable this
 * auto-format behavior and fail the mount instead.
 *
 * @param fs Pointer to the file system.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the detected file system is not ZMS.
 * @retval -EPROTONOSUPPORT if the ZMS version is not supported.
 * @retval -EINVAL if `fs` is NULL or any of the flash parameters or the sector layout is invalid.
 * @retval -ENXIO if there is a device error.
 * @retval -EIO if there is a memory read/write error.
 */
int zms_mount(struct zms_fs *fs);

/**
 * @brief Mount a ZMS file system onto the device specified in `fs`, wiping the partition if
 * mounting fails the first time.
 *
 * @param fs Pointer to the file system.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the detected file system is not ZMS.
 * @retval -EPROTONOSUPPORT if the ZMS version is not supported.
 * @retval -EINVAL if `fs` is NULL or any of the flash parameters or the sector layout is invalid.
 * @retval -ENXIO if there is a device error.
 * @retval -EIO if there is a memory read/write error.
 */
int zms_mount_force(struct zms_fs *fs);

/**
 * @brief Clear the ZMS file system from device. The ZMS file system must be re-mounted after this
 * operation.
 *
 * @param fs Pointer to the file system.
 *
 * @retval 0 on success.
 * @retval -EACCES if `fs` is not mounted.
 * @retval -ENXIO if there is a device error.
 * @retval -EIO if there is a memory read/write error.
 * @retval -EINVAL if `fs` is NULL.
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
 * @return Number of bytes written (`len` or 0) on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -ENXIO if there is a device error.
 * @retval -EIO if there is a memory read/write error.
 * @retval -EINVAL if `fs` is NULL or `len` is invalid.
 * @retval -ENOSPC if no space is left on the device.
 */
ssize_t zms_write(struct zms_fs *fs, zms_id_t id, const void *data, size_t len);

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
 * @retval -EINVAL if `fs` is NULL.
 */
int zms_delete(struct zms_fs *fs, zms_id_t id);

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
 * @return Number of bytes read (> 0) on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -EIO if there is a memory read/write error.
 * @retval -ENOENT if there is no entry with the given `id`.
 * @retval -EINVAL if `fs` is NULL.
 */
ssize_t zms_read(struct zms_fs *fs, zms_id_t id, void *data, size_t len);

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
 * @return Number of bytes read (> 0) on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -EIO if there is a memory read/write error.
 * @retval -ENOENT if there is no entry with the given `id` and history counter.
 * @retval -EINVAL if `fs` is NULL.
 */
ssize_t zms_read_hist(struct zms_fs *fs, zms_id_t id, void *data, size_t len, uint32_t cnt);

/**
 * @brief Gets the length of the data that is stored in an entry with a given `id`
 *
 * @param fs Pointer to the file system.
 * @param id ID of the entry whose data length to retrieve.
 *
 * @return Data length contained in the ATE. On success, it will be equal to the number of bytes
 * in the ATE. On error, returns negative value of error codes defined in `errno.h`.
 * @return Length of the entry with the given `id` (> 0) on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -EIO if there is a memory read/write error.
 * @retval -ENOENT if there is no entry with the given id.
 * @retval -EINVAL if `fs` is NULL.
 */
ssize_t zms_get_data_length(struct zms_fs *fs, zms_id_t id);

/**
 * @brief Calculate the available free space in the file system.
 *
 * @param fs Pointer to the file system.
 *
 * @return Number of free bytes. On success, it will be equal to the number of bytes that can
 * still be written to the file system.
 * Calculating the free space is a time-consuming operation, especially on SPI flash.
 * On error, returns negative value of error codes defined in `errno.h`.
 * @return Number of free bytes (>= 0) on success.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -EIO if there is a memory read/write error.
 * @retval -EINVAL if `fs` is NULL.
 */
ssize_t zms_calc_free_space(struct zms_fs *fs);

/**
 * @brief Tell how much contiguous free space remains in the currently active ZMS sector.
 *
 * @param fs Pointer to the file system.
 *
 * @retval >=0 Number of free bytes in the currently active sector. On success, it will be equal
 * to the number of bytes that can be written without automatically advancing to the next sector.
 * @retval -EACCES if ZMS is still not initialized.
 * @retval -EINVAL if `fs` is NULL.
 */
ssize_t zms_active_sector_free_space(struct zms_fs *fs);

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
 * @retval -EINVAL if `fs` is NULL.
 */
int zms_sector_use_next(struct zms_fs *fs);

#if CONFIG_ZMS_LOOKUP_CACHE_MANUAL
/**
 * @brief Assign a buffer for the lookup cache for a ZMS instance.
 *
 * This must be called before zms_mount(). If not called, the cache
 * will be disabled for this file system instance.
 *
 * @param fs Pointer to the file system.
 * @param buffer Pointer to the buffer to be used for the cache.
 * @param size The number of entries in the buffer.
 *
 * @retval 0 on success.
 * @retval -EINVAL if `fs` or `buffer` are NULL, or `size` is 0.
 * @retval -EBUSY if ZMS is already mounted.
 */
int zms_set_lookup_cache(struct zms_fs *fs, uint64_t *buffer, size_t size);
#endif

/**
 * @brief Return the maximum sector recycle count across all sectors.
 *
 * Iterates all sectors and stores the highest 32-bit cycle counter found in
 * each sector's empty ATE in @p cycles. This can be used to estimate
 * write-cycle consumption during testing.
 *
 * @param fs Pointer to the file system.
 * @param cycles Pointer to store the maximum 32-bit cycle count across sectors.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p fs or @p cycles is NULL.
 * @retval -EACCES if the file system is not mounted.
 */
int zms_get_num_cycles(struct zms_fs *fs, uint32_t *cycles);

/**
 * @brief Return the recycle count for a specific sector.
 *
 * @param fs Pointer to the file system.
 * @param sector Sector index (0-based, must be less than @c fs->sector_count).
 * @param cycles Pointer to store the 32-bit cycle count.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p fs or @p cycles is NULL, or @p sector is out of range.
 * @retval -EACCES if the file system is not mounted.
 * @retval -ENOENT if the sector has no valid empty ATE.
 */
int zms_get_sector_num_cycles(struct zms_fs *fs, uint32_t sector, uint32_t *cycles);

/**
 * @brief Initialise a ZMS iterator with filtering configuration.
 *
 * Captures the current write position of the file system as the iteration
 * boundary. Entries written or deleted after this call will not appear in
 * subsequent calls to @ref zms_iter_next.
 *
 * When @p config->use_mask is true, an entry is returned only if
 * ``(id & config->mask_id) == id``.
 * When @p config->use_range is true, an entry is returned only if its ID lies
 * in the inclusive range ``[config->min_id, config->max_id]``.
 * When @p config->use_predicate is true, an entry is returned only if
 * ``config->predicate_func(id)`` returns true.
 *
 * If either filter is disabled, the iterator falls back to the default value:
 * @ref ZMS_ITER_MASK_ALL for the mask and the full
 * ``[ZMS_ITER_ID_MIN, ZMS_ITER_ID_MAX]`` range. Predicate filtering is disabled
 * by default.
 *
 * @note The caller must not write to or delete from @p fs between
 *       zms_iter_init_with_config() and the last zms_iter_next() call.
 *
 * @param fs      Mounted file system instance.
 * @param iter    Iterator state to initialise.
 * @param config  Iterator configuration supplied by the caller.
 *
 * @retval 0       Success.
 * @retval -EINVAL @p fs, @p iter, or @p config is NULL, @p fs is not mounted,
 *                 @p config->predicate_func is NULL while @p use_predicate is
 *                 enabled, or the configured range is invalid.
 */
int zms_iter_init_with_config(const struct zms_fs *fs, struct zms_iter *iter,
			      const struct zms_iter_config *config);

/**
 * @brief Initialise a ZMS iterator.
 *
 * Captures the current write position of the file system as the iteration
 * boundary. Entries written or deleted after this call will not appear in
 * subsequent calls to @ref zms_iter_next.
 *
 * @note The caller must not write to or delete from @p fs between
 *       zms_iter_init() and the last zms_iter_next() call.
 *
 * @param fs   Mounted file system instance.
 * @param iter Iterator state to initialise.
 *
 * @retval 0       Success.
 * @retval -EINVAL @p fs or @p iter is NULL, or @p fs is not mounted.
 */
int zms_iter_init(const struct zms_fs *fs, struct zms_iter *iter);

/**
 * @brief Advance the iterator to the next live entry.
 *
 * Walks the ATE ring from newest to oldest. For each ID, only the most
 * recently written, non-deleted (len > 0) entry is yielded; older history
 * revisions and delete-markers are skipped transparently.
 *
 * @param fs    Mounted file system instance.
 * @param iter  Iterator state (must be initialised with @ref zms_iter_init).
 * @param id    On success (return value 1): populated with the entry ID.
 * @param len   On success (return value 1): populated with the stored data
 *              length in bytes.
 *
 * @retval 1   Entry found; @p id and @p len are valid.
 * @retval 0   No more entries; the walk is complete.
 * @retval -EINVAL @p fs, @p iter, @p id, or @p len is NULL, or @p fs is
 *              not mounted.
 * @retval -EIO   Flash read error.
 * @retval -ENXIO Device error.
 */
int zms_iter_next(struct zms_fs *fs, struct zms_iter *iter,
	zms_id_t *id, size_t *len);

/**
 * @brief Advance the iterator to the next matching ATE.
 *
 * Walks the ATE ring from newest to oldest and returns all matching ATEs,
 * including delete markers (len == 0) and older history revisions.
 *
 * This function does not perform ID uniqueness filtering. IDs can therefore
 * appear multiple times during traversal.
 *
 * @param fs    Mounted file system instance.
 * @param iter  Iterator state (must be initialised with @ref zms_iter_init or
 *              @ref zms_iter_init_with_config).
 * @param id    On success (return value 1): populated with the entry ID.
 * @param len   On success (return value 1): populated with the stored data
 *              length in bytes (0 means delete marker).
 *
 * @retval 1   Entry found; @p id and @p len are valid.
 * @retval 0   No more entries; the walk is complete.
 * @retval -EINVAL @p fs, @p iter, @p id, or @p len is NULL, or @p fs is
 *              not mounted.
 * @retval -EIO   Flash read error.
 * @retval -ENXIO Device error.
 */
int zms_iter_next_all(struct zms_fs *fs, struct zms_iter *iter,
	zms_id_t *id, size_t *len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KVSS_ZMS_H_ */
