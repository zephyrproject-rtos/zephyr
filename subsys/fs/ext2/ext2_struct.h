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
};

struct ext2_disk_bgroup {
	uint32_t bg_block_bitmap;
	uint32_t bg_inode_bitmap;
	uint32_t bg_inode_table;
	uint16_t bg_free_blocks_count;
	uint16_t bg_free_inodes_count;
	uint16_t bg_used_dirs_count;
	uint16_t bg_pad;
	uint8_t bg_reserved[12];
};

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
};

struct ext2_disk_dentry {
	uint32_t de_inode;
	uint16_t de_rec_len;
	uint8_t de_name_len;
	uint8_t de_file_type;
	char de_name[];
};

/* Program structures ------------------------------------------------------- */

struct ext2_block {
	uint32_t num;
	uint8_t *data;
} __aligned(sizeof(void *));

#define EXT2_DATA_FLAGS_RO BIT(0)

/* Accessing superblock disk structure (it is at some offset in stored block) */
#define EXT2_DATA_SBLOCK(fs) \
	((struct ext2_disk_superblock *)((uint8_t *)(fs)->sblock->data + (fs)->sblock_offset))

struct ext2_data;

struct ext2_backend_ops {
	int64_t (*get_device_size)(struct ext2_data *fs);
	int64_t (*get_write_size)(struct ext2_data *fs);
	int (*read_block)(struct ext2_data *fs, void *buf, uint32_t num);
	int (*write_block)(struct ext2_data *fs, const void *buf, uint32_t num);
	int (*read_superblock)(struct ext2_data *fs, struct ext2_disk_superblock *sb);
	int (*sync)(struct ext2_data *fs);
};

struct ext2_data {
	struct ext2_block *sblock;       /* superblock */

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
