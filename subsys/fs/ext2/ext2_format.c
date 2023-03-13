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

LOG_MODULE_DECLARE(ext2, LOG_LEVEL_DBG);

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

static void set_bitmap_padding(uint8_t *bitmap, uint32_t nelems, struct ext2_cfg *cfg)
{
	uint32_t used_bytes = nelems / 8 + (nelems % 8 != 0);

	LOG_DBG("Set bitmap padding: %d bytes", nelems);
	memset(bitmap, 0x00, used_bytes);

	/* Set padding in block-bitmap block */
	if (nelems % 8) {
		bitmap[used_bytes - 1] = (0xff << (nelems % 8)) & 0xff;
		LOG_DBG("last byte: %02x", (0xff << (nelems % 8)) & 0xff);
	}
	memset(bitmap + used_bytes, 0xff, cfg->block_size - used_bytes);
}

static void set_bitmap_bits(uint8_t *bitmap, uint32_t to_set)
{
	int i = 0, bits;
	uint16_t set_value;

	while (to_set > 0) {
		bits = MIN(8, to_set);
		set_value = (1 << bits) - 1;
		bitmap[i] = (uint8_t)set_value;
		to_set -= bits;
		i++;
	}
}

static void default_directory_inode(struct ext2_disk_inode *in, uint32_t nblocks,
					struct ext2_cfg *cfg)
{
	LOG_DBG("Set directory inode: %p", in);
	in->i_mode = EXT2_S_IFDIR;
	in->i_uid = 0;
	in->i_size = nblocks * cfg->block_size;
	in->i_atime = 0;
	in->i_ctime = 0;
	in->i_mtime = 0;
	in->i_dtime = 0;
	in->i_gid = 0;
	in->i_blocks = nblocks * cfg->block_size / 512;
	in->i_flags = 0;
	in->i_osd1 = 0;
	in->i_generation = 0;
	in->i_file_acl = 0;
	in->i_dir_acl = 0;
	in->i_faddr = 0;
	memset(in->i_block, 0, EXT2_INODE_BLOCKS * sizeof(uint32_t));
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
	uint32_t inodes_per_block = cfg->block_size / sizeof(struct ext2_disk_inode);
	uint32_t mem_per_inode = cfg->bytes_per_inode + sizeof(struct ext2_disk_inode);

	/* 24 block should be enough to fit minimal file system. */
	if (blocks_count < 24) {
		LOG_ERR("Storage device too small to fit ext2 file system");
		return -ENOSPC;
	}

	uint32_t sb_offset;
	uint32_t first_data_block;
	uint32_t occupied_blocks;

	if (cfg->block_size == 1024) {
		/* Superblock is stored in 1st block */
		sb_offset = 0;
		first_data_block = 1;
		occupied_blocks = 2;
	} else {
		/* Superblock is stored in 0th block */
		sb_offset = 1024;
		first_data_block = 0;
		occupied_blocks = 1;
	}

	/* Reserve blocks for block group and bitmaps. */
	uint32_t bg_block_num = occupied_blocks++;
	uint32_t bbitmap_block_num = occupied_blocks++;
	uint32_t ibitmap_block_num = occupied_blocks++;

	/* We want to have only 1 block group (that starts with first data block) */
	if (blocks_count > blocks_per_group + first_data_block) {
		LOG_ERR("File systems with more than 1 block group are not supported.");
		return -ENOTSUP;
	}

	uint32_t mem_for_inodes = fs_memory - occupied_blocks * cfg->block_size;
	uint32_t inodes_count = mem_for_inodes / mem_per_inode;

	/* Align indes_count to use last block of inode table entirely. */
	if (inodes_count % inodes_per_block) {
		inodes_count += inodes_per_block - (inodes_count % inodes_per_block);
	}

	uint32_t itable_blocks = inodes_count / inodes_per_block;
	uint32_t used_inodes = EXT2_RESERVED_INODES;
	uint32_t lost_found_inode = 1 + used_inodes++; /* We count inodes from 1. */

	/* First unoccupied block will be the start of inode table. */
	uint32_t itable_block_num = occupied_blocks;

	occupied_blocks += itable_blocks;

	/* Two next block after inode table will be the blocks for '/' and 'lost+found' dirs */
	uint32_t root_dir_blk_num = occupied_blocks++;
	uint32_t lost_found_dir_blk_num = occupied_blocks++;

	LOG_INF("root: %d l+f: %d", root_dir_blk_num, lost_found_dir_blk_num);

	/* All blocks available for writes after creating file system. */
	uint32_t free_blocks = blocks_count - occupied_blocks;

	/* Blocks that will be described in bitmaps. */
	uint32_t used_blocks = occupied_blocks - first_data_block;

	LOG_INF("[Blocks] total:%d per_grp:%d occupied:%d used:%d",
			blocks_count, blocks_per_group, occupied_blocks, used_blocks);
	LOG_INF("[Inodes] total:%d used:%d itable_blocks:%d",
			inodes_count, used_inodes, itable_blocks);

	struct ext2_block *sb_block = ext2_get_block(fs, first_data_block);
	struct ext2_block *bg_block = ext2_get_block(fs, bg_block_num);
	struct ext2_block *bbitmap_block = ext2_get_block(fs, bbitmap_block_num);
	struct ext2_block *ibitmap_block = ext2_get_block(fs, ibitmap_block_num);
	struct ext2_block *itable_block1, *itable_block2, *root_dir_blk, *lost_found_dir_blk;

	itable_block1 = itable_block2 = root_dir_blk = lost_found_dir_blk =  NULL;

	if (ibitmap_block == NULL || bbitmap_block == NULL ||
	    bg_block == NULL || sb_block == NULL) {
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
	sb->s_free_inodes_count = inodes_count - used_inodes;
	sb->s_first_data_block = first_data_block;
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
	sb->s_algo_bitmap = 0;
	sb->s_prealloc_blocks = 0;
	sb->s_prealloc_dir_blocks = 0;
	sb->s_journal_inum = 0;
	sb->s_journal_dev = 0;
	sb->s_last_orphan = 0;

	memcpy(sb->s_uuid, cfg->uuid, 16);
	strcpy(sb->s_volume_name, cfg->volume_name);

	sb_block->flags |= EXT2_BLOCK_DIRTY;

	/* Block descriptor table */

	struct ext2_disk_bgroup *bg = (struct ext2_disk_bgroup *)bg_block->data;

	memset(bg, 0, cfg->block_size);
	bg->bg_block_bitmap = bbitmap_block_num;
	bg->bg_inode_bitmap = ibitmap_block_num;
	bg->bg_inode_table = itable_block_num;
	bg->bg_free_blocks_count = free_blocks;
	bg->bg_free_inodes_count = inodes_count - used_inodes;
	bg->bg_used_dirs_count = 2; /* '/' and 'lost+found' */

	bg_block->flags |= EXT2_BLOCK_DIRTY;

	/* Block bitmap */
	uint8_t *bbitmap = bbitmap_block->data;

	/* In bitmap we describe blocks starting from s_first_data_block. */
	set_bitmap_padding(bbitmap, blocks_count - sb->s_first_data_block, cfg);
	set_bitmap_bits(bbitmap, used_blocks);
	bbitmap_block->flags |= EXT2_BLOCK_DIRTY;

	/* Inode bitmap */
	uint8_t *ibitmap = ibitmap_block->data;

	set_bitmap_padding(ibitmap, inodes_count, cfg);
	set_bitmap_bits(ibitmap, used_inodes);
	ibitmap_block->flags |= EXT2_BLOCK_DIRTY;

	/* Inode table */
	/* Zero inode table */
	for (int i = 0; i < itable_blocks; i++) {
		struct ext2_block *blk = ext2_get_block(fs, itable_block_num + i);

		memset(blk->data, 0, cfg->block_size);
		blk->flags |= EXT2_BLOCK_DIRTY;
		ext2_drop_block(fs, blk);
	}

	struct ext2_disk_inode *in;
	int inode_offset;

	/* Set inode 2 ('/' directory) */
	itable_block1 = ext2_get_block(fs, itable_block_num);
	in = (struct ext2_disk_inode *)itable_block1->data;
	inode_offset = EXT2_ROOT_INODE - 1;
	default_directory_inode(&in[inode_offset], 1, cfg);

	in[inode_offset].i_mode = EXT2_DEF_DIR_MODE;
	in[inode_offset].i_links_count = 3; /* 2 from itself and 1 from child directory */
	in[inode_offset].i_block[0] = root_dir_blk_num;
	itable_block1->flags |= EXT2_BLOCK_DIRTY;

	/* Set inode for 'lost+found' directory */
	inode_offset = (lost_found_inode - 1) % inodes_per_block; /* We count inodes from 1 */

	LOG_DBG("Inode offset: %d", inode_offset);

	if (inodes_per_block < lost_found_inode) {
		/* We need to fetch new inode table block */
		uint32_t block_num = itable_block_num + lost_found_inode / inodes_per_block;

		itable_block2 = ext2_get_block(fs, block_num);
		in = (struct ext2_disk_inode *)itable_block2->data;
	}

	default_directory_inode(&in[inode_offset], 1, cfg);
	in[inode_offset].i_mode = EXT2_DEF_DIR_MODE;
	in[inode_offset].i_links_count = 2; /* 1 from itself and 1 from parent directory */
	in[inode_offset].i_block[0] = lost_found_dir_blk_num;
	if (itable_block2) {
		itable_block2->flags |= EXT2_BLOCK_DIRTY;
	}

	struct ext2_disk_dentry *de;
	uint32_t current_size;

	/* Contents of '/' directory */
	LOG_DBG("Root dir blk: %d", root_dir_blk_num);
	root_dir_blk = ext2_get_block(fs, root_dir_blk_num);
	if (root_dir_blk == NULL) {
		ret = ENOMEM;
		goto out;
	}
	memset(root_dir_blk->data, 0, cfg->block_size);

	current_size = 0;

	de = (struct ext2_disk_dentry *)root_dir_blk->data;
	ext2_fill_direntry(de, ".", 1, EXT2_ROOT_INODE, EXT2_FT_DIR);
	current_size += de->de_rec_len;

	de = EXT2_NEXT_DISK_DIRENTRY(de);
	ext2_fill_direntry(de, "..", 2, EXT2_ROOT_INODE, EXT2_FT_DIR);
	current_size += de->de_rec_len;

	de = EXT2_NEXT_DISK_DIRENTRY(de);
	ext2_fill_direntry(de, "lost+found", strlen("lost+found"), lost_found_inode, EXT2_FT_DIR);
	current_size += de->de_rec_len;

	/* This was the last entry so add padding until end of block */
	de->de_rec_len += cfg->block_size - current_size;

	root_dir_blk->flags |= EXT2_BLOCK_DIRTY;

	/* Contents of 'lost+found' directory */
	LOG_DBG("Lost found dir blk: %d", lost_found_dir_blk_num);
	lost_found_dir_blk = ext2_get_block(fs, lost_found_dir_blk_num);
	if (lost_found_dir_blk == NULL) {
		ret = ENOMEM;
		goto out;
	}
	memset(lost_found_dir_blk->data, 0, cfg->block_size);

	current_size = 0;

	de = (struct ext2_disk_dentry *)lost_found_dir_blk->data;
	ext2_fill_direntry(de, ".", 1, lost_found_inode, EXT2_FT_DIR);
	current_size += de->de_rec_len;

	de = EXT2_NEXT_DISK_DIRENTRY(de);
	ext2_fill_direntry(de, "..", 2, EXT2_ROOT_INODE, EXT2_FT_DIR);
	current_size += de->de_rec_len;

	/* This was the last entry so add padding until end of block */
	de->de_rec_len += cfg->block_size - current_size;

	LOG_DBG("Initialized directory entry %p{%s(%d) %d %d %c}",
			de, de->de_name, de->de_name_len, de->de_inode, de->de_rec_len,
			de->de_file_type == EXT2_FT_DIR ? 'd' : 'f');

	lost_found_dir_blk->flags |= EXT2_BLOCK_DIRTY;

out:
	ext2_drop_block(fs, sb_block);
	ext2_drop_block(fs, bg_block);
	ext2_drop_block(fs, bbitmap_block);
	ext2_drop_block(fs, ibitmap_block);
	ext2_drop_block(fs, itable_block1);
	ext2_drop_block(fs, itable_block2);
	ext2_drop_block(fs, root_dir_blk);
	ext2_drop_block(fs, lost_found_dir_blk);
	fs->backend_ops->sync(fs);
	return ret;
}
