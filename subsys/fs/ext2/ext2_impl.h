/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EXT2_IMPL_H__
#define __EXT2_IMPL_H__

#include <zephyr/fs/fs.h>
#include <zephyr/fs/ext2.h>

#include "ext2_struct.h"

extern struct k_heap direntry_heap;

void error_behavior(struct ext2_data *fs, const char *msg);

/* Memory allocation for ext2 implementation */
void *ext2_heap_alloc(size_t size);
void  ext2_heap_free(void *ptr);

/* Initialization of disk storage. */
int ext2_init_disk_access_backend(struct ext2_data *fs, const void *storage_dev, int flags);

/**
 * @brief Get block from the disk.
 */
struct ext2_block *ext2_get_block(struct ext2_data *fs, uint32_t block);

struct ext2_block *ext2_get_empty_block(struct ext2_data *fs);

/**
 * @brief Free the block structure.
 */
void ext2_drop_block(struct ext2_block *b);

/**
 * @brief Write block to the disk.
 *
 * NOTICE: to ensure that all writes has ended the sync of disk must be triggered
 * (fs::sync function).
 */
int ext2_write_block(struct ext2_data *fs, struct ext2_block *b);

void ext2_init_blocks_slab(struct ext2_data *fs);

/**
 * @brief Write block to the disk.
 *
 * NOTICE: to ensure that all writes has ended the sync of disk must be triggered
 * (fs::sync function).
 */
int ext2_write_block(struct ext2_data *fs, struct ext2_block *b);

int ext2_assign_block_num(struct ext2_data *fs, struct ext2_block *b);

/* FS operations */

/**
 * @brief Initialize structure with data needed to access the storage device
 *
 * @param fs File system data structure to initialize
 * @param storage_dev Pointer to storage
 * @param flags Additional flags (e.g. RO flag)
 *
 * @retval 0 on success
 * @retval -EINVAL when superblock of ext2 was not detected
 * @retval -ENOTSUP when described file system is not supported
 * @retval <0 other error
 */
int ext2_init_storage(struct ext2_data **fsp, const void *storage_dev, int flags);

/**
 * @brief Verify superblock of file system
 *
 * Checks if file system is supported by the implementation.
 * @retval 0 when superblock is valid
 * @retval -EROFS when superblock is not valid but file system may be mounted read only
 * @retval -EINVAL when superblock is not valid and file system cannot be mounted at all
 * @retval -ENOTSUP when superblock has some field set to value that we don't support
 */
int ext2_verify_disk_superblock(struct ext2_disk_superblock *sb);

/**
 * @brief Initialize all data needed to perform operations on file system
 *
 * Fetches the superblock. Initializes structure fields.
 */
int ext2_init_fs(struct ext2_data *fs);

/**
 * @brief Clear the data used by file system implementation
 *
 */
int ext2_close_fs(struct ext2_data *fs);

/**
 * @brief Clear the data used to communicate with storage device
 *
 */
int ext2_close_struct(struct ext2_data *fs);

/**
 * @brief Create the ext2 file system
 *
 * This function uses functions stored in `ext2_data` structure to create new
 * file system on storage device.
 *
 * NOTICE: fs structure must be first initialized with `ext2_init_fs` function.
 *
 * After this function succeeds the `ext2_clean` function must be called.
 *
 * @param fs File system data (must be initialized before)
 *
 * @retval 0 on success
 * @retval -ENOSPC when storage device is too small for ext2 file system
 * @retval -ENOTSUP when storage device is too big (file systems with more than
 *         8192 blocks are not supported)
 */
int ext2_format(struct ext2_data *fs, struct ext2_cfg *cfg);

/* Lookup flags */
#define LOOKUP_ARG_OPEN BIT(0)
#define LOOKUP_ARG_CREATE BIT(1)
#define LOOKUP_ARG_STAT BIT(2)
#define LOOKUP_ARG_UNLINK BIT(3)

/* Structure for lookup arguments and results.
 *
 * Required fields (must be filled when lookup function is invoked):
 *  - path
 *  - flags
 *
 * Fields that hold the result:
 *  - inode
 *  - parent
 *  - offset
 *  - name_pos
 *  - name_len
 *
 * Some of these fields have a meaning only for a specific function.
 * (E.g. during stat only the fields parent and offset are used)
 *
 * Field is marked with these labels when lookup is used from other function:
 *  OP -- open
 *  CR -- create
 *  ST -- stat
 *  UN -- unlink
 */
struct ext2_lookup_args {
	const char *path;  /* path of inode */
	struct ext2_inode *inode;  /* (OP, CR, ST, UN) found inode */
	struct ext2_inode *parent; /* (CR, ST, UN) parent of found inode */
	uint32_t offset;   /* (CR, ST, UN) offset of entry in directory */
	uint32_t name_pos; /* (CR) position of name in input path */
	uint32_t name_len; /* (CR) length of name */
	uint8_t flags;     /* indicates from which function lookup is invoked */
};

/**
 * @brief Look for an inode.
 *
 * @param fs File system data
 * @param args All needed arguments for lookup
 *
 * @retval 0 on success
 * @retval -ENOENT inode or path component not found
 * @retval -ENOTDIR path component not a directory
 * @retval <0 other error
 */
int ext2_lookup_inode(struct ext2_data *fs, struct ext2_lookup_args *args);

/* Inode operations */

/**
 * @brief Read from inode at given offset
 *
 * @param inode Inode
 * @param buf Buffer to hold read data
 * @param offset Offset in inode
 * @param nbytes Number of bytes to read
 *
 * @retval >=0 number of bytes read on success
 * @retval <0 error code
 */
k_ssize_t ext2_inode_read(struct ext2_inode *inode, void *buf, uint32_t offset, size_t nbytes);

/**
 * @brief Write to inode at given offset
 *
 * @param inode Inode
 * @param buf Buffer with data to write
 * @param offset Offset in inode
 * @param nbytes Number of bytes to write
 *
 * @retval >=0 number of bytes read on success
 * @retval <0 error code
 */
k_ssize_t ext2_inode_write(struct ext2_inode *inode, const void *buf, uint32_t offset,
			      size_t nbytes);

/**
 * @brief Truncate the inode
 *
 * @param inode Inode
 * @param size New size for inode
 *
 * @retval 0 on success
 * @retval -ENOTSUP when requested size is too big
 * @retval <0 other error
 */
int ext2_inode_trunc(struct ext2_inode *inode, k_off_t size);

/**
 * @brief Sync currently fetched blocks
 *
 * @param inode Inode
 *
 */
int ext2_inode_sync(struct ext2_inode *inode);

/* Directory operations */

/**
 * @brief Get directory entry
 *
 * Reads directory entry that is at offset specified in `ext2_file` structure.
 *
 * @param dir Read directory
 * @param ent Directory entry to fill in
 *
 * @retval 0 on success
 * @retval <0 on error
 */
int ext2_get_direntry(struct ext2_file *dir, struct fs_dirent *ent);

/**
 * @brief Create a directory entry with given attributes
 *
 * Automatically calculates and sets de_rec_len field.
 *
 * NOTE: if you need to adjust the size (e.g. when this entry is the last one in the block)
 *       then just update the size after this function returns.
 *
 * @param name Name of direntry
 * @param namelen Length of name
 * @param ino Inode associated with that entry
 * @param filetype File type of that entry
 *
 * @returns structure allocated on direntry_heap filled with given data
 */
struct ext2_direntry *ext2_create_direntry(const char *name, uint8_t namelen, uint32_t ino,
		uint8_t filetype);

/**
 * @brief Create a file
 *
 * @param parent Parent directory
 * @param inode Pointer to inode structure that will be filled with new inode
 * @param args Lookup arguments that describe file to create
 *
 * @retval 0 on success
 * @retval -ENOSPC there is not enough memory on storage device to create a file
 * @retval <0 on error
 */
int ext2_create_file(struct ext2_inode *parent, struct ext2_inode *inode,
		     struct ext2_lookup_args *args);


/**
 * @brief Create a directory
 *
 * @param parent Parent directory
 * @param inode Pointer to inode structure that will be filled with new inode
 * @param args Lookup arguments that describe directory to create
 *
 * @retval 0 on success
 * @retval -ENOSPC there is not enough memory on storage device to create a file
 * @retval <0 on error
 */
int ext2_create_dir(struct ext2_inode *parent, struct ext2_inode *inode,
		    struct ext2_lookup_args *args);

/**
 * @brief Unlink the directory entry at given offset in parent directory
 *
 * @param parent Parent directory
 * @param inode File to unlink
 * @param offset Offset of unlinked file in the parent directory
 *
 * @retval 0 on success
 * @retval -ENOTEMPTY when directory to unlink is not empty
 * @retval <0 other error
 */
int ext2_inode_unlink(struct ext2_inode *parent, struct ext2_inode *inode,
		      uint32_t offset);

/**
 * @brief Move a file
 *
 * Invoked when rename destination entry doesn't exist.
 *
 * @param args_from Describe source file
 * @param args_to Describe destination
 *
 * @retval 0 on success
 * @retval <0 on error
 */
int ext2_move_file(struct ext2_lookup_args *args_from, struct ext2_lookup_args *args_to);

/**
 * @brief Replace the file with another
 *
 * Invoked when rename destination entry does exist
 *
 * @param args_from Describe source file
 * @param args_to Describe destination file
 *
 * @retval 0 on success
 * @retval <0 on error
 */
int ext2_replace_file(struct ext2_lookup_args *args_from, struct ext2_lookup_args *args_to);

/* Inode pool operations */

/**
 * @brief Get the inode
 *
 * Retrieves inode structure and stores it in the inode pool. The inode is
 * filled with data of requested inode.
 *
 * @param fs File system data
 * @param ino Inode number
 * @param ret Pointer to place where to store new inode pointer
 *
 * @retval 0 on success
 * @retval -ENOMEM when there is no memory to hold the requested inode
 * @retval <0 on error
 */
int ext2_inode_get(struct ext2_data *fs, uint32_t ino, struct ext2_inode **ret);

/**
 * @brief Remove reference to the inode structure
 *
 * When removed reference is the last reference to that inode then it is freed.
 *
 * @param inode Dropped inode
 *
 * @retval 0 on success
 * @retval -EINVAL the dropped inode is not stored in the inode pool
 * @retval <0 on error
 */
int ext2_inode_drop(struct ext2_inode *inode);

/* Drop blocks fetched in inode structure. */
void ext2_inode_drop_blocks(struct ext2_inode *inode);

/**
 * @brief Remove all blocks starting with some block
 *
 * @param first First block to remove
 *
 * @retval >=0 number of removed blocks
 * @retval <0 error code
 */
int64_t ext2_inode_remove_blocks(struct ext2_inode *inode, uint32_t first);

#endif /* __EXT2_IMPL_H__ */
