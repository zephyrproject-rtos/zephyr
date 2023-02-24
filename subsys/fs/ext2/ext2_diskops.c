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

/**
 * @brief Fetch block group and inode table of given inode.
 *
 * @return Offset of inode in currently fetched inode table block.
 */
static int32_t get_itable_entry(struct ext2_data *fs, uint32_t ino)
{
	int rc;
	uint32_t ino_group = (ino - 1) / EXT2_DATA_SBLOCK(fs)->s_inodes_per_group;
	uint32_t ino_index = (ino - 1) % EXT2_DATA_SBLOCK(fs)->s_inodes_per_group;

	LOG_DBG("ino_group:%d ino_index:%d", ino_group, ino_index);

	rc = ext2_fetch_block_group(fs, ino_group);
	if (rc < 0) {
		return rc;
	}

	uint32_t inode_size = EXT2_DATA_SBLOCK(fs)->s_inode_size;
	uint32_t inodes_per_block = fs->block_size / inode_size;

	uint32_t block_index  = ino_index / inodes_per_block;
	uint32_t block_offset = ino_index % inodes_per_block;

	LOG_DBG("block_index:%d block_offset:%d", block_index, block_offset);

	rc = ext2_fetch_bg_itable(fs->bgroup, block_index);
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

	struct ext2_disk_inode *dino = &BGROUP_INODE_TABLE(fs->bgroup)[itable_offset];

	/* Copy needed data into inode structure */
	inode->i_fs = fs;
	inode->flags = 0;
	inode->i_id = ino;
	inode->i_mode = dino->i_mode;
	inode->i_size = dino->i_size;
	inode->i_links_count = dino->i_links_count;
	inode->i_blocks = dino->i_blocks;
	memcpy(inode->i_block, dino->i_block, sizeof(uint32_t) * EXT2_INODE_BLOCKS);

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

		ext2_drop_block(inode->i_fs, inode->blocks[lvl]);

		if (lvl == 0) {
			block = inode->i_block[offsets[0]];
		} else {
			block = ((uint32_t *)inode->blocks[lvl - 1]->data)[offsets[lvl]];
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
	if (block_num == 0) {
		return 0;
	}
	__ASSERT(lvl >= 0 && lvl < MAX_OFFSETS_SIZE,
			"Expected 0 <= lvl < %d (got: lvl=%d)", lvl, MAX_OFFSETS_SIZE);

	int ret;
	int64_t removed = 0, rem;
	uint32_t *list, start_blk;
	struct ext2_block *list_block = NULL;
	bool remove_current = false;

	if (lvl == 0) {
		/* If we got here we will remove this block
		 * and it is also a block with actual inode data, hence we count it.
		 */
		remove_current = true;
		removed++;
	} else {
		list_block = ext2_get_block(fs, block_num);

		if (list_block == NULL) {
			return -ENOENT;
		}
		list = (uint32_t *)list_block->data;

		if (all_zero(offsets, lvl)) {
			/* We remove all blocks that are referenced by current block and current
			 * block isn't needed anymore.
			 */
			remove_current = true;
			start_blk = 0;
		} else {
			/* We don't remove all blocks referenced by current block. */
			start_blk = offsets[0] + 1;
			rem = delete_blocks(fs, list[offsets[0]], lvl - 1, &offsets[1]);
			if (rem < 0) {
				removed = rem;
				goto out;
			}
			removed += rem;
		}

		/* Iterate over blocks that will be entirely deleted */
		for (uint32_t i = start_blk; i < fs->block_size / EXT2_BLOCK_NUM_SIZE; ++i) {
			if (list[i] == 0) {
				continue;
			}
			rem = delete_blocks(fs, list[i], lvl - 1, zero_offsets);
			if (rem < 0) {
				removed = rem;
				goto out;
			}
			removed += rem;
		}
	}

	if (remove_current) {
		LOG_DBG("free block %d (lvl %d)", block_num, lvl);

		ret = ext2_free_block(fs, block_num);
		if (ret < 0) {
			removed = ret;
		}
	}
out:
	ext2_drop_block(fs, list_block);

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

	if (block < 0) {
		return -EINVAL;
	}

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

int inode_remove_blocks(struct ext2_inode *inode, uint32_t first)
{
	uint32_t start;
	int max_lvl;
	int64_t removed;
	uint32_t offsets[4];

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
		start = offsets[0] + 1;
		removed = delete_blocks(inode->i_fs, inode->i_block[offsets[0]],
				block0_level(offsets[0]), &offsets[1]);
		if (removed < 0) {
			return removed;
		}

		inode->i_blocks -= removed * (inode->i_fs->block_size / 512);
	}

	for (uint32_t i = start; i < EXT2_INODE_BLOCKS; i++) {
		removed = delete_blocks(inode->i_fs, inode->i_block[i], block0_level(i),
				zero_offsets);
		if (removed < 0) {
			return removed;
		}
		inode->i_blocks -= removed * (inode->i_fs->block_size / 512);
		inode->i_block[i] = 0;
	}
	return 0;
}

static inline uint32_t get_ngroups(struct ext2_data *fs)
{
	uint32_t ngroups =
		EXT2_DATA_SBLOCK(fs)->s_blocks_count / EXT2_DATA_SBLOCK(fs)->s_blocks_per_group;

	if (EXT2_DATA_SBLOCK(fs)->s_blocks_count % EXT2_DATA_SBLOCK(fs)->s_blocks_per_group != 0) {
		/* there is one more group if the last group is incomplete */
		ngroups += 1;
	}
	return ngroups;
}

int ext2_fetch_block_group(struct ext2_data *fs, uint32_t group)
{
	if (fs->bgroup == NULL) {
		fs->bgroup = ext2_heap_alloc(sizeof(struct ext2_bgroup));
		if (fs->bgroup == NULL) {
			return -ENOMEM;
		}
		memset(fs->bgroup, 0, sizeof(struct ext2_bgroup));
		LOG_DBG("Allocated new bgroup (%p) for fs (%p)", fs->bgroup, fs);
	}

	struct ext2_bgroup *bg = fs->bgroup;

	/* Check if block group is cached */
	if (bg->block && group == bg->num) {
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

	/* fetch block group block if not cached */
	if ((bg->block == NULL) || (block != bg->block->num)) {
		uint32_t global_block = EXT2_DATA_SBLOCK(fs)->s_first_data_block + 1 + block;

		ext2_drop_block(fs, bg->block);
		bg->block = ext2_get_block(fs, global_block);
		if (bg->block == NULL) {
			return -ENOENT;
		}
	}

	/* Invalidate previously fetched blocks */
	ext2_drop_block(fs, bg->inode_table);
	ext2_drop_block(fs, bg->inode_bitmap);
	ext2_drop_block(fs, bg->block_bitmap);
	bg->inode_table = bg->inode_bitmap = bg->block_bitmap = NULL;

	bg->fs = fs;
	bg->num = group;
	bg->num_in_block = offset;

	LOG_DBG("[BG:%d] itable:%d free_blk:%d free_ino:%d useddirs:%d bbitmap:%d ibitmap:%d",
			group, current_disk_bgroup(bg)->bg_inode_table,
			current_disk_bgroup(bg)->bg_free_blocks_count,
			current_disk_bgroup(bg)->bg_free_inodes_count,
			current_disk_bgroup(bg)->bg_used_dirs_count,
			current_disk_bgroup(bg)->bg_block_bitmap,
			current_disk_bgroup(bg)->bg_inode_bitmap);

	return 0;
}

int ext2_fetch_bg_itable(struct ext2_bgroup *bg, uint32_t block)
{
	if (bg->inode_table && bg->inode_table_block == block) {
		return 0;
	}

	struct ext2_data *fs = bg->fs;
	uint32_t global_block = current_disk_bgroup(bg)->bg_inode_table + block;

	ext2_drop_block(fs, bg->inode_table);
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
	uint32_t global_block = current_disk_bgroup(bg)->bg_inode_bitmap;

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
	uint32_t global_block = current_disk_bgroup(bg)->bg_block_bitmap;

	bg->block_bitmap = ext2_get_block(fs, global_block);
	if (bg->block_bitmap == NULL) {
		return -ENOENT;
	}
	return 0;
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
		}

		if (*block == 0) {
			ret = ext2_assign_block_num(fs, inode->blocks[lvl]);
			if (ret < 0) {
				return ret;
			}

			/* Update block from higher level. */
			*block = inode->blocks[lvl]->num;
			if (lvl > 0) {
				inode->blocks[lvl-1]->flags |= EXT2_BLOCK_DIRTY;
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

int ext2_commit_inode_block(struct ext2_inode *inode)
{
	if (!(inode->flags & INODE_FETCHED_BLOCK)) {
		return -EINVAL;
	}

	int ret;

	LOG_DBG("inode:%d current_blk:%d", inode->i_id, inode->block_num);

	ret = alloc_level_blocks(inode);
	inode_current_block(inode)->flags |= EXT2_BLOCK_DIRTY;
	return ret;
}

int ext2_clear_inode(struct ext2_data *fs, uint32_t ino)
{
	int32_t itable_offset = get_itable_entry(fs, ino);

	if (itable_offset < 0) {
		return itable_offset;
	}

	memset(&BGROUP_INODE_TABLE(fs->bgroup)[itable_offset], 0, sizeof(struct ext2_disk_inode));
	fs->bgroup->inode_table->flags |= EXT2_BLOCK_DIRTY;
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
	struct ext2_disk_inode *dino = &BGROUP_INODE_TABLE(fs->bgroup)[itable_offset];

	/* fill dinode */
	dino->i_size = inode->i_size;
	dino->i_mode = inode->i_mode;
	dino->i_links_count = inode->i_links_count;
	dino->i_blocks = inode->i_blocks;
	memcpy(dino->i_block, inode->i_block, sizeof(uint32_t) * 15);

	fs->bgroup->inode_table->flags |= EXT2_BLOCK_DIRTY;

	return 0;
}

int64_t ext2_alloc_block(struct ext2_data *fs)
{
	int rc;
	uint32_t group = 0;

	rc = ext2_fetch_block_group(fs, group);
	if (rc < 0) {
		return rc;
	}

	LOG_DBG("Free blocks: %d", current_disk_bgroup(fs->bgroup)->bg_free_blocks_count);
	while ((rc >= 0) && (current_disk_bgroup(fs->bgroup)->bg_free_blocks_count == 0)) {
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

	rc = ext2_fetch_bg_bbitmap(fs->bgroup);
	if (rc < 0) {
		return rc;
	}

	int r = ext2_bitmap_find_free(BGROUP_BLOCK_BITMAP(fs->bgroup), fs->block_size);

	if (r < 0) {
		LOG_WRN("Cannot find free block in group %d (rc: %d)", group, r);
		return r;
	}

	/* Add 1 because bitmaps describe blocks except the first one */
	int32_t total = group * EXT2_DATA_SBLOCK(fs)->s_blocks_per_group + r + 1;

	LOG_DBG("Found free block %d in group %d (total: %d)", r, group, total);

	rc = ext2_bitmap_set(BGROUP_BLOCK_BITMAP(fs->bgroup), r, fs->block_size);
	if (rc < 0) {
		return rc;
	}

	current_disk_bgroup(fs->bgroup)->bg_free_blocks_count -= 1;
	EXT2_DATA_SBLOCK(fs)->s_free_blocks_count -= 1;

	fs->sblock->flags |= EXT2_BLOCK_DIRTY;
	fs->bgroup->block->flags |= EXT2_BLOCK_DIRTY;
	fs->bgroup->block_bitmap->flags |= EXT2_BLOCK_DIRTY;

	return total;
}

int32_t ext2_alloc_inode(struct ext2_data *fs)
{
	int rc;
	uint32_t group = 0;

	rc = ext2_fetch_block_group(fs, group);

	while (current_disk_bgroup(fs->bgroup)->bg_free_inodes_count == 0 && rc >= 0) {
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

	LOG_DBG("Free inodes (bg): %d", current_disk_bgroup(fs->bgroup)->bg_free_inodes_count);
	LOG_DBG("Free inodes (sb): %d", EXT2_DATA_SBLOCK(fs)->s_free_inodes_count);

	rc = ext2_fetch_bg_ibitmap(fs->bgroup);
	if (rc < 0) {
		return rc;
	}

	int r = ext2_bitmap_find_free(BGROUP_INODE_BITMAP(fs->bgroup), fs->block_size);

	if (r < 0) {
		LOG_DBG("Cannot find free inode in group %d (rc: %d)", group, r);
		return r;
	}

	/* Add 1 because bitmaps describe blocks except the first one */
	int32_t total = group * EXT2_DATA_SBLOCK(fs)->s_inodes_per_group + r + 1;

	LOG_DBG("Found free inode %d in group %d (total: %d)", r, group, total);

	rc = ext2_bitmap_set(BGROUP_INODE_BITMAP(fs->bgroup), r, fs->block_size);
	if (rc < 0) {
		return rc;
	}

	current_disk_bgroup(fs->bgroup)->bg_free_inodes_count -= 1;
	EXT2_DATA_SBLOCK(fs)->s_free_inodes_count -= 1;

	fs->sblock->flags |= EXT2_BLOCK_DIRTY;
	fs->bgroup->block->flags |= EXT2_BLOCK_DIRTY;
	fs->bgroup->inode_bitmap->flags |= EXT2_BLOCK_DIRTY;

	LOG_DBG("Free inodes (bg): %d", current_disk_bgroup(fs->bgroup)->bg_free_inodes_count);
	LOG_DBG("Free inodes (sb): %d", EXT2_DATA_SBLOCK(fs)->s_free_inodes_count);

	return total;
}

int ext2_free_block(struct ext2_data *fs, uint32_t block)
{
	LOG_DBG("Free block %d", block);

	/* Block bitmaps tracks blocks starting from block number 1.
	 * (Block 0 is ommited.)
	 */
	block -= 1;

	int rc;
	uint32_t group = block / EXT2_DATA_SBLOCK(fs)->s_blocks_per_group;
	uint32_t off = block % EXT2_DATA_SBLOCK(fs)->s_blocks_per_group;

	rc = ext2_fetch_block_group(fs, group);
	if (rc < 0) {
		return rc;
	}

	rc = ext2_fetch_bg_bbitmap(fs->bgroup);
	if (rc < 0) {
		return rc;
	}

	rc = ext2_bitmap_unset(BGROUP_BLOCK_BITMAP(fs->bgroup), off, fs->block_size);
	if (rc < 0) {
		return rc;
	}

	current_disk_bgroup(fs->bgroup)->bg_free_blocks_count += 1;
	EXT2_DATA_SBLOCK(fs)->s_free_blocks_count += 1;

	fs->sblock->flags |= EXT2_BLOCK_DIRTY;
	fs->bgroup->block->flags |= EXT2_BLOCK_DIRTY;
	fs->bgroup->block_bitmap->flags |= EXT2_BLOCK_DIRTY;

	return rc;
}
