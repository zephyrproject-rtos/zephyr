/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <stdint.h>

#include "ext2.h"
#include "ext2_struct.h"
#include "ext2_impl.h"
#include "ext2_diskops.h"
#include "ext2_bitmap.h"

LOG_MODULE_DECLARE(ext2);

/* Static declarations */
static int get_level_offsets(struct ext2_data *fs, uint32_t block, uint32_t offsets[4]);
static inline uint32_t get_ngroups(struct ext2_data *fs);

#define MAX_OFFSETS_SIZE 4
/* Array of zeros to be used in inode block calculation */
static const uint32_t zero_offsets[MAX_OFFSETS_SIZE];

static void fill_sblock(struct ext2_superblock *sb, struct ext2_disk_superblock *disk_sb)
{
	sb->s_inodes_count      = sys_le32_to_cpu(disk_sb->s_inodes_count);
	sb->s_blocks_count      = sys_le32_to_cpu(disk_sb->s_blocks_count);
	sb->s_free_blocks_count = sys_le32_to_cpu(disk_sb->s_free_blocks_count);
	sb->s_free_inodes_count = sys_le32_to_cpu(disk_sb->s_free_inodes_count);
	sb->s_first_data_block  = sys_le32_to_cpu(disk_sb->s_first_data_block);
	sb->s_log_block_size    = sys_le32_to_cpu(disk_sb->s_log_block_size);
	sb->s_log_frag_size     = sys_le32_to_cpu(disk_sb->s_log_frag_size);
	sb->s_blocks_per_group  = sys_le32_to_cpu(disk_sb->s_blocks_per_group);
	sb->s_frags_per_group   = sys_le32_to_cpu(disk_sb->s_frags_per_group);
	sb->s_inodes_per_group  = sys_le32_to_cpu(disk_sb->s_inodes_per_group);
	sb->s_mnt_count         = sys_le16_to_cpu(disk_sb->s_mnt_count);
	sb->s_max_mnt_count     = sys_le16_to_cpu(disk_sb->s_max_mnt_count);
	sb->s_magic             = sys_le16_to_cpu(disk_sb->s_magic);
	sb->s_state             = sys_le16_to_cpu(disk_sb->s_state);
	sb->s_errors            = sys_le16_to_cpu(disk_sb->s_errors);
	sb->s_creator_os        = sys_le32_to_cpu(disk_sb->s_creator_os);
	sb->s_rev_level         = sys_le32_to_cpu(disk_sb->s_rev_level);
	sb->s_first_ino         = sys_le32_to_cpu(disk_sb->s_first_ino);
	sb->s_inode_size        = sys_le16_to_cpu(disk_sb->s_inode_size);
	sb->s_block_group_nr    = sys_le16_to_cpu(disk_sb->s_block_group_nr);
	sb->s_feature_compat    = sys_le32_to_cpu(disk_sb->s_feature_compat);
	sb->s_feature_incompat  = sys_le32_to_cpu(disk_sb->s_feature_incompat);
	sb->s_feature_ro_compat = sys_le32_to_cpu(disk_sb->s_feature_ro_compat);
}

static void fill_disk_sblock(struct ext2_disk_superblock *disk_sb, struct ext2_superblock *sb)
{
	disk_sb->s_inodes_count      = sys_cpu_to_le32(sb->s_inodes_count);
	disk_sb->s_blocks_count      = sys_cpu_to_le32(sb->s_blocks_count);
	disk_sb->s_free_blocks_count = sys_cpu_to_le32(sb->s_free_blocks_count);
	disk_sb->s_free_inodes_count = sys_cpu_to_le32(sb->s_free_inodes_count);
	disk_sb->s_first_data_block  = sys_cpu_to_le32(sb->s_first_data_block);
	disk_sb->s_log_block_size    = sys_cpu_to_le32(sb->s_log_block_size);
	disk_sb->s_log_frag_size     = sys_cpu_to_le32(sb->s_log_frag_size);
	disk_sb->s_blocks_per_group  = sys_cpu_to_le32(sb->s_blocks_per_group);
	disk_sb->s_frags_per_group   = sys_cpu_to_le32(sb->s_frags_per_group);
	disk_sb->s_inodes_per_group  = sys_cpu_to_le32(sb->s_inodes_per_group);
	disk_sb->s_mnt_count         = sys_cpu_to_le16(sb->s_mnt_count);
	disk_sb->s_max_mnt_count     = sys_cpu_to_le16(sb->s_max_mnt_count);
	disk_sb->s_magic             = sys_cpu_to_le16(sb->s_magic);
	disk_sb->s_state             = sys_cpu_to_le16(sb->s_state);
	disk_sb->s_errors            = sys_cpu_to_le16(sb->s_errors);
	disk_sb->s_creator_os        = sys_cpu_to_le32(sb->s_creator_os);
	disk_sb->s_rev_level         = sys_cpu_to_le32(sb->s_rev_level);
	disk_sb->s_first_ino         = sys_cpu_to_le32(sb->s_first_ino);
	disk_sb->s_inode_size        = sys_cpu_to_le16(sb->s_inode_size);
	disk_sb->s_block_group_nr    = sys_cpu_to_le16(sb->s_block_group_nr);
	disk_sb->s_feature_compat    = sys_cpu_to_le32(sb->s_feature_compat);
	disk_sb->s_feature_incompat  = sys_cpu_to_le32(sb->s_feature_incompat);
	disk_sb->s_feature_ro_compat = sys_cpu_to_le32(sb->s_feature_ro_compat);
}

static void fill_bgroup(struct ext2_bgroup *bg, struct ext2_disk_bgroup *disk_bg)
{
	bg->bg_block_bitmap      = sys_le32_to_cpu(disk_bg->bg_block_bitmap);
	bg->bg_inode_bitmap      = sys_le32_to_cpu(disk_bg->bg_inode_bitmap);
	bg->bg_inode_table       = sys_le32_to_cpu(disk_bg->bg_inode_table);
	bg->bg_free_blocks_count = sys_le16_to_cpu(disk_bg->bg_free_blocks_count);
	bg->bg_free_inodes_count = sys_le16_to_cpu(disk_bg->bg_free_inodes_count);
	bg->bg_used_dirs_count   = sys_le16_to_cpu(disk_bg->bg_used_dirs_count);
}

static void fill_disk_bgroup(struct ext2_disk_bgroup *disk_bg, struct ext2_bgroup *bg)
{
	disk_bg->bg_block_bitmap      = sys_cpu_to_le32(bg->bg_block_bitmap);
	disk_bg->bg_inode_bitmap      = sys_cpu_to_le32(bg->bg_inode_bitmap);
	disk_bg->bg_inode_table       = sys_cpu_to_le32(bg->bg_inode_table);
	disk_bg->bg_free_blocks_count = sys_cpu_to_le16(bg->bg_free_blocks_count);
	disk_bg->bg_free_inodes_count = sys_cpu_to_le16(bg->bg_free_inodes_count);
	disk_bg->bg_used_dirs_count   = sys_cpu_to_le16(bg->bg_used_dirs_count);
}

static void fill_inode(struct ext2_inode *inode, struct ext2_disk_inode *dino)
{
	inode->i_mode        = sys_le16_to_cpu(dino->i_mode);
	inode->i_size        = sys_le32_to_cpu(dino->i_size);
	inode->i_links_count = sys_le16_to_cpu(dino->i_links_count);
	inode->i_blocks      = sys_le32_to_cpu(dino->i_blocks);
	for (int i = 0; i < EXT2_INODE_BLOCKS; i++) {
		inode->i_block[i] = sys_le32_to_cpu(dino->i_block[i]);
	}
}

static void fill_disk_inode(struct ext2_disk_inode *dino, struct ext2_inode *inode)
{
	dino->i_mode        = sys_cpu_to_le16(inode->i_mode);
	dino->i_size        = sys_cpu_to_le32(inode->i_size);
	dino->i_links_count = sys_cpu_to_le16(inode->i_links_count);
	dino->i_blocks      = sys_cpu_to_le32(inode->i_blocks);
	for (int i = 0; i < EXT2_INODE_BLOCKS; i++) {
		dino->i_block[i] = sys_cpu_to_le32(inode->i_block[i]);
	}
}

struct ext2_direntry *ext2_fetch_direntry(struct ext2_disk_direntry *disk_de)
{

	if (disk_de->de_name_len > EXT2_MAX_FILE_NAME) {
		return NULL;
	}
	uint32_t prog_rec_len = sizeof(struct ext2_direntry) + disk_de->de_name_len;
	struct ext2_direntry *de = k_heap_alloc(&direntry_heap, prog_rec_len, K_FOREVER);

	__ASSERT(de != NULL, "allocated direntry can't be NULL");

	de->de_inode     = sys_le32_to_cpu(disk_de->de_inode);
	de->de_rec_len   = sys_le16_to_cpu(disk_de->de_rec_len);
	de->de_name_len  = disk_de->de_name_len;
	de->de_file_type = disk_de->de_file_type;
	memcpy(de->de_name, disk_de->de_name, de->de_name_len);
	return de;
}

void ext2_write_direntry(struct ext2_disk_direntry *disk_de, struct ext2_direntry *de)
{
	disk_de->de_inode     = sys_le32_to_cpu(de->de_inode);
	disk_de->de_rec_len   = sys_le16_to_cpu(de->de_rec_len);
	disk_de->de_name_len  = de->de_name_len;
	disk_de->de_file_type = de->de_file_type;
	memcpy(disk_de->de_name, de->de_name, de->de_name_len);
}

uint32_t ext2_get_disk_direntry_inode(struct ext2_disk_direntry *de)
{
	return sys_le32_to_cpu(de->de_inode);
}

uint32_t ext2_get_disk_direntry_reclen(struct ext2_disk_direntry *de)
{
	return sys_le16_to_cpu(de->de_rec_len);
}

uint8_t ext2_get_disk_direntry_namelen(struct ext2_disk_direntry *de)
{
	return de->de_name_len;
}

uint8_t ext2_get_disk_direntry_type(struct ext2_disk_direntry *de)
{
	return de->de_file_type;
}

void ext2_set_disk_direntry_inode(struct ext2_disk_direntry *de, uint32_t inode)
{
	de->de_inode = sys_cpu_to_le32(inode);
}

void ext2_set_disk_direntry_reclen(struct ext2_disk_direntry *de, uint16_t reclen)
{
	de->de_rec_len = sys_cpu_to_le16(reclen);
}

void ext2_set_disk_direntry_namelen(struct ext2_disk_direntry *de, uint8_t namelen)
{
	de->de_name_len = namelen;
}

void ext2_set_disk_direntry_type(struct ext2_disk_direntry *de, uint8_t type)
{
	de->de_file_type = type;
}

void ext2_set_disk_direntry_name(struct ext2_disk_direntry *de, const char *name, size_t len)
{
	memcpy(de->de_name, name, len);
}

int ext2_fetch_superblock(struct ext2_data *fs)
{
	struct ext2_block *b;
	uint32_t sblock_offset;

	if (fs->block_size == 1024) {
		sblock_offset = 0;
		b = ext2_get_block(fs, 1);
	} else {
		sblock_offset = 1024;
		b = ext2_get_block(fs, 0);
	}
	if (b == NULL) {
		return -ENOENT;
	}

	struct ext2_disk_superblock *disk_sb =
		(struct ext2_disk_superblock *)(b->data + sblock_offset);

	fill_sblock(&fs->sblock, disk_sb);

	ext2_drop_block(b);
	return 0;
}

static inline uint32_t get_ngroups(struct ext2_data *fs)
{
	uint32_t ngroups =
		fs->sblock.s_blocks_count / fs->sblock.s_blocks_per_group;

	if (fs->sblock.s_blocks_count % fs->sblock.s_blocks_per_group != 0) {
		/* there is one more group if the last group is incomplete */
		ngroups += 1;
	}
	return ngroups;
}

int ext2_fetch_block_group(struct ext2_data *fs, uint32_t group)
{
	struct ext2_bgroup *bg = &fs->bgroup;

	/* Check if block group is cached */
	if (group == bg->num) {
		return 0;
	}

	uint32_t ngroups = get_ngroups(fs);

	LOG_DBG("ngroups:%d", ngroups);
	LOG_DBG("cur_group:%d fetch_group:%d", bg->num, group);

	if (group > ngroups) {
		return -ERANGE;
	}

	uint32_t groups_per_block = fs->block_size / sizeof(struct ext2_disk_bgroup);
	uint32_t block = group / groups_per_block;
	uint32_t offset = group % groups_per_block;
	uint32_t global_block = fs->sblock.s_first_data_block + 1 + block;

	struct ext2_block *b = ext2_get_block(fs, global_block);

	if (b == NULL) {
		return -ENOENT;
	}

	struct ext2_disk_bgroup *disk_bg = ((struct ext2_disk_bgroup *)b->data) + offset;

	fill_bgroup(bg, disk_bg);

	/* Drop unused block */
	ext2_drop_block(b);

	/* Invalidate previously fetched blocks */
	ext2_drop_block(bg->inode_table);
	ext2_drop_block(bg->inode_bitmap);
	ext2_drop_block(bg->block_bitmap);
	bg->inode_table = bg->inode_bitmap = bg->block_bitmap = NULL;

	bg->fs = fs;
	bg->num = group;

	LOG_DBG("[BG:%d] itable:%d free_blk:%d free_ino:%d useddirs:%d bbitmap:%d ibitmap:%d",
			group, bg->bg_inode_table,
			bg->bg_free_blocks_count,
			bg->bg_free_inodes_count,
			bg->bg_used_dirs_count,
			bg->bg_block_bitmap,
			bg->bg_inode_bitmap);
	return 0;
}

int ext2_fetch_bg_itable(struct ext2_bgroup *bg, uint32_t block)
{
	if (bg->inode_table && bg->inode_table_block == block) {
		return 0;
	}

	struct ext2_data *fs = bg->fs;
	uint32_t global_block = bg->bg_inode_table + block;

	ext2_drop_block(bg->inode_table);
	bg->inode_table = ext2_get_block(fs, global_block);
	if (bg->inode_table == NULL) {
		return -ENOENT;
	}

	bg->inode_table_block = block;
	return 0;
}

int ext2_fetch_bg_ibitmap(struct ext2_bgroup *bg)
{
	if (bg->inode_bitmap) {
		return 0;
	}

	struct ext2_data *fs = bg->fs;
	uint32_t global_block = bg->bg_inode_bitmap;

	bg->inode_bitmap = ext2_get_block(fs, global_block);
	if (bg->inode_bitmap == NULL) {
		return -ENOENT;
	}
	return 0;
}

int ext2_fetch_bg_bbitmap(struct ext2_bgroup *bg)
{
	if (bg->block_bitmap) {
		return 0;
	}

	struct ext2_data *fs = bg->fs;
	uint32_t global_block = bg->bg_block_bitmap;

	bg->block_bitmap = ext2_get_block(fs, global_block);
	if (bg->block_bitmap == NULL) {
		return -ENOENT;
	}
	return 0;
}

/**
 * @brief Fetch block group and inode table of given inode.
 *
 * @return Offset of inode in currently fetched inode table block.
 */
static int32_t get_itable_entry(struct ext2_data *fs, uint32_t ino)
{
	int rc;
	uint32_t ino_group = (ino - 1) / fs->sblock.s_inodes_per_group;
	uint32_t ino_index = (ino - 1) % fs->sblock.s_inodes_per_group;

	LOG_DBG("ino_group:%d ino_index:%d", ino_group, ino_index);

	rc = ext2_fetch_block_group(fs, ino_group);
	if (rc < 0) {
		return rc;
	}

	uint32_t inode_size = fs->sblock.s_inode_size;
	uint32_t inodes_per_block = fs->block_size / inode_size;

	uint32_t block_index  = ino_index / inodes_per_block;
	uint32_t block_offset = ino_index % inodes_per_block;

	LOG_DBG("block_index:%d block_offset:%d", block_index, block_offset);

	rc = ext2_fetch_bg_itable(&fs->bgroup, block_index);
	if (rc < 0) {
		return rc;
	}
	return block_offset;
}

int ext2_fetch_inode(struct ext2_data *fs, uint32_t ino, struct ext2_inode *inode)
{

	int32_t itable_offset = get_itable_entry(fs, ino);

	LOG_DBG("fetch inode: %d", ino);

	if (itable_offset < 0) {
		return itable_offset;
	}

	struct ext2_disk_inode *dino = &BGROUP_INODE_TABLE(&fs->bgroup)[itable_offset];

	fill_inode(inode, dino);

	/* Copy needed data into inode structure */
	inode->i_fs = fs;
	inode->flags = 0;
	inode->i_id = ino;

	LOG_DBG("mode:%d size:%d links:%d", dino->i_mode, dino->i_size, dino->i_links_count);
	return 0;
}

/*
 * @param try_current -- if true then check if searched offset matches offset of currently fetched
 *        block on that level. If they match then it is the block we are looking for.
 */
static int fetch_level_blocks(struct ext2_inode *inode, uint32_t offsets[4], int lvl, int max_lvl,
		bool try_current)
{
	uint32_t block;
	bool already_fetched = try_current && (offsets[lvl] == inode->offsets[lvl]);

	/* all needed blocks fetched */
	if (lvl > max_lvl) {
		return 0;
	}

	/* If already fetched block matches desired one we can use it and move to the next level. */
	if (!already_fetched) {
		/* Fetched block on current level was wrong.
		 * We can't use fetched blocks on this and next levels.
		 */
		try_current = false;

		ext2_drop_block(inode->blocks[lvl]);

		if (lvl == 0) {
			block = inode->i_block[offsets[0]];
		} else {
			uint32_t *list = (uint32_t *)inode->blocks[lvl - 1]->data;

			block = sys_le32_to_cpu(list[offsets[lvl]]);
		}

		if (block == 0) {
			inode->blocks[lvl] = ext2_get_empty_block(inode->i_fs);
		} else {
			inode->blocks[lvl] = ext2_get_block(inode->i_fs, block);
		}

		if (inode->blocks[lvl] == NULL) {
			return -ENOENT;
		}
		LOG_DBG("[fetch] lvl:%d off:%d num:%d", lvl, offsets[lvl], block);
	}
	return fetch_level_blocks(inode, offsets, lvl + 1, max_lvl, try_current);
}

int ext2_fetch_inode_block(struct ext2_inode *inode, uint32_t block)
{
	/* Check if correct inode block is cached. */
	if (inode->flags & INODE_FETCHED_BLOCK && inode->block_num == block) {
		return 0;
	}

	LOG_DBG("inode:%d cur_blk:%d fetch_blk:%d", inode->i_id, inode->block_num, block);

	struct ext2_data *fs = inode->i_fs;
	int max_lvl, ret;
	uint32_t offsets[MAX_OFFSETS_SIZE];
	bool try_current = inode->flags & INODE_FETCHED_BLOCK;

	max_lvl = get_level_offsets(fs, block, offsets);

	ret = fetch_level_blocks(inode, offsets, 0, max_lvl, try_current);
	if (ret < 0) {
		ext2_inode_drop_blocks(inode);
		return ret;
	}

	memcpy(inode->offsets, offsets, MAX_OFFSETS_SIZE * sizeof(uint32_t));
	inode->block_lvl = max_lvl;
	inode->block_num = block;
	inode->flags |= INODE_FETCHED_BLOCK;

	LOG_DBG("[ino:%d fetch]\t Lvl:%d {%d, %d, %d, %d}", inode->i_id, inode->block_lvl,
			inode->offsets[0], inode->offsets[1], inode->offsets[2], inode->offsets[3]);
	return 0;
}

static bool all_zero(const uint32_t *offsets, int lvl)
{
	for (int i = 0; i < lvl; ++i) {
		if (offsets[i] > 0) {
			return false;
		}
	}
	return true;
}

/**
 * @brief delete blocks from one described with offsets array
 *
 * NOTE: To use this function safely drop all fetched inode blocks
 *
 * @retval >=0 Number of removed blocks (only the blocks with actual inode data)
 * @retval <0 Error
 */
static int64_t delete_blocks(struct ext2_data *fs, uint32_t block_num, int lvl,
		const uint32_t *offsets)
{
	__ASSERT(block_num != 0, "Can't delete zero block");
	__ASSERT(lvl >= 0 && lvl < MAX_OFFSETS_SIZE,
			"Expected 0 <= lvl < %d (got: lvl=%d)", lvl, MAX_OFFSETS_SIZE);

	int ret;
	int64_t removed = 0, rem;
	uint32_t *list, start_blk;
	struct ext2_block *list_block = NULL;
	bool remove_current = false;
	bool block_dirty = false;

	if (lvl == 0) {
		/* If we got here we will remove this block
		 * and it is also a block with actual inode data, hence we count it.
		 */
		remove_current = true;
		removed++;
	} else {
		/* Current block holds a list of blocks. */
		list_block = ext2_get_block(fs, block_num);

		if (list_block == NULL) {
			return -ENOENT;
		}
		list = (uint32_t *)list_block->data;

		if (all_zero(offsets, lvl)) {
			/* We remove all blocks that are referenced by current block, hence current
			 * block isn't needed anymore.
			 */
			remove_current = true;
			start_blk = 0;

		} else if (lvl == 1) {
			/* We are on one before last layer of inode block table. The next layer are
			 * single blocks, hence we will just remove them.
			 * We can just set start_blk here and remove blocks in loop at the end of
			 * this function.
			 */
			start_blk = offsets[0];

		} else {
			uint32_t block_num2 = sys_le32_to_cpu(list[offsets[0]]);

			/* We don't remove all blocks referenced by current block. We have to use
			 * offsets to decide which part of next block we want to remove.
			 */
			if (block_num2 == 0) {
				LOG_ERR("Inode block that references other blocks must be nonzero");
				fs->flags |= EXT2_DATA_FLAGS_ERR;
				removed = -EINVAL;
				goto out;
			}

			/* We will start removing whole blocks from next block on this level */
			start_blk = offsets[0] + 1;

			/* Remove desired part of lower level block. */
			rem = delete_blocks(fs, block_num2, lvl - 1, &offsets[1]);
			if (rem < 0) {
				removed = rem;
				goto out;
			}
			removed += rem;
		}

		/* Iterate over blocks that will be entirely deleted */
		for (uint32_t i = start_blk; i < fs->block_size / EXT2_BLOCK_NUM_SIZE; ++i) {
			uint32_t block_num2 = sys_le32_to_cpu(list[i]);

			if (block_num2 == 0) {
				continue;
			}
			rem = delete_blocks(fs, block_num2, lvl - 1, zero_offsets);
			if (rem < 0) {
				removed = rem;
				goto out;
			}
			removed += rem;
			list[i] = 0;
			block_dirty = true;
		}
	}

	if (remove_current) {
		LOG_DBG("free block %d (lvl %d)", block_num, lvl);

		/* If we remove current block, we don't have to write it's updated content. */
		if (list_block) {
			block_dirty = false;
		}

		ret = ext2_free_block(fs, block_num);
		if (ret < 0) {
			removed = ret;
		}
	}
out:
	if (removed >= 0 && list_block && block_dirty) {
		ret = ext2_write_block(fs, list_block);
		if (ret < 0) {
			removed = ret;
		}
	}
	ext2_drop_block(list_block);

	/* On error removed will contain negative error code */
	return removed;
}

static int get_level_offsets(struct ext2_data *fs, uint32_t block, uint32_t offsets[4])
{
	const uint32_t B = fs->block_size / EXT2_BLOCK_NUM_SIZE;
	const uint32_t lvl0_blks = EXT2_INODE_BLOCK_1LVL;
	const uint32_t lvl1_blks = B;
	const uint32_t lvl2_blks = B * B;
	const uint32_t lvl3_blks = B * B * B;

	/* Level 0 */
	if (block < lvl0_blks) {
		offsets[0] = block;
		return 0;
	}

	/* Level 1 */
	block -= lvl0_blks;
	if (block < lvl1_blks) {
		offsets[0] = EXT2_INODE_BLOCK_1LVL;
		 offsets[1] = block;
		return 1;
	}

	/* Level 2 */
	block -= lvl1_blks;
	if (block < lvl2_blks) {
		offsets[0] = EXT2_INODE_BLOCK_2LVL;
		offsets[1] = block / B;
		offsets[2] = block % B;
		return 2;
	}

	/* Level 3 */
	if (block < lvl3_blks) {
		block -= lvl2_blks;
		offsets[0] = EXT2_INODE_BLOCK_3LVL;
		offsets[1] = block / (B * B);
		offsets[2] = (block % (B * B)) / B;
		offsets[3] = (block % (B * B)) % B;
		return 3;
	}
	/* Block number is too large */
	return -EINVAL;
}

static int block0_level(uint32_t block)
{
	if (block >= EXT2_INODE_BLOCK_1LVL) {
		return block - EXT2_INODE_BLOCK_1LVL + 1;
	}
	return 0;
}

int64_t ext2_inode_remove_blocks(struct ext2_inode *inode, uint32_t first)
{
	uint32_t start;
	int max_lvl;
	int64_t ret, removed = 0;
	uint32_t offsets[4];
	struct ext2_data *fs = inode->i_fs;

	max_lvl = get_level_offsets(inode->i_fs, first, offsets);

	if (all_zero(offsets, max_lvl)) {
		/* We remove also the first block because all blocks referenced from it will be
		 * deleted.
		 */
		start = offsets[0];
	} else {
		/* There will be some blocks referenced from first affected block hence we can't
		 * remove it.
		 */
		if (inode->i_block[offsets[0]] == 0) {
			LOG_ERR("Inode block that references other blocks must be nonzero");
			fs->flags |= EXT2_DATA_FLAGS_ERR;
			return -EINVAL;
		}

		start = offsets[0] + 1;
		ret = delete_blocks(inode->i_fs, inode->i_block[offsets[0]],
				block0_level(offsets[0]), &offsets[1]);
		if (ret < 0) {
			return ret;
		}
		removed += ret;
	}

	for (uint32_t i = start; i < EXT2_INODE_BLOCKS; i++) {
		if (inode->i_block[i] == 0) {
			continue;
		}
		ret = delete_blocks(inode->i_fs, inode->i_block[i], block0_level(i),
				zero_offsets);
		if (ret < 0) {
			return ret;
		}
		removed += ret;
		inode->i_block[i] = 0;
	}
	return removed;
}
static int alloc_level_blocks(struct ext2_inode *inode)
{
	int ret = 0;
	uint32_t *block;
	bool allocated = false;
	struct ext2_data *fs = inode->i_fs;

	for (int lvl = 0; lvl <= inode->block_lvl; ++lvl) {
		if (lvl == 0) {
			block = &inode->i_block[inode->offsets[lvl]];
		} else {
			block = &((uint32_t *)inode->blocks[lvl - 1]->data)[inode->offsets[lvl]];
			*block = sys_le32_to_cpu(*block);
		}

		if (*block == 0) {
			ret = ext2_assign_block_num(fs, inode->blocks[lvl]);
			if (ret < 0) {
				return ret;
			}

			/* Update block from higher level. */
			*block = sys_cpu_to_le32(inode->blocks[lvl]->num);
			if (lvl > 0) {
				ret = ext2_write_block(fs, inode->blocks[lvl-1]);
				if (ret < 0) {
					return ret;
				}
			}
			allocated = true;
			/* Allocating block on that level implies that blocks on lower levels will
			 * be allocated too hence we can set allocated here.
			 */
			LOG_DBG("Alloc lvl:%d (num: %d) %s", lvl, *block,
					lvl == inode->block_lvl ? "data" : "indirect");
		}
	}
	if (allocated) {
		/* Update number of reserved blocks.
		 * (We are always counting 512 size blocks.)
		 */
		inode->i_blocks += fs->block_size / 512;
		ret = ext2_commit_inode(inode);
	}
	return ret;
}

int ext2_commit_superblock(struct ext2_data *fs)
{
	int ret;
	struct ext2_block *b;
	uint32_t sblock_offset;

	if (fs->block_size == 1024) {
		sblock_offset = 0;
		b = ext2_get_block(fs, 1);
	} else {
		sblock_offset = 1024;
		b = ext2_get_block(fs, 0);
	}
	if (b == NULL) {
		return -ENOENT;
	}

	struct ext2_disk_superblock *disk_sb =
		(struct ext2_disk_superblock *)(b->data + sblock_offset);

	fill_disk_sblock(disk_sb, &fs->sblock);

	ret = ext2_write_block(fs, b);
	if (ret < 0) {
		return ret;
	}
	ext2_drop_block(b);
	return 0;
}

int ext2_commit_bg(struct ext2_data *fs)
{
	int ret;
	struct ext2_bgroup *bg = &fs->bgroup;

	uint32_t groups_per_block = fs->block_size / sizeof(struct ext2_disk_bgroup);
	uint32_t block = bg->num / groups_per_block;
	uint32_t offset = bg->num % groups_per_block;
	uint32_t global_block = fs->sblock.s_first_data_block + 1 + block;

	struct ext2_block *b = ext2_get_block(fs, global_block);

	if (b == NULL) {
		return -ENOENT;
	}

	struct ext2_disk_bgroup *disk_bg = ((struct ext2_disk_bgroup *)b->data) + offset;

	fill_disk_bgroup(disk_bg, bg);

	ret = ext2_write_block(fs, b);
	if (ret < 0) {
		return ret;
	}
	ext2_drop_block(b);
	return 0;
}

int ext2_commit_inode(struct ext2_inode *inode)
{
	struct ext2_data *fs = inode->i_fs;

	int32_t itable_offset = get_itable_entry(fs, inode->i_id);

	if (itable_offset < 0) {
		return itable_offset;
	}

	/* get pointer to proper inode in fetched block */
	struct ext2_disk_inode *dino = &BGROUP_INODE_TABLE(&fs->bgroup)[itable_offset];

	/* fill dinode */
	fill_disk_inode(dino, inode);

	return ext2_write_block(fs, fs->bgroup.inode_table);
}

int ext2_commit_inode_block(struct ext2_inode *inode)
{
	if (!(inode->flags & INODE_FETCHED_BLOCK)) {
		return -EINVAL;
	}

	int ret;

	LOG_DBG("inode:%d current_blk:%d", inode->i_id, inode->block_num);

	ret = alloc_level_blocks(inode);
	if (ret < 0) {
		return ret;
	}
	ret = ext2_write_block(inode->i_fs, inode_current_block(inode));
	return ret;
}

int ext2_clear_inode(struct ext2_data *fs, uint32_t ino)
{
	int ret;
	int32_t itable_offset = get_itable_entry(fs, ino);

	if (itable_offset < 0) {
		return itable_offset;
	}

	memset(&BGROUP_INODE_TABLE(&fs->bgroup)[itable_offset], 0, sizeof(struct ext2_disk_inode));
	ret = ext2_write_block(fs, fs->bgroup.inode_table);
	return ret;
}

int64_t ext2_alloc_block(struct ext2_data *fs)
{
	int rc, bitmap_slot;
	uint32_t group = 0, set;
	int32_t total;

	rc = ext2_fetch_block_group(fs, group);
	if (rc < 0) {
		return rc;
	}

	LOG_DBG("Free blocks: %d", fs->bgroup.bg_free_blocks_count);
	while ((rc >= 0) && (fs->bgroup.bg_free_blocks_count == 0)) {
		group++;
		rc = ext2_fetch_block_group(fs, group);
		if (rc == -ERANGE) {
			/* reached last group */
			return -ENOSPC;
		}
	}
	if (rc < 0) {
		return rc;
	}

	rc = ext2_fetch_bg_bbitmap(&fs->bgroup);
	if (rc < 0) {
		return rc;
	}

	bitmap_slot = ext2_bitmap_find_free(BGROUP_BLOCK_BITMAP(&fs->bgroup), fs->block_size);
	if (bitmap_slot < 0) {
		LOG_WRN("Cannot find free block in group %d (rc: %d)", group, bitmap_slot);
		return bitmap_slot;
	}

	/* In bitmap blocks are counted from s_first_data_block hence we have to add this offset. */
	total = group * fs->sblock.s_blocks_per_group + bitmap_slot + fs->sblock.s_first_data_block;

	LOG_DBG("Found free block %d in group %d (total: %d)", bitmap_slot, group, total);

	rc = ext2_bitmap_set(BGROUP_BLOCK_BITMAP(&fs->bgroup), bitmap_slot, fs->block_size);
	if (rc < 0) {
		return rc;
	}

	fs->bgroup.bg_free_blocks_count -= 1;
	fs->sblock.s_free_blocks_count -= 1;

	set = ext2_bitmap_count_set(BGROUP_BLOCK_BITMAP(&fs->bgroup), fs->sblock.s_blocks_count);

	if (set != (fs->sblock.s_blocks_count - fs->sblock.s_free_blocks_count)) {
		error_behavior(fs, "Wrong number of used blocks in superblock and bitmap");
		return -EINVAL;
	}

	rc = ext2_commit_superblock(fs);
	if (rc < 0) {
		LOG_DBG("super block write returned: %d", rc);
		return -EIO;
	}
	rc = ext2_commit_bg(fs);
	if (rc < 0) {
		LOG_DBG("block group write returned: %d", rc);
		return -EIO;
	}
	rc = ext2_write_block(fs, fs->bgroup.block_bitmap);
	if (rc < 0) {
		LOG_DBG("block bitmap write returned: %d", rc);
		return -EIO;
	}
	return total;
}

static int check_zero_inode(struct ext2_data *fs, uint32_t ino)
{
	int32_t itable_offset = get_itable_entry(fs, ino);

	if (itable_offset < 0) {
		return itable_offset;
	}

	uint8_t *bytes = (uint8_t *)&BGROUP_INODE_TABLE(&fs->bgroup)[itable_offset];

	for (int i = 0; i < sizeof(struct ext2_disk_inode); ++i) {
		if (bytes[i] != 0) {
			return -EINVAL;
		}
	}
	return 0;
}

int32_t ext2_alloc_inode(struct ext2_data *fs)
{
	int rc, r;
	uint32_t group = 0, set;
	int32_t global_idx;

	rc = ext2_fetch_block_group(fs, group);

	while (fs->bgroup.bg_free_inodes_count == 0 && rc >= 0) {
		group++;
		rc = ext2_fetch_block_group(fs, group);
		if (rc == -ERANGE) {
			/* reached last group */
			return -ENOSPC;
		}
	}

	if (rc < 0) {
		return rc;
	}

	LOG_DBG("Free inodes (bg): %d", fs->bgroup.bg_free_inodes_count);
	LOG_DBG("Free inodes (sb): %d", fs->sblock.s_free_inodes_count);

	rc = ext2_fetch_bg_ibitmap(&fs->bgroup);
	if (rc < 0) {
		return rc;
	}

	r = ext2_bitmap_find_free(BGROUP_INODE_BITMAP(&fs->bgroup), fs->block_size);
	if (r < 0) {
		LOG_DBG("Cannot find free inode in group %d (rc: %d)", group, r);
		return r;
	}

	/* Add 1 because inodes are counted from 1 not 0. */
	global_idx = group * fs->sblock.s_inodes_per_group + r + 1;

	/* Inode table entry for found inode must be cleared. */
	if (check_zero_inode(fs, global_idx) != 0) {
		error_behavior(fs,  "Inode is not cleared in inode table!");
		return -EINVAL;
	}

	LOG_DBG("Found free inode %d in group %d (global_idx: %d)", r, group, global_idx);

	rc = ext2_bitmap_set(BGROUP_INODE_BITMAP(&fs->bgroup), r, fs->block_size);
	if (rc < 0) {
		return rc;
	}

	fs->bgroup.bg_free_inodes_count -= 1;
	fs->sblock.s_free_inodes_count -= 1;

	set = ext2_bitmap_count_set(BGROUP_INODE_BITMAP(&fs->bgroup), fs->sblock.s_inodes_count);

	if (set != fs->sblock.s_inodes_count - fs->sblock.s_free_inodes_count) {
		error_behavior(fs, "Wrong number of used inodes in superblock and bitmap");
		return -EINVAL;
	}

	rc = ext2_commit_superblock(fs);
	if (rc < 0) {
		LOG_DBG("super block write returned: %d", rc);
		return -EIO;
	}
	rc = ext2_commit_bg(fs);
	if (rc < 0) {
		LOG_DBG("block group write returned: %d", rc);
		return -EIO;
	}
	rc = ext2_write_block(fs, fs->bgroup.inode_bitmap);
	if (rc < 0) {
		LOG_DBG("block bitmap write returned: %d", rc);
		return -EIO;
	}

	LOG_DBG("Free inodes (bg): %d", fs->bgroup.bg_free_inodes_count);
	LOG_DBG("Free inodes (sb): %d", fs->sblock.s_free_inodes_count);

	return global_idx;
}

int ext2_free_block(struct ext2_data *fs, uint32_t block)
{
	LOG_DBG("Free block %d", block);

	/* Block bitmaps tracks blocks starting from s_first_data_block. */
	block -= fs->sblock.s_first_data_block;

	int rc;
	uint32_t group = block / fs->sblock.s_blocks_per_group;
	uint32_t off = block % fs->sblock.s_blocks_per_group;
	uint32_t set;

	rc = ext2_fetch_block_group(fs, group);
	if (rc < 0) {
		return rc;
	}

	rc = ext2_fetch_bg_bbitmap(&fs->bgroup);
	if (rc < 0) {
		return rc;
	}

	rc = ext2_bitmap_unset(BGROUP_BLOCK_BITMAP(&fs->bgroup), off, fs->block_size);
	if (rc < 0) {
		return rc;
	}

	fs->bgroup.bg_free_blocks_count += 1;
	fs->sblock.s_free_blocks_count += 1;

	set = ext2_bitmap_count_set(BGROUP_BLOCK_BITMAP(&fs->bgroup), fs->sblock.s_blocks_count);

	if (set != fs->sblock.s_blocks_count - fs->sblock.s_free_blocks_count) {
		error_behavior(fs, "Wrong number of used blocks in superblock and bitmap");
		return -EINVAL;
	}

	rc = ext2_commit_superblock(fs);
	if (rc < 0) {
		LOG_DBG("super block write returned: %d", rc);
		return -EIO;
	}
	rc = ext2_commit_bg(fs);
	if (rc < 0) {
		LOG_DBG("block group write returned: %d", rc);
		return -EIO;
	}
	rc = ext2_write_block(fs, fs->bgroup.block_bitmap);
	if (rc < 0) {
		LOG_DBG("block bitmap write returned: %d", rc);
		return -EIO;
	}
	return 0;
}

int ext2_free_inode(struct ext2_data *fs, uint32_t ino, bool directory)
{
	LOG_DBG("Free inode %d", ino);

	int rc;
	uint32_t group = (ino - 1) / fs->sblock.s_inodes_per_group;
	uint32_t bitmap_off = (ino - 1) % fs->sblock.s_inodes_per_group;
	uint32_t set;

	rc = ext2_fetch_block_group(fs, group);
	if (rc < 0) {
		return rc;
	}

	rc = ext2_fetch_bg_ibitmap(&fs->bgroup);
	if (rc < 0) {
		return rc;
	}

	rc = ext2_bitmap_unset(BGROUP_INODE_BITMAP(&fs->bgroup), bitmap_off, fs->block_size);
	if (rc < 0) {
		return rc;
	}

	rc = ext2_clear_inode(fs, ino);
	if (rc < 0) {
		return rc;
	}

	fs->bgroup.bg_free_inodes_count += 1;
	fs->sblock.s_free_inodes_count += 1;

	if (directory) {
		fs->bgroup.bg_used_dirs_count -= 1;
	}

	set = ext2_bitmap_count_set(BGROUP_INODE_BITMAP(&fs->bgroup), fs->sblock.s_inodes_count);

	if (set != fs->sblock.s_inodes_count - fs->sblock.s_free_inodes_count) {
		error_behavior(fs, "Wrong number of used inodes in superblock and bitmap");
		return -EINVAL;
	}

	LOG_INF("Inode %d is free", ino);

	rc = ext2_commit_superblock(fs);
	if (rc < 0) {
		LOG_DBG("super block write returned: %d", rc);
		return -EIO;
	}
	rc = ext2_commit_bg(fs);
	if (rc < 0) {
		LOG_DBG("block group write returned: %d", rc);
		return -EIO;
	}
	rc = ext2_write_block(fs, fs->bgroup.inode_bitmap);
	if (rc < 0) {
		LOG_DBG("block bitmap write returned: %d", rc);
		return -EIO;
	}
	rc = fs->backend_ops->sync(fs);
	if (rc < 0) {
		return -EIO;
	}
	return 0;
}
