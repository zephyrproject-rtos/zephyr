/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/fs/ext2.h>
#include <zephyr/sys/byteorder.h>

#include "ext2.h"
#include "ext2_impl.h"
#include "ext2_struct.h"
#include "ext2_diskops.h"

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
	in->i_mode       = sys_cpu_to_le16(EXT2_DEF_DIR_MODE);
	in->i_uid        = 0;
	in->i_size       = sys_cpu_to_le32(nblocks * cfg->block_size);
	in->i_atime      = 0;
	in->i_ctime      = 0;
	in->i_mtime      = 0;
	in->i_dtime      = 0;
	in->i_gid        = 0;
	in->i_blocks     = sys_cpu_to_le32(nblocks * cfg->block_size / 512);
	in->i_flags      = 0;
	in->i_osd1       = 0;
	in->i_generation = 0;
	in->i_file_acl   = 0;
	in->i_dir_acl    = 0;
	in->i_faddr      = 0;
	memset(in->i_block, 0, EXT2_INODE_BLOCKS * sizeof(uint32_t));
}

int ext2_format(struct ext2_data *fs, struct ext2_cfg *cfg)
{
	int rc, ret = 0;

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
	sb->s_inodes_count        = sys_cpu_to_le32(inodes_count);
	sb->s_blocks_count        = sys_cpu_to_le32(blocks_count);
	sb->s_r_blocks_count      = sys_cpu_to_le32(0);
	sb->s_free_blocks_count   = sys_cpu_to_le32(free_blocks);
	sb->s_free_inodes_count   = sys_cpu_to_le32(inodes_count - used_inodes);
	sb->s_first_data_block    = sys_cpu_to_le32(first_data_block);
	sb->s_log_block_size      = sys_cpu_to_le32(block_log_size);
	sb->s_log_frag_size       = sys_cpu_to_le32(block_log_size);
	sb->s_blocks_per_group    = sys_cpu_to_le32(cfg->block_size * 8);
	sb->s_frags_per_group     = sys_cpu_to_le32(cfg->block_size * 8);
	sb->s_inodes_per_group    = sys_cpu_to_le32(inodes_count);
	sb->s_mtime               = sys_cpu_to_le32(0);
	sb->s_wtime               = sys_cpu_to_le32(0);
	sb->s_mnt_count           = sys_cpu_to_le32(0);
	sb->s_max_mnt_count       = sys_cpu_to_le32(-1);
	sb->s_magic               = sys_cpu_to_le32(0xEF53);
	sb->s_state               = sys_cpu_to_le32(EXT2_VALID_FS);
	sb->s_errors              = sys_cpu_to_le32(EXT2_ERRORS_RO);
	sb->s_minor_rev_level     = sys_cpu_to_le32(0);
	sb->s_lastcheck           = sys_cpu_to_le32(0);
	sb->s_checkinterval       = sys_cpu_to_le32(0);
	sb->s_creator_os          = sys_cpu_to_le32(5); /* Unknown OS */
	sb->s_rev_level           = sys_cpu_to_le32(EXT2_DYNAMIC_REV);
	sb->s_def_resuid          = sys_cpu_to_le32(0);
	sb->s_def_resgid          = sys_cpu_to_le32(0);
	sb->s_first_ino           = sys_cpu_to_le32(11);
	sb->s_inode_size          = sys_cpu_to_le32(sizeof(struct ext2_disk_inode));
	sb->s_block_group_nr      = sys_cpu_to_le32(0);
	sb->s_feature_compat      = sys_cpu_to_le32(0);
	sb->s_feature_incompat    = sys_cpu_to_le32(EXT2_FEATURE_INCOMPAT_FILETYPE);
	sb->s_feature_ro_compat   = sys_cpu_to_le32(0);
	sb->s_algo_bitmap         = sys_cpu_to_le32(0);
	sb->s_prealloc_blocks     = sys_cpu_to_le32(0);
	sb->s_prealloc_dir_blocks = sys_cpu_to_le32(0);
	sb->s_journal_inum        = sys_cpu_to_le32(0);
	sb->s_journal_dev         = sys_cpu_to_le32(0);
	sb->s_last_orphan         = sys_cpu_to_le32(0);

	memcpy(sb->s_uuid, cfg->uuid, 16);
	strcpy(sb->s_volume_name, cfg->volume_name);

	if (ext2_write_block(fs, sb_block) < 0) {
		ret = -EIO;
		goto out;
	}

	/* Block descriptor table */

	struct ext2_disk_bgroup *bg = (struct ext2_disk_bgroup *)bg_block->data;

	memset(bg, 0, cfg->block_size);
	bg->bg_block_bitmap      = sys_cpu_to_le32(bbitmap_block_num);
	bg->bg_inode_bitmap      = sys_cpu_to_le32(ibitmap_block_num);
	bg->bg_inode_table       = sys_cpu_to_le32(itable_block_num);
	bg->bg_free_blocks_count = sys_cpu_to_le16(free_blocks);
	bg->bg_free_inodes_count = sys_cpu_to_le16(inodes_count - used_inodes);
	bg->bg_used_dirs_count   = sys_cpu_to_le16(2); /* '/' and 'lost+found' */

	if (ext2_write_block(fs, bg_block) < 0) {
		ret = -EIO;
		goto out;
	}

	/* Block bitmap */
	uint8_t *bbitmap = bbitmap_block->data;

	/* In bitmap we describe blocks starting from s_first_data_block. */
	set_bitmap_padding(bbitmap, blocks_count - sb->s_first_data_block, cfg);
	set_bitmap_bits(bbitmap, used_blocks);
	if (ext2_write_block(fs, bbitmap_block) < 0) {
		ret = -EIO;
		goto out;
	}

	/* Inode bitmap */
	uint8_t *ibitmap = ibitmap_block->data;

	set_bitmap_padding(ibitmap, inodes_count, cfg);
	set_bitmap_bits(ibitmap, used_inodes);
	if (ext2_write_block(fs, ibitmap_block) < 0) {
		ret = -EIO;
		goto out;
	}

	/* Inode table */
	/* Zero inode table */
	for (int i = 0; i < itable_blocks; i++) {
		struct ext2_block *blk = ext2_get_block(fs, itable_block_num + i);

		memset(blk->data, 0, cfg->block_size);
		rc = ext2_write_block(fs, blk);
		ext2_drop_block(blk);
		if (rc < 0) {
			ret = -EIO;
			goto out;
		}
	}

	struct ext2_disk_inode *in;
	int inode_offset;

	/* Set inode 2 ('/' directory) */
	itable_block1 = ext2_get_block(fs, itable_block_num);
	in = (struct ext2_disk_inode *)itable_block1->data;
	inode_offset = EXT2_ROOT_INODE - 1;
	default_directory_inode(&in[inode_offset], 1, cfg);

	in[inode_offset].i_links_count = sys_cpu_to_le16(3); /* 2 from itself, 1 from child */
	in[inode_offset].i_block[0]    = sys_cpu_to_le32(root_dir_blk_num);
	if (ext2_write_block(fs, itable_block1) < 0) {
		ret = -EIO;
		goto out;
	}

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
	in[inode_offset].i_links_count = sys_cpu_to_le16(2); /* 1 from itself, 1 from parent */
	in[inode_offset].i_block[0]    = sys_cpu_to_le32(lost_found_dir_blk_num);
	if (itable_block2) {
		if (ext2_write_block(fs, itable_block2) < 0) {
			ret = -EIO;
			goto out;
		}
	}

	struct ext2_disk_direntry *disk_de;
	struct ext2_direntry *de;
	uint32_t de_offset;

	/* Contents of '/' directory */
	LOG_DBG("Root dir blk: %d", root_dir_blk_num);
	root_dir_blk = ext2_get_block(fs, root_dir_blk_num);
	if (root_dir_blk == NULL) {
		ret = ENOMEM;
		goto out;
	}
	memset(root_dir_blk->data, 0, cfg->block_size);

	de_offset = 0;

	disk_de = EXT2_DISK_DIRENTRY_BY_OFFSET(root_dir_blk->data, de_offset);
	de = ext2_create_direntry(".", 1, EXT2_ROOT_INODE, EXT2_FT_DIR);
	ext2_write_direntry(disk_de, de);

	de_offset += de->de_rec_len;
	k_heap_free(&direntry_heap, de);

	disk_de = EXT2_DISK_DIRENTRY_BY_OFFSET(root_dir_blk->data, de_offset);
	de = ext2_create_direntry("..", 2, EXT2_ROOT_INODE, EXT2_FT_DIR);
	ext2_write_direntry(disk_de, de);

	de_offset += de->de_rec_len;
	k_heap_free(&direntry_heap, de);

	char *name = "lost+found";

	disk_de = EXT2_DISK_DIRENTRY_BY_OFFSET(root_dir_blk->data, de_offset);
	de = ext2_create_direntry(name, strlen(name), lost_found_inode, EXT2_FT_DIR);
	de_offset += de->de_rec_len;

	/* This was the last entry so add padding until end of block */
	de->de_rec_len += cfg->block_size - de_offset;

	ext2_write_direntry(disk_de, de);
	k_heap_free(&direntry_heap, de);

	if (ext2_write_block(fs, root_dir_blk) < 0) {
		ret = -EIO;
		goto out;
	}

	/* Contents of 'lost+found' directory */
	LOG_DBG("Lost found dir blk: %d", lost_found_dir_blk_num);
	lost_found_dir_blk = ext2_get_block(fs, lost_found_dir_blk_num);
	if (lost_found_dir_blk == NULL) {
		ret = ENOMEM;
		goto out;
	}
	memset(lost_found_dir_blk->data, 0, cfg->block_size);

	de_offset = 0;

	disk_de = EXT2_DISK_DIRENTRY_BY_OFFSET(lost_found_dir_blk->data, de_offset);
	de = ext2_create_direntry(".", 1, lost_found_inode, EXT2_FT_DIR);
	ext2_write_direntry(disk_de, de);

	de_offset += de->de_rec_len;
	k_heap_free(&direntry_heap, de);

	disk_de = EXT2_DISK_DIRENTRY_BY_OFFSET(lost_found_dir_blk->data, de_offset);
	de = ext2_create_direntry("..", 2, EXT2_ROOT_INODE, EXT2_FT_DIR);
	de_offset += de->de_rec_len;

	/* This was the last entry so add padding until end of block */
	de->de_rec_len += cfg->block_size - de_offset;

	ext2_write_direntry(disk_de, de);
	k_heap_free(&direntry_heap, de);

	if (ext2_write_block(fs, lost_found_dir_blk) < 0) {
		ret = -EIO;
		goto out;
	}
out:
	ext2_drop_block(sb_block);
	ext2_drop_block(bg_block);
	ext2_drop_block(bbitmap_block);
	ext2_drop_block(ibitmap_block);
	ext2_drop_block(itable_block1);
	ext2_drop_block(itable_block2);
	ext2_drop_block(root_dir_blk);
	ext2_drop_block(lost_found_dir_blk);
	if ((ret >= 0) && (fs->backend_ops->sync(fs)) < 0) {
		ret = -EIO;
	}
	return ret;
}
