/*  NVS: non volatile storage in flash
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __NVS_H_
#define __NVS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
/**
 * @brief Non-volatile Storage
 * @defgroup nvs Non-volatile Storage
 * @{
 * @}
 */

/**
 * @brief Non-volatile Storage Data Structures
 * @defgroup nvs_data_structures Non-volatile Storage Data Structures
 * @ingroup nvs
 * @{
 */

/**
 * @brief Non-volatile Storage File system structure
 *
 * @param magic File system magic, repeated at start of each sector
 * @param write_location Next write location for entry header
 * @param offset File system offset in flash
 * @param sector_id Counter for sector
 * @param sector_size File system is divided into sectors each sector should be
 * multiple of pagesize and also a power of 2
 * @param free_space Indicator for available space in the filesystem
 * @param max_len Maximum size of storage items
 * @param sector_count Amount of sectors in the file systems
 * @param entry_sector Oldest sector in used
 * @param write_block_size Alignment size in bytes_to_copy
 * @param nvs_lock Mutex
 * @param flash_device Flash Device
 */
struct nvs_fs {
	u32_t magic; /* filesystem magic, repeated at start of each sector */
	off_t write_location; /* next write location for entry header */
	off_t offset; /* filesystem offset in flash */
	u16_t sector_id; /* sector id, a counter for each created sector */
	u16_t sector_size; /* filesystem is divided into sectors,
			    * sector size should be multiple of pagesize
			    * and a power of 2
			    */
	u16_t free_space; /* Indicator for available space in the file system.
			   * Writes are only allowed if free_space is equal to
			   * max_len. Is set to zero when the file system is
			   * full. Deletes increase free_space.
			   */
	u16_t max_len; /* maximum size of stored item, set to sector_size/4 */
	u8_t sector_count; /* how many sectors in the filesystem */

	u8_t entry_sector; /* oldest sector in use */
	u8_t write_block_size; /* write block size for alignment */

	struct k_mutex nvs_lock;
	struct device *flash_device;
};

/**
 * @}
 */

 /**
  * @brief Non-volatile Storage APIs
  * @defgroup nvs_high_level_api Non-volatile Storage APIs
  * @ingroup nvs
  * @{
  */

 /**
  * @brief nvs_init
  *
  * Initializes a NVS file system in flash.
  *
  * @param fs Pointer to file system
  * @param dev_name Pointer to flash device name
  * @param magic Magic number used in file system
  * @retval 0 Success
  * @retval -ERRNO errno code if error
  */
int nvs_init(struct nvs_fs *fs, const char *dev_name, u32_t magic);

/**
 * @brief nvs_clear
 *
 * Clears the NVS file system from flash.
 *
 * @param fs Pointer to file system
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int nvs_clear(struct nvs_fs *fs);

/**
 * @brief nvs_write
 *
 * Write an entry to the file system.
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be written
 * @param data Pointer to the data to be written
 * @param len Number of bytes to be written
 *
 * @return Number of bytes written. On success, it will be equal to the number
 * of bytes requested to be written. Any other value, indicates an error. Will
 * return -ERRNO code on error.
 */
ssize_t nvs_write(struct nvs_fs *fs, u16_t id, const void *data, size_t len);

/**
 * @brief nvs_delete
 *
 * Delete an entry from the file system
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be deleted
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int nvs_delete(struct nvs_fs *fs, u16_t id);

/**
 * @brief nvs_read
 *
 * Read an entry from the file system.
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be read
 * @param data Pointer to data buffer
 * @param len Number of bytes to be read
 *
 * @return Number of bytes read. On success, it will be equal to the number
 * of bytes requested to be read. Any other value, indicates an error. When
 * the number of bytes read is larger than the number of bytes requested to
 * read this indicates not all bytes were read, and more data is available.
 * Return -ERRNO code on error.
 */
ssize_t nvs_read(struct nvs_fs *fs, u16_t id, void *data, size_t len);

/**
 * @brief nvs_read_hist
 *
 * Read a history entry from the file system.
 *
 * @param fs Pointer to file system
 * @param id Id of the entry to be read
 * @param data Pointer to data buffer
 * @param len Number of bytes to be read
 * @param cnt History counter: 0: latest entry, 1:one before latest ...
 *
 * @return Number of bytes read. On success, it will be equal to the number
 * of bytes requested to be read. Any other value, indicates an error. When
 * the number of bytes read is larger than the number of bytes requested to
 * read this indicates not all bytes were read, and more data is available.
 * Return -ERRNO code on error.
 */
ssize_t nvs_read_hist(struct nvs_fs *fs, u16_t id, void *data, size_t len,
		  u16_t cnt);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __NVS_H_ */
