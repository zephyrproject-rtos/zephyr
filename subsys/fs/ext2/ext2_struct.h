/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EXT2_STRUCT_H__
#define __EXT2_STRUCT_H__

#include <zephyr/kernel.h>
#include "ext2.h"

/* Disk structures ---------------------------------------------------------- */

struct ext2_disk_superblock {
	uint32_t s_inodes_count;
	uint32_t s_blocks_count;
	uint32_t s_r_blocks_count;
	uint32_t s_free_blocks_count;
	uint32_t s_free_inodes_count;
	uint32_t s_first_data_block;
	uint32_t s_log_block_size;
	uint32_t s_log_frag_size;
	uint32_t s_blocks_per_group;
	uint32_t s_frags_per_group;
	uint32_t s_inodes_per_group;
	uint32_t s_mtime;
	uint32_t s_wtime;
	uint16_t s_mnt_count;
	uint16_t s_max_mnt_count;
	uint16_t s_magic;
	uint16_t s_state;
	uint16_t s_errors;
	uint16_t s_minor_rev_level;
	uint32_t s_lastcheck;
	uint32_t s_checkinterval;
	uint32_t s_creator_os;
	uint32_t s_rev_level;
	uint16_t s_def_resuid;
	uint16_t s_def_resgid;
	uint32_t s_first_ino;
	uint16_t s_inode_size;
	uint16_t s_block_group_nr;
	uint32_t s_feature_compat;
	uint32_t s_feature_incompat;
	uint32_t s_feature_ro_compat;
	uint8_t s_uuid[16];
	uint8_t s_volume_name[16];
	uint8_t s_last_mounted[64];
	uint32_t s_algo_bitmap;
	uint8_t s_prealloc_blocks;
	uint8_t s_prealloc_dir_blocks;
	uint8_t s_align[2];
	uint8_t s_journal_uuid[16];
	uint32_t s_journal_inum;
	uint32_t s_journal_dev;
	uint32_t s_last_orphan;
	uint8_t  s_padding[788];
} __packed;

struct ext2_disk_bgroup {
	uint32_t bg_block_bitmap;
	uint32_t bg_inode_bitmap;
	uint32_t bg_inode_table;
	uint16_t bg_free_blocks_count;
	uint16_t bg_free_inodes_count;
	uint16_t bg_used_dirs_count;
	uint16_t bg_pad;
	uint8_t bg_reserved[12];
} __packed;

struct ext2_disk_inode {
	uint16_t i_mode;
	uint16_t i_uid;
	uint32_t i_size;
	uint32_t i_atime;
	uint32_t i_ctime;
	uint32_t i_mtime;
	uint32_t i_dtime;
	uint16_t i_gid;
	uint16_t i_links_count;
	uint32_t i_blocks;
	uint32_t i_flags;
	uint32_t i_osd1;
	uint32_t i_block[15];
	uint32_t i_generation;
	uint32_t i_file_acl;
	uint32_t i_dir_acl;
	uint32_t i_faddr;
	uint8_t i_osd2[12];
} __packed;

struct ext2_disk_direntry {
	uint32_t de_inode;
	uint16_t de_rec_len;
	uint8_t de_name_len;
	uint8_t de_file_type;
	char de_name[];
} __packed;

/* Program structures ------------------------------------------------------- */

struct ext2_superblock {
	uint32_t s_inodes_count;
	uint32_t s_blocks_count;
	uint32_t s_free_blocks_count;
	uint32_t s_free_inodes_count;
	uint32_t s_first_data_block;
	uint32_t s_log_block_size;
	uint32_t s_log_frag_size;
	uint32_t s_blocks_per_group;
	uint32_t s_frags_per_group;
	uint32_t s_inodes_per_group;
	uint16_t s_mnt_count;
	uint16_t s_max_mnt_count;
	uint16_t s_magic;
	uint16_t s_state;
	uint16_t s_errors;
	uint32_t s_creator_os;
	uint32_t s_rev_level;
	uint32_t s_first_ino;
	uint16_t s_inode_size;
	uint16_t s_block_group_nr;
	uint32_t s_feature_compat;
	uint32_t s_feature_incompat;
	uint32_t s_feature_ro_compat;
};

#define EXT2_BLOCK_NUM_SIZE (sizeof(uint32_t))
#define EXT2_DISK_DIRENTRY_BY_OFFSET(addr, offset) \
	((struct ext2_disk_direntry *)(((uint8_t *)(addr)) + (offset)))

#define EXT2_BLOCK_ASSIGNED BIT(0)

struct ext2_block {
	uint32_t num;
	uint8_t flags;
	uint8_t *data;
} __aligned(sizeof(void *));

#define BGROUP_INODE_TABLE(bg) ((struct ext2_disk_inode *)(bg)->inode_table->data)
#define BGROUP_INODE_BITMAP(bg) ((uint8_t *)(bg)->inode_bitmap->data)
#define BGROUP_BLOCK_BITMAP(bg) ((uint8_t *)(bg)->block_bitmap->data)

struct ext2_bgroup {
	struct ext2_data *fs;       /* pointer to file system data */

	struct ext2_block *inode_table;  /* fetched block of inode table */
	struct ext2_block *inode_bitmap; /* inode bitmap */
	struct ext2_block *block_bitmap; /* block bitmap */

	int32_t num;                /* number of described block group */
	uint32_t inode_table_block; /* number of fetched block (relative) */

	uint32_t bg_block_bitmap;
	uint32_t bg_inode_bitmap;
	uint32_t bg_inode_table;
	uint16_t bg_free_blocks_count;
	uint16_t bg_free_inodes_count;
	uint16_t bg_used_dirs_count;
};

/* Flags for inode */
#define INODE_FETCHED_BLOCK BIT(0)
#define INODE_REMOVE BIT(1)

struct ext2_inode {
	struct ext2_data *i_fs;      /* pointer to file system data */
	uint8_t  i_ref;              /* reference count */

	uint8_t  flags;
	uint32_t i_id;             /* inode number */
	uint16_t i_mode;           /* mode */
	uint16_t i_links_count;    /* link count */
	uint32_t i_size;           /* size */
	uint32_t i_blocks;         /* number of reserved blocks (of size 512B) */
	uint32_t i_block[15];      /* numbers of blocks */

	int block_lvl;             /* level of current block */
	uint32_t block_num;        /* relative number of fetched block */
	uint32_t offsets[4];       /* offsets describing path to fetched block */
	struct ext2_block *blocks[4];   /* fetched blocks for each level */
};

static inline struct ext2_block *inode_current_block(struct ext2_inode *inode)
{
	return inode->blocks[inode->block_lvl];
}

static inline uint8_t *inode_current_block_mem(struct ext2_inode *inode)
{
	return (uint8_t *)inode_current_block(inode)->data;
}

struct ext2_direntry {
	uint32_t de_inode;
	uint16_t de_rec_len;
	uint8_t de_name_len;
	uint8_t de_file_type;
	char de_name[];
};

/* Max size of directory entry that could be allocated from heap. */
#define MAX_DIRENTRY_SIZE (sizeof(struct ext2_direntry) + UINT8_MAX)

/* Structure common for files and directories representation */
struct ext2_file {
	struct ext2_inode *f_inode;
	uint32_t f_off;
	uint8_t f_flags;
};

#define EXT2_DATA_FLAGS_RO  BIT(0)
#define EXT2_DATA_FLAGS_ERR BIT(1)

struct ext2_data;

struct ext2_backend_ops {
	int64_t (*get_device_size)(struct ext2_data *fs);
	int64_t (*get_write_size)(struct ext2_data *fs);
	int (*read_block)(struct ext2_data *fs, void *buf, uint32_t num);
	int (*write_block)(struct ext2_data *fs, const void *buf, uint32_t num);
	int (*read_superblock)(struct ext2_data *fs, struct ext2_disk_superblock *sb);
	int (*sync)(struct ext2_data *fs);
};

#define MAX_INODES (CONFIG_MAX_FILES + 2)

struct ext2_data {
	struct ext2_superblock sblock; /* superblock */
	struct ext2_bgroup bgroup;     /* block group */

	int32_t open_inodes;
	int32_t open_files;
	struct ext2_inode *inode_pool[MAX_INODES];

	uint32_t sblock_offset;
	uint32_t block_size; /* fs block size */
	uint32_t write_size; /* dev minimal write size */
	uint64_t device_size;
	struct k_thread sync_thr;

	void *backend; /* pointer to implementation specific resource */
	const struct ext2_backend_ops *backend_ops;
	uint8_t flags;
};

#endif /* __EXT2_STRUCT_H__ */
