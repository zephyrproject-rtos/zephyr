/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/random/rand32.h>
#include <zephyr/fs/ext2.h>

#include "ext2.h"
#include "ext2_impl.h"
#include "ext2_struct.h"

LOG_MODULE_DECLARE(ext2);

FS_EXT2_DECLARE_DEFAULT_CONFIG(ext2_default);

static void validate_config(struct ext2_cfg *cfg)
{
	if (cfg->block_size == 0) {
		cfg->block_size = ext2_default.block_size;
	}

	if (cfg->bytes_per_inode == 0) {
		cfg->bytes_per_inode = ext2_default.bytes_per_inode;
	}

	if (cfg->volume_name[0] == '\0') {
		strcpy(cfg->volume_name, "ext2");
	}

	if (!cfg->set_uuid) {
		/* Generate random UUID */
		sys_rand_get(cfg->uuid, 16);

		/* Set version of UUID (ver. 4 variant 1) */
		cfg->uuid[6] = (cfg->uuid[6] & 0x0f) | 0x40;
		cfg->uuid[8] = (cfg->uuid[8] & 0x3f) | 0x80;
	}
}

int ext2_format(struct ext2_data *fs, struct ext2_cfg *cfg)
{
	int ret = 0;

	validate_config(cfg);
	LOG_INF("[Config] blk_sz:%d fs_sz:%d ino_bytes:%d uuid:'%s' vol:'%s'",
			cfg->block_size, cfg->fs_size, cfg->bytes_per_inode, cfg->uuid,
			cfg->volume_name);

	uint32_t fs_memory = cfg->fs_size ? MIN(cfg->fs_size, fs->device_size) : fs->device_size;

	/* Calculate value that will be stored in superblock field 's_log_block_size'. That value
	 * tells how much we have to shift 1024 to obtain block size.
	 * To obtain it we calculate: log(block_size) - 11
	 */
	uint8_t block_log_size = find_msb_set(cfg->block_size) - 11;

	LOG_INF("[Memory] available:%lld requested:%d", fs->device_size, fs_memory);

	if (fs_memory > fs->device_size) {
		LOG_ERR("No enough space on storage device");
		return -ENOSPC;
	}

	uint32_t blocks_count = fs_memory / cfg->block_size;
	uint32_t blocks_per_group = cfg->block_size * 8;

	/* Require at least 24 blocks to have at least 1 block per inode */
	if (blocks_count < 24) {
		LOG_ERR("Storage device too small to fit ext2 file system");
		return -ENOSPC;
	}

	/* We want to have only 1 block group (and one extra block) */
	if (blocks_count > blocks_per_group + 1) {
		LOG_ERR("File systems with more than 1 block group are not supported.");
		return -ENOTSUP;
	}


	/* Superblock, group descriptor table, 2 x bitmap. */
	uint32_t reserved_blocks = 4;

	uint32_t mem_for_inodes = fs_memory - reserved_blocks * cfg->block_size;
	uint32_t mem_per_inode = cfg->bytes_per_inode + sizeof(struct ext2_disk_inode);

	uint32_t inodes_per_block = cfg->block_size / sizeof(struct ext2_disk_inode);
	uint32_t inodes_count = mem_for_inodes / mem_per_inode;

	if (inodes_count % inodes_per_block) {
		/* Increase inodes_count to use entire blocks that are reserved for inode table. */
		inodes_count += inodes_per_block - (inodes_count % inodes_per_block);
	}

	uint32_t itable_blocks = inodes_count / inodes_per_block;
	uint32_t reserved_inodes = 10;

	/* Used blocks:
	 * Reserved blocks:
	 *   0 - boot sector
	 *   1 - superblock
	 *   2 - block group descriptor table
	 *   3 - block bitmap
	 *   4 - inode bitmap
	 *
	 * Inode table blocks:
	 *   5-x - inode table (x = 5 + inode_count / inodes per block )
	 *
	 * Other used blocks:
	 *   x+1 - root dir
	 */
	uint32_t used_blocks = reserved_blocks + itable_blocks + 1;
	uint32_t free_blocks = blocks_count - used_blocks - 1; /* boot sector block also counts */

	LOG_INF("[Blocks] total:%d per_grp:%d reserved:%d",
			blocks_count, blocks_per_group, used_blocks);
	LOG_INF("[Inodes] total:%d reserved:%d itable_blocks:%d",
			inodes_count, reserved_inodes, itable_blocks);

	uint32_t sb_offset;
	uint32_t sb_block_num, bg_block_num, bbitmap_block_num, ibitmap_block_num, in1_block_num,
		 root_dir_blk_num;

	if (cfg->block_size == 1024) {
		sb_offset = 0;
		sb_block_num = 1;
		bg_block_num = 2;
		bbitmap_block_num = 3;
		ibitmap_block_num = 4;
		in1_block_num = 5;
		root_dir_blk_num = used_blocks; /* last used block */
	} else {
		sb_offset = 1024;
		sb_block_num = 0;
		bg_block_num = 1;
		bbitmap_block_num = 2;
		ibitmap_block_num = 3;
		in1_block_num = 4;
		root_dir_blk_num = used_blocks; /* last used block */
	}

	struct ext2_block *sb_block = ext2_get_block(fs, sb_block_num);
	struct ext2_block *bg_block = ext2_get_block(fs, bg_block_num);
	struct ext2_block *bbitmap_block = ext2_get_block(fs, bbitmap_block_num);
	struct ext2_block *ibitmap_block = ext2_get_block(fs, ibitmap_block_num);
	struct ext2_block *in1_block = ext2_get_block(fs, in1_block_num);
	struct ext2_block *root_dir_blk_block = ext2_get_block(fs, root_dir_blk_num);

	if (root_dir_blk_block == NULL || in1_block == NULL || ibitmap_block == NULL ||
			bbitmap_block == NULL || bg_block == NULL || sb_block == NULL) {
		ret =  -ENOMEM;
		goto out;
	}

	struct ext2_disk_superblock *sb =
		(struct ext2_disk_superblock *)((uint8_t *)sb_block->data + sb_offset);

	memset(sb, 0, 1024);
	sb->s_inodes_count = inodes_count;
	sb->s_blocks_count = blocks_count;
	sb->s_r_blocks_count = 0;
	sb->s_free_blocks_count = free_blocks;
	sb->s_free_inodes_count = inodes_count - reserved_inodes;
	sb->s_first_data_block = sb_block_num;
	sb->s_log_block_size = block_log_size;
	sb->s_log_frag_size = block_log_size;
	sb->s_blocks_per_group = cfg->block_size * 8;
	sb->s_frags_per_group = cfg->block_size * 8;
	sb->s_inodes_per_group = inodes_count;
	sb->s_mtime = 0;
	sb->s_wtime = 0;
	sb->s_mnt_count = 0;
	sb->s_max_mnt_count = -1;
	sb->s_magic = 0xEF53;
	sb->s_state = EXT2_VALID_FS;
	sb->s_errors = EXT2_ERRORS_RO;
	sb->s_minor_rev_level = 0;
	sb->s_lastcheck = 0;
	sb->s_checkinterval = 0;
	sb->s_creator_os = 5; /* Unknown OS */
	sb->s_rev_level = EXT2_DYNAMIC_REV;
	sb->s_def_resuid = 0;
	sb->s_def_resgid = 0;
	sb->s_first_ino = 11;
	sb->s_inode_size = sizeof(struct ext2_disk_inode);
	sb->s_block_group_nr = 0;
	sb->s_feature_compat = 0;
	sb->s_feature_incompat = EXT2_FEATURE_INCOMPAT_FILETYPE;
	sb->s_feature_ro_compat = 0;

	memcpy(sb->s_uuid, cfg->uuid, 16);
	memcpy(sb->s_volume_name, cfg->uuid, 16);

	sb->s_algo_bitmap = 0;
	sb->s_prealloc_blocks = 0;
	sb->s_prealloc_dir_blocks = 0;
	sb->s_journal_inum = 0;
	sb->s_journal_dev = 0;
	sb->s_last_orphan = 0;

	/* Block descriptor table */

	struct ext2_disk_bgroup *bg = (struct ext2_disk_bgroup *)bg_block->data;

	memset(bg, 0, cfg->block_size);
	bg->bg_block_bitmap = bbitmap_block_num;
	bg->bg_inode_bitmap = ibitmap_block_num;
	bg->bg_inode_table = in1_block_num;
	bg->bg_free_blocks_count = free_blocks;
	bg->bg_free_inodes_count = inodes_count - reserved_inodes;
	bg->bg_used_dirs_count = 1;

	/* Inode table */
	struct ext2_disk_inode *in1 = (struct ext2_disk_inode *)in1_block->data;

	memset(in1, 0, cfg->block_size);
	in1[1].i_mode = 0x4000 | 0755;
	in1[1].i_uid = 0;
	in1[1].i_size = cfg->block_size;
	in1[1].i_atime = 0;
	in1[1].i_ctime = 0;
	in1[1].i_mtime = 0;
	in1[1].i_dtime = 0;
	in1[1].i_gid = 0;
	in1[1].i_links_count = 2;
	in1[1].i_blocks = cfg->block_size / 512;
	in1[1].i_flags = 0;
	in1[1].i_osd1 = 0;
	in1[1].i_generation = 0;
	in1[1].i_file_acl = 0;
	in1[1].i_dir_acl = 0;
	in1[1].i_faddr = 0;
	in1[1].i_block[0] = root_dir_blk_num;

	/* Block bitmap */

	uint8_t *bbitmap = bbitmap_block->data;

	memset(bbitmap, 0, cfg->block_size);

	int i = 0, blocks = used_blocks;
	int bits;
	uint16_t to_set;

	while (blocks > 0) {
		bits = MIN(8, blocks);
		to_set = (1 << bits) - 1;
		bbitmap[i] = (uint8_t)to_set;
		blocks -= 8;
		i++;
	}

	/* Inode bitmap */

	uint8_t *ibitmap = ibitmap_block->data;

	memset(ibitmap, 0, cfg->block_size);
	ibitmap[0] = 0xff;
	ibitmap[1] = 0x03;

	memset(root_dir_blk_block->data, 0, cfg->block_size);

	struct ext2_disk_dentry *de = (struct ext2_disk_dentry *)root_dir_blk_block->data;

	de->de_inode = 2;
	de->de_rec_len = sizeof(struct ext2_disk_dentry) + 4;
	de->de_name_len = 1;
	de->de_file_type = EXT2_FT_DIR;
	memset(de->de_name, '.', 1);

	de = (struct ext2_disk_dentry *)(((uint8_t *)de) + de->de_rec_len);
	de->de_inode = 2;
	de->de_rec_len = cfg->block_size - 12;
	de->de_name_len = 2;
	de->de_file_type = EXT2_FT_DIR;
	memset(de->de_name, '.', 2);

	sb_block->flags |= EXT2_BLOCK_DIRTY;
	bg_block->flags |= EXT2_BLOCK_DIRTY;
	in1_block->flags |= EXT2_BLOCK_DIRTY;
	bbitmap_block->flags |= EXT2_BLOCK_DIRTY;
	ibitmap_block->flags |= EXT2_BLOCK_DIRTY;
	root_dir_blk_block->flags |= EXT2_BLOCK_DIRTY;
out:
	ext2_drop_block(fs, sb_block);
	ext2_drop_block(fs, bg_block);
	ext2_drop_block(fs, in1_block);
	ext2_drop_block(fs, bbitmap_block);
	ext2_drop_block(fs, ibitmap_block);
	ext2_drop_block(fs, root_dir_blk_block);
	return ret;
}
