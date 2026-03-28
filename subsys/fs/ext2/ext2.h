/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EXT2_H__
#define __EXT2_H__

#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_MAGIC_NUMBER 0xEF53
#define EXT2_MAX_FILE_NAME 255
#define EXT2_ROOT_INODE 2
#define EXT2_RESERVED_INODES 10
#define EXT2_GOOD_OLD_INODE_SIZE 128

#define EXT2_FEATURE_INCOMPAT_COMPRESSION 0x0001 /* Disk/File compression is used */
#define EXT2_FEATURE_INCOMPAT_FILETYPE    0x0002 /* Directory entries record the file type */
#define EXT3_FEATURE_INCOMPAT_RECOVER     0x0004 /* Filesystem needs recovery */
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV 0x0008 /* Filesystem has a separate journal device */
#define EXT2_FEATURE_INCOMPAT_META_BG     0x0010 /* Meta block groups */

#define EXT2_FEATURE_INCOMPAT_SUPPORTED (EXT2_FEATURE_INCOMPAT_FILETYPE)

#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001 /* Sparse Superblock */
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002 /* Large file support, 64-bit file size */
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR    0x0004 /* Binary tree sorted directory files */

#define EXT2_FEATURE_RO_COMPAT_SUPPORTED (0)

#define EXT2_INODE_BLOCKS 15 /* number of blocks referenced by inode i_block field */
#define EXT2_INODE_BLOCK_DIRECT 12
#define EXT2_INODE_BLOCK_1LVL 12
#define EXT2_INODE_BLOCK_2LVL 13
#define EXT2_INODE_BLOCK_3LVL 14

/* Inode mode flags */
#define EXT2_S_IFMT   0xF000 /* format mask */
#define EXT2_S_IFSOCK 0xC000 /* socket */
#define EXT2_S_IFLNK  0xA000 /* symbolic link */
#define EXT2_S_IFREG  0x8000 /* regular file */
#define EXT2_S_IFBLK  0x6000 /* block device */
#define EXT2_S_IFDIR  0x4000 /* directory */
#define EXT2_S_IFCHR  0x2000 /* character device */
#define EXT2_S_IFIFO  0x1000 /* fifo */

#define EXT2_S_IRUSR  0x100 /* owner may read */
#define EXT2_S_IWUSR  0x080 /* owner may write */
#define EXT2_S_IXUSR  0x040 /* owner may execute */
#define EXT2_S_IRGRP  0x020 /* group members may read */
#define EXT2_S_IWGRP  0x010 /* group members may write */
#define EXT2_S_IXGRP  0x008 /* group members may execute */
#define EXT2_S_IROTH  0x004 /* others may read */
#define EXT2_S_IWOTH  0x002 /* others may write */
#define EXT2_S_IXOTH  0x001 /* others may execute */

/* Default file mode: rw-r--r-- */
#define EXT2_DEF_FILE_MODE             \
	(EXT2_S_IFREG |                \
	 EXT2_S_IRUSR | EXT2_S_IWUSR | \
	 EXT2_S_IRGRP |                \
	 EXT2_S_IROTH)

/* Default dir mode: rwxr-xr-x */
#define EXT2_DEF_DIR_MODE                             \
	(EXT2_S_IFDIR |                               \
	 EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IXUSR | \
	 EXT2_S_IRGRP | EXT2_S_IXGRP |                \
	 EXT2_S_IROTH | EXT2_S_IXOTH)

#define IS_REG_FILE(mode) (((mode) & EXT2_S_IFMT) == EXT2_S_IFREG)
#define IS_DIR(mode) (((mode) & EXT2_S_IFMT) == EXT2_S_IFDIR)

/* Directory file type flags */
#define EXT2_FT_UNKNOWN  0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR      2
#define EXT2_FT_CHRDEV   3
#define EXT2_FT_BLKDEV   4
#define EXT2_FT_FIFO     5
#define EXT2_FT_SOCK     6
#define EXT2_FT_SYMLINK  7
#define EXT2_FT_MAX      8

/* Superblock status flags.
 * When file system is mounted the status is set to EXT2_ERROR_FS.
 * When file system is cleanly unmounted then flag is reset to EXT2_VALID_FS.
 */
#define EXT2_VALID_FS 0x0001 /* Unmounted cleanly */
#define EXT2_ERROR_FS 0x0002 /* Errors detected */

/* Revision flags. */
#define EXT2_GOOD_OLD_REV 0x0 /* Revision 0 */
#define EXT2_DYNAMIC_REV 0x1  /* Revision 1 */

/* Strategy when error detected. */
#define EXT2_ERRORS_CONTINUE 1 /* Continue as if nothing happened. */
#define EXT2_ERRORS_RO       2 /* Mount read only. */
#define EXT2_ERRORS_PANIC    3 /* Cause kernel panic. */

#endif /* __EXT2_H__ */
