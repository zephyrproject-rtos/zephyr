/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fs/fs.h>
#include <nffs/queue.h>
#include <nffs/nffs.h>
#include <nffs/os.h>
#include <drivers/flash.h>
#include <ztest_assert.h>
#include "nffs_test_utils.h"

/*
 * This should fit the largest area used in test (128K).
 */
#define AREA_BUF_MAX_SIZE (128 * 1024)
static u8_t area_buf[AREA_BUF_MAX_SIZE];

#define NFFS_TEST_BUF_SIZE (24 * 1024)
u8_t nffs_test_buf[NFFS_TEST_BUF_SIZE];

void nffs_test_util_overwrite_data(u8_t *data, u32_t data_len,
				   u32_t addr)
{
	struct device *dev;
	struct flash_pages_info info;
	off_t off;

	dev = device_get_binding(CONFIG_FS_NFFS_FLASH_DEV_NAME);
	flash_get_page_info_by_offs(dev, addr, &info);

	nffs_os_flash_read(0, info.start_offset, area_buf, info.size);

	/*
	 * To make this simpler, assume we always overwrite within sector
	 *  boundary (which is the case here).
	 */
	off = addr - info.start_offset;
	memcpy(&area_buf[off], data, data_len);

	nffs_os_flash_erase(0, info.start_offset, info.size);
	nffs_os_flash_write(0, info.start_offset, area_buf, info.size);
}

void nffs_test_util_assert_ent_name(struct fs_dirent *fs_dirent,
				    const char *expected_name)
{
	zassert_equal(strcmp(fs_dirent->name, expected_name), 0, NULL);
}

void nffs_test_util_assert_file_len(struct nffs_file *file, u32_t expected)
{
	uint32_t len;
	int rc;

	rc = nffs_inode_data_len(file->nf_inode_entry, &len);
	zassert_equal(rc, 0, NULL);
	zassert_equal(len, expected, NULL);
}

void nffs_test_util_assert_cache_is_sane(const char *filename)
{
	struct nffs_cache_inode *cache_inode;
	struct nffs_cache_block *cache_block;
	struct nffs_file *file;
	struct fs_file_t fs_file;
	uint32_t cache_start;
	uint32_t cache_end;
	uint32_t block_end;
	int rc;

	rc = fs_open(&fs_file, filename);
	zassert_equal(rc, 0, NULL);

	file = fs_file.filep;
	rc = nffs_cache_inode_ensure(&cache_inode, file->nf_inode_entry);
	zassert_equal(rc, 0, NULL);

	nffs_cache_inode_range(cache_inode, &cache_start, &cache_end);

	if (TAILQ_EMPTY(&cache_inode->nci_block_list)) {
		zassert_equal(cache_start, 0, NULL);
		zassert_equal(cache_end, 0, NULL);
	} else {
		block_end = 0U;  /* Pacify gcc. */
		TAILQ_FOREACH(cache_block, &cache_inode->nci_block_list,
			      ncb_link) {
			if (cache_block ==
					TAILQ_FIRST(&cache_inode->nci_block_list)) {
				zassert_equal(cache_block->ncb_file_offset,
						cache_start, NULL);
			} else {
				/* Ensure no gap between this block and its
				 * predecessor.
				 */
				zassert_equal(cache_block->ncb_file_offset,
						block_end, NULL);
			}

			block_end = cache_block->ncb_file_offset +
					cache_block->ncb_block.nb_data_len;
			if (cache_block == TAILQ_LAST(&cache_inode->nci_block_list,
					nffs_cache_block_list)) {
				zassert_equal(block_end, cache_end, NULL);
			}
		}
	}

	rc = fs_close(&fs_file);
	zassert_equal(rc, 0, NULL);
}

void
nffs_test_util_assert_contents(const char *filename, const char *contents,
			       int contents_len)
{
	struct fs_file_t file;
	u32_t bytes_read;
	void *buf;
	int rc;

	rc = fs_open(&file, filename);
	zassert_equal(rc, 0, NULL);

	zassert_true(contents_len <= AREA_BUF_MAX_SIZE, "contents too large");
	buf = area_buf;

	bytes_read = fs_read(&file, buf, contents_len);
	zassert_equal(bytes_read, contents_len, NULL);
	zassert_equal(memcmp(buf, contents, contents_len), 0, NULL);

	rc = fs_close(&file);
	zassert_equal(rc, 0, NULL);

	nffs_test_util_assert_cache_is_sane(filename);
}

int nffs_test_util_block_count(const char *filename)
{
	struct nffs_hash_entry *entry;
	struct nffs_block block;
	struct nffs_file *file;
	struct fs_file_t fs_file;
	int count;
	int rc;

	rc = fs_open(&fs_file, filename);
	zassert_equal(rc, 0, NULL);

	file = fs_file.filep;
	count = 0;
	entry = file->nf_inode_entry->nie_last_block_entry;
	while (entry != NULL) {
		count++;
		rc = nffs_block_from_hash_entry(&block, entry);
		zassert_equal(rc, 0, NULL);
		zassert_not_equal(block.nb_prev, entry, NULL);
		entry = block.nb_prev;
	}

	rc = fs_close(&fs_file);
	zassert_equal(rc, 0, NULL);

	return count;
}

void nffs_test_util_assert_block_count(const char *filename, int expected_count)
{
	int actual_count;

	actual_count = nffs_test_util_block_count(filename);
	zassert_equal(actual_count, expected_count, NULL);
}

void nffs_test_util_assert_cache_range(const char *filename,
				       u32_t expected_cache_start,
				       u32_t expected_cache_end)
{
	struct nffs_cache_inode *cache_inode;
	struct nffs_file *file;
	struct fs_file_t fs_file;
	uint32_t cache_start;
	uint32_t cache_end;
	int rc;

	rc = fs_open(&fs_file, filename);
	zassert_equal(rc, 0, NULL);

	file = fs_file.filep;
	rc = nffs_cache_inode_ensure(&cache_inode, file->nf_inode_entry);
	zassert_equal(rc, 0, NULL);

	nffs_cache_inode_range(cache_inode, &cache_start, &cache_end);
	zassert_equal(cache_start, expected_cache_start, NULL);
	zassert_equal(cache_end, expected_cache_end, NULL);

	rc = fs_close(&fs_file);
	zassert_equal(rc, 0, NULL);

	nffs_test_util_assert_cache_is_sane(filename);
}

void nffs_test_util_create_file_blocks(const char *filename,
				       const struct nffs_test_block_desc *blocks,
				       int num_blocks)
{
	struct fs_file_t file;
	u32_t total_len;
	u32_t offset;
	char *buf;
	int num_writes;
	int rc;
	int i;

	/* We do not have 'truncate' flag in fs_open, so unlink here instead*/
	rc = fs_unlink(filename);
	/* Don't fail on -ENOENT or 0, as can't truncate as file doesn't exists
	 * or 0 on successful, fail on all other error values
	 */
	zassert_true(((rc == 0) || (rc == -ENOENT)), "unlink/truncate failed");

	rc = fs_open(&file, filename);
	zassert_equal(rc, 0, NULL);

	total_len = 0U;
	if (num_blocks <= 0) {
		num_writes = 1;
	} else {
		num_writes = num_blocks;
	}
	for (i = 0; i < num_writes; i++) {
		rc = fs_write(&file, blocks[i].data, blocks[i].data_len);
		zassert_equal(rc, blocks[i].data_len, NULL);

		total_len += blocks[i].data_len;
	}

	rc = fs_close(&file);
	zassert_equal(rc, 0, NULL);

	zassert_true(total_len <= AREA_BUF_MAX_SIZE, "contents too large");
	buf = area_buf;

	offset = 0U;
	for (i = 0; i < num_writes; i++) {
		memcpy(buf + offset, blocks[i].data, blocks[i].data_len);
		offset += blocks[i].data_len;
	}
	zassert_equal(offset, total_len, NULL);

	nffs_test_util_assert_contents(filename, buf, total_len);
	if (num_blocks > 0) {
		nffs_test_util_assert_block_count(filename, num_blocks);
	}
}

void nffs_test_util_create_file(const char *filename, const char *contents,
				int contents_len)
{
	struct nffs_test_block_desc block;

	block.data = contents;
	block.data_len = contents_len;

	nffs_test_util_create_file_blocks(filename, &block, 0);
}

void nffs_test_util_append_file(const char *filename, const char *contents,
				int contents_len)
{
	struct fs_file_t file;
	int rc;

	rc = fs_open(&file, filename);
	zassert_equal(rc, 0, NULL);

	rc = fs_seek(&file, 0, FS_SEEK_END);
	zassert_equal(rc, 0, NULL);

	rc = fs_write(&file, contents, contents_len);
	zassert_equal(rc, contents_len, NULL);

	rc = fs_close(&file);
	zassert_equal(rc, 0, NULL);
}

void nffs_test_copy_area(const struct nffs_area_desc *from,
			 const struct nffs_area_desc *to)
{
	int rc;
	void *buf;

	zassert_equal(from->nad_length, to->nad_length, NULL);

	zassert_true(from->nad_length <= AREA_BUF_MAX_SIZE, "area too large");
	buf = area_buf;

	rc = nffs_os_flash_read(from->nad_flash_id, from->nad_offset, buf,
			from->nad_length);
	zassert_equal(rc, 0, NULL);

	rc = nffs_os_flash_erase(from->nad_flash_id, to->nad_offset,
			to->nad_length);
	zassert_equal(rc, 0, NULL);

	rc = nffs_os_flash_write(to->nad_flash_id, to->nad_offset, buf,
			to->nad_length);
	zassert_equal(rc, 0, NULL);
}

void nffs_test_util_create_subtree(const char *parent_path,
				   const struct nffs_test_file_desc *elem)
{
	char *path;
	int rc;
	int i;

	if (parent_path == NULL) {
		path = k_malloc(1);
		zassert_not_null(path, NULL);
		path[0] = '\0';
	} else {
		path = k_malloc(strlen(parent_path) + strlen(elem->filename) + 2);
		zassert_not_null(path, NULL);

		sprintf(path, "%s/%s", parent_path, elem->filename);
	}

	if (elem->is_dir) {
		if ((parent_path != NULL) &&
				(strlen(parent_path) > strlen(NFFS_MNTP))) {
			rc = fs_mkdir(path);
			zassert_equal(rc, 0, NULL);
		}

		if (elem->children != NULL) {
			for (i = 0; elem->children[i].filename != NULL; i++) {
				nffs_test_util_create_subtree(path,
						elem->children + i);
			}
		}
	} else {
		nffs_test_util_create_file(path, elem->contents,
				elem->contents_len);
	}

	k_free(path);
}

void nffs_test_util_create_tree(const struct nffs_test_file_desc *root_dir)
{
	nffs_test_util_create_subtree(NFFS_MNTP, root_dir);
}

#define NFFS_TEST_TOUCHED_ARR_SZ     (16 * 64)
/*#define NFFS_TEST_TOUCHED_ARR_SZ     (16 * 1024)*/
struct nffs_hash_entry
*nffs_test_touched_entries[NFFS_TEST_TOUCHED_ARR_SZ];
int nffs_test_num_touched_entries;

/* Recursively descend directory structure */
void nffs_test_assert_file(const struct nffs_test_file_desc *file,
			   struct nffs_inode_entry *inode_entry,
			   const char *path)
{
	const struct nffs_test_file_desc *child_file;
	struct nffs_inode inode;
	struct nffs_inode_entry *child_inode_entry;
	char *child_path, *abs_path;
	int child_filename_len;
	int path_len;
	int rc;

	/*
	 * track of hash entries that have been examined
	 */
	zassert_true(nffs_test_num_touched_entries < NFFS_TEST_TOUCHED_ARR_SZ,
		     NULL);
	nffs_test_touched_entries[nffs_test_num_touched_entries] =
			&inode_entry->nie_hash_entry;
	nffs_test_num_touched_entries++;

	path_len = strlen(path);

	rc = nffs_inode_from_entry(&inode, inode_entry);
	zassert_equal(rc, 0, NULL);

	/* recursively examine each child of directory */
	if (nffs_hash_id_is_dir(inode_entry->nie_hash_entry.nhe_id)) {
		for (child_file = file->children;
				child_file != NULL && child_file->filename != NULL;
				child_file++) {

			/*
			 * Construct full pathname for file
			 * Not null terminated
			 */
			child_filename_len = strlen(child_file->filename);
			child_path = k_malloc(path_len + child_filename_len + 2);
			zassert_not_null(child_path, NULL);
			memcpy(child_path, path, path_len);
			child_path[path_len] = '/';
			memcpy(child_path + path_len + 1, child_file->filename,
			       child_filename_len);
			child_path[path_len + 1 + child_filename_len] = '\0';

			/*
			 * Verify child inode can be found using full pathname
			 */
			rc = nffs_path_find_inode_entry(child_path,
					&child_inode_entry);
			zassert_equal(rc, 0, NULL);

			nffs_test_assert_file(child_file, child_inode_entry,
					      child_path);

			k_free(child_path);
		}
	} else {
		abs_path = k_malloc(strlen(NFFS_MNTP) + path_len + 2);
		sprintf(abs_path, "%s%s", NFFS_MNTP, path);
		nffs_test_util_assert_contents(abs_path, file->contents,
				file->contents_len);
		k_free(abs_path);
	}
}

void nffs_test_assert_branch_touched(struct nffs_inode_entry *inode_entry)
{
	struct nffs_inode_entry *child;
	int i;

	if (inode_entry == nffs_lost_found_dir) {
		return;
	}

	for (i = 0; i < nffs_test_num_touched_entries; i++) {
		if (nffs_test_touched_entries[i] == &inode_entry->nie_hash_entry) {
			break;
		}
	}
	zassert_true(i < nffs_test_num_touched_entries, NULL);
	nffs_test_touched_entries[i] = NULL;

	if (nffs_hash_id_is_dir(inode_entry->nie_hash_entry.nhe_id)) {
		SLIST_FOREACH(child, &inode_entry->nie_child_list,
				nie_sibling_next) {
			nffs_test_assert_branch_touched(child);
		}
	}
}

void nffs_test_assert_child_inode_present(struct nffs_inode_entry *child)
{
	const struct nffs_inode_entry *inode_entry;
	const struct nffs_inode_entry *parent;
	struct nffs_inode inode;
	int rc;

	/*
	 * Successfully read inode data from flash
	 */
	rc = nffs_inode_from_entry(&inode, child);
	zassert_equal(rc, 0, NULL);

	/*
	 * Validate parent
	 */
	parent = inode.ni_parent;
	zassert_not_null(parent, NULL);
	zassert_true(nffs_hash_id_is_dir(parent->nie_hash_entry.nhe_id), NULL);

	/*
	 * Make sure inode is in parents child list
	 */
	SLIST_FOREACH(inode_entry, &parent->nie_child_list, nie_sibling_next) {
		if (inode_entry == child) {
			return;
		}
	}

	zassert_true(0, NULL);
}

void nffs_test_assert_block_present(struct nffs_hash_entry *block_entry)
{
	const struct nffs_inode_entry *inode_entry;
	struct nffs_hash_entry *cur;
	struct nffs_block block;
	int rc;

	/*
	 * Successfully read block data from flash
	 */
	rc = nffs_block_from_hash_entry(&block, block_entry);
	zassert_equal(rc, 0, NULL);

	/*
	 * Validate owning inode
	 */
	inode_entry = block.nb_inode_entry;
	zassert_not_null(inode_entry, NULL);
	zassert_true(nffs_hash_id_is_file(inode_entry->nie_hash_entry.nhe_id),
		     NULL);

	/*
	 * Validate that block is in owning inode's block chain
	 */
	cur = inode_entry->nie_last_block_entry;
	while (cur != NULL) {
		if (cur == block_entry) {
			return;
		}

		rc = nffs_block_from_hash_entry(&block, cur);
		zassert_equal(rc, 0, NULL);
		cur = block.nb_prev;
	}

	zassert_true(0, NULL);
}

/*
 * Recursively verify that the children of each directory are sorted
 * on the directory children linked list by filename length
 */
void nffs_test_assert_children_sorted(struct nffs_inode_entry *inode_entry)
{
	struct nffs_inode_entry *child_entry;
	struct nffs_inode_entry *prev_entry;
	struct nffs_inode child_inode;
	struct nffs_inode prev_inode;
	int cmp;
	int rc;

	prev_entry = NULL;
	SLIST_FOREACH(child_entry, &inode_entry->nie_child_list,
		      nie_sibling_next) {
		rc = nffs_inode_from_entry(&child_inode, child_entry);
		zassert_equal(rc, 0, NULL);

		if (prev_entry != NULL) {
			rc = nffs_inode_from_entry(&prev_inode, prev_entry);
			zassert_equal(rc, 0, NULL);

			rc = nffs_inode_filename_cmp_flash(&prev_inode,
					&child_inode, &cmp);
			zassert_equal(rc, 0, NULL);
			zassert_true(cmp < 0, NULL);
		}

		if (nffs_hash_id_is_dir(child_entry->nie_hash_entry.nhe_id)) {
			nffs_test_assert_children_sorted(child_entry);
		}

		prev_entry = child_entry;
	}
}

void nffs_test_assert_system_once(const struct nffs_test_file_desc *root_dir)
{
	struct nffs_inode_entry *inode_entry;
	struct nffs_hash_entry *entry;
	struct nffs_hash_entry *next;
	int i;

	nffs_test_num_touched_entries = 0;
	nffs_test_assert_file(root_dir, nffs_root_dir, "");
	nffs_test_assert_branch_touched(nffs_root_dir);

	/* Ensure no orphaned inodes or blocks. */
	NFFS_HASH_FOREACH(entry, i, next) {
		zassert_true(entry->nhe_flash_loc != NFFS_FLASH_LOC_NONE, NULL);
		if (nffs_hash_id_is_inode(entry->nhe_id)) {
			inode_entry = (void *)entry;
			zassert_equal(inode_entry->nie_refcnt, 1, NULL);
			if (entry->nhe_id == NFFS_ID_ROOT_DIR) {
				zassert_true(inode_entry == nffs_root_dir,
						NULL);
			} else {
				nffs_test_assert_child_inode_present(inode_entry);
			}
		} else {
			nffs_test_assert_block_present(entry);
		}
	}

	/* Ensure proper sorting. */
	nffs_test_assert_children_sorted(nffs_root_dir);
}

void nffs_test_assert_system(const struct nffs_test_file_desc *root_dir,
			     const struct nffs_area_desc *area_descs)
{
	int rc;

	/* Ensure files are as specified, and that there are no other files or
	 * orphaned inodes / blocks.
	 */
	nffs_test_assert_system_once(root_dir);

	/* Force a garbage collection cycle. */
	rc = nffs_gc(NULL);
	zassert_equal(rc, 0, NULL);

	/* Ensure file system is still as expected. */
	nffs_test_assert_system_once(root_dir);

	/* Clear cached data and restore from flash (i.e, simulate a reboot). */
	rc = nffs_misc_reset();
	zassert_equal(rc, 0, NULL);
	rc = nffs_restore_full(area_descs);
	zassert_equal(rc, 0, NULL);

	/* Ensure file system is still as expected. */
	nffs_test_assert_system_once(root_dir);
}

void nffs_test_assert_area_seqs(int seq1, int count1, int seq2, int count2)
{
	struct nffs_disk_area disk_area;
	int cur1;
	int cur2;
	int rc;
	int i;

	cur1 = 0;
	cur2 = 0;

	for (i = 0; i < nffs_num_areas; i++) {
		rc = nffs_flash_read(i, 0, &disk_area, sizeof(disk_area));
		zassert_equal(rc, 0, NULL);
		zassert_true(nffs_area_magic_is_set(&disk_area), NULL);
		zassert_equal(disk_area.nda_gc_seq, nffs_areas[i].na_gc_seq,
			      NULL);
		if (i == nffs_scratch_area_idx) {
			zassert_equal(disk_area.nda_id, NFFS_AREA_ID_NONE,
					NULL);
		}

		if (nffs_areas[i].na_gc_seq == seq1) {
			cur1++;
		} else if (nffs_areas[i].na_gc_seq == seq2) {
			cur2++;
		} else {
			zassert_true(0, NULL);
		}
	}

	zassert_equal(cur1, count1, NULL);
	zassert_equal(cur2, count2, NULL);
}
