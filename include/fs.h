/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FS_H_
#define _FS_H_

#include <sys/types.h>
#include <fs/fs_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Create a fs_file_t type similar to FILE for familiarity */
typedef struct _fs_file_object fs_file_t;

/* Create a fs_dir_t type similar to DIR for familiarity */
typedef struct _fs_dir_object fs_dir_t;

enum fs_dir_entry_type {
	FS_DIR_ENTRY_FILE,
	FS_DIR_ENTRY_DIR
};

/**
 * @brief File System
 * @defgroup file_system File System
 * @{
 * @}
 */

/**
 * @brief File System Data Structures
 * @defgroup data_structures File System Data Structures
 * @ingroup file_system
 * @{
 */

/** @var fs_file_t
 * @brief File object representing an open file
 */

/** @var fs_dir_t
 * @brief Directory object representing an open directory
 */

/**
 * @brief Structure to receive file or directory information
 *
 * Used in functions that reads the directory entries to get
 * file or directory information.
 *
 * @param dir_entry_type Whether file or directory
 * - FS_DIR_ENTRY_FILE
 * - FS_DIR_ENTRY_DIR
 * @param name Name of directory or file
 * @param size Size of file. 0 if directory
 */
struct fs_dirent {
	enum fs_dir_entry_type type;
	char name[MAX_FILE_NAME + 1];
	size_t size;
};

/**
 * @brief Structure to receive volume statistics
 *
 * Used to retrieve information about total and available space
 * in the volume.
 *
 * @param f_bsize Optimal transfer block size
 * @param f_frsize Allocation unit size
 * @param f_blocks Size of FS in f_frsize units
 * @param f_bfree Number of free blocks
 */
struct fs_statvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	unsigned long f_blocks;
	unsigned long f_bfree;
};

/**
 * @}
 */

#ifndef FS_SEEK_SET
#define FS_SEEK_SET	0	/* Seek from beginning of file. */
#endif
#ifndef FS_SEEK_CUR
#define FS_SEEK_CUR	1	/* Seek from current position. */
#endif
#ifndef FS_SEEK_END
#define FS_SEEK_END	2	/* Seek from end of file.  */
#endif

/**
 * @brief File System APIs
 * @defgroup file_system_api File System APIs
 * @ingroup file_system
 * @{
 */

/**
 * @brief File open
 *
 * Opens an existing file or create a new one and associates
 * a stream with it.
 *
 * @param zfp Pointer to file object
 * @param file_name The name of file to open
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int fs_open(fs_file_t *zfp, const char *file_name);

/**
 * @brief File close
 *
 * Flushes the associated stream and closes
 * the file.
 *
 * @param zfp Pointer to the file object
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int fs_close(fs_file_t *zfp);

/**
 * @brief File unlink
 *
 * Deletes the specified file or directory
 *
 * @param path Path to the file or directory to delete
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int fs_unlink(const char *path);

/**
 * @brief File read
 *
 * Reads items of data of size bytes long.
 *
 * @param zfp Pointer to the file object
 * @param ptr Pointer to the data buffer
 * @param size Number of bytes to be read
 *
 * @return Number of bytes read. On success, it will be equal to number of
 * items requested to be read. Returns less than number of bytes
 * requested if there are not enough bytes available in file. Will return
 * -ERRNO code on error.
 */
ssize_t fs_read(fs_file_t *zfp, void *ptr, size_t size);

/**
 * @brief File write
 *
 * Writes items of data of size bytes long.
 *
 * @param zfp Pointer to the file object
 * @param ptr Pointer to the data buffer
 * @param size Number of bytes to be write
 *
 * @return Number of bytes written. On success, it will be equal to the number
 * of bytes requested to be written. Any other value, indicates an error. Will
 * return -ERRNO code on error.
 * In the case where -ERRNO is returned, the file pointer will not be
 * advanced because it couldn't start the operation.
 * In the case where it is able to write, but is not able to complete writing
 * all of the requested number of bytes, then it is because the disk got full.
 * In that case, it returns less number of bytes written than requested, but
 * not a negative -ERRNO value as in regular error case.
 */
ssize_t fs_write(fs_file_t *zfp, const void *ptr, size_t size);

/**
 * @brief File seek
 *
 * Moves the file position to a new location in the file. The offset is added
 * to file position based on the 'whence' parameter.
 *
 * @param zfp Pointer to the file object
 * @param offset Relative location to move the file pointer to
 * @param whence Relative location from where offset is to be calculated.
 * - FS_SEEK_SET = from beginning of file
 * - FS_SEEK_CUR = from current position,
 * - FS_SEEK_END = from end of file.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error.
 */
int fs_seek(fs_file_t *zfp, off_t offset, int whence);

/**
 * @brief Get current file position.
 *
 * Retrieves the current position in the file.
 *
 * @param zfp Pointer to the file object
 *
 * @retval position Current position in file
 * Current revision does not validate the file object.
 */
off_t fs_tell(fs_file_t *zfp);

/**
 * @brief Change the size of an open file
 *
 * Truncates the file to the new length if it is shorter than the current
 * size of the file. Expands the file if the new length is greater than the
 * current size of the file. The expanded region would be filled with zeroes.
 *
 * @note In the case of expansion, if the volume got full during the
 * expansion process, the function will expand to the maximum possible length
 * and returns success. Caller should check if the expanded size matches the
 * requested length.
 *
 * @param zfp Pointer to the file object
 * @param length New size of the file in bytes
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int fs_truncate(fs_file_t *zfp, off_t length);

/**
 * @brief Flushes any cached write of an open file
 *
 * This function can be used to flush the cache of an open file. This can
 * be called to ensure data gets written to the storage media immediately.
 * This may be done to avoid data loss if power is removed unexpectedly.
 * Note that closing a file will cause caches to be flushed correctly so it
 * need not be called if the file is being closed.
 *
 * @param zfp Pointer to the file object
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int fs_sync(fs_file_t *zfp);

/**
 * @brief Directory create
 *
 * Creates a new directory using specified path.
 *
 * @param path Path to the directory to create
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int fs_mkdir(const char *path);

/**
 * @brief Directory open
 *
 * Opens an existing directory specified by the path.
 *
 * @param zdp Pointer to the directory object
 * @param path Path to the directory to open
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int fs_opendir(fs_dir_t *zdp, const char *path);

/**
 * @brief Directory read entry
 *
 * Reads directory entries of a open directory
 *
 * @param zdp Pointer to the directory object
 * @param entry Pointer to zfs_dirent structure to read the entry into
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 * @return In end-of-dir condition, this will return 0 and set
 * entry->name[0] = 0
 */
int fs_readdir(fs_dir_t *zdp, struct fs_dirent *entry);

/**
 * @brief Directory close
 *
 * Closes an open directory.
 *
 * @param zdp Pointer to the directory object
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int fs_closedir(fs_dir_t *zdp);

/**
 * @brief File or directory status
 *
 * Checks the status of a file or directory specified by the path
 *
 * @param path Path to the file or directory
 * @param entry Pointer to zfs_dirent structure to fill if file or directory
 * exists.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int fs_stat(const char *path, struct fs_dirent *entry);

/**
 * @brief Retrieves statistics of the file system volume
 *
 * Returns the total and available space in the file system volume.
 *
 * @param stat Pointer to zfs_statvfs structure to receive the fs statistics
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int fs_statvfs(struct fs_statvfs *stat);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _FS_H_ */
