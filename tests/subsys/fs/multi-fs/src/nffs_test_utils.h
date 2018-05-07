/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_NFFS_TEST_UTILS_
#define H_NFFS_TEST_UTILS_

#include <zephyr/types.h>
#include <nffs/nffs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NFFS_MNTP	"/nffs"

extern struct nffs_area_desc *nffs_default_area_descs;

struct nffs_test_block_desc {
	const char *data;
	int data_len;
};

struct nffs_test_file_desc {
	const char *filename;
	int is_dir;
	const char *contents;
	int contents_len;
	struct nffs_test_file_desc *children;
};

int nffs_test_num_touched_entries;

extern int flash_native_memset(u32_t offset, uint8_t c, u32_t len);
extern u8_t nffs_test_buf[];

void nffs_test_util_overwrite_data(u8_t *data, u32_t data_len, u32_t addr);
void nffs_test_util_assert_ent_name(struct fs_dirent *dirent,
				    const char *expected_name);
void nffs_test_util_assert_file_len(struct nffs_file *file, u32_t expected);
void nffs_test_util_assert_cache_is_sane(const char *filename);
void nffs_test_util_assert_contents(const char *filename,
				    const char *contents, int contents_len);
int nffs_test_util_block_count(const char *filename);
void nffs_test_util_assert_block_count(const char *filename,
				       int expected_count);
void nffs_test_util_assert_cache_range(const char *filename,
				       u32_t expected_cache_start,
				       u32_t expected_cache_end);
void nffs_test_util_create_file_blocks(const char *filename,
				       const struct nffs_test_block_desc *blks,
				       int num_blocks);
void nffs_test_util_create_file(const char *filename, const char *contents,
				int contents_len);
void nffs_test_util_append_file(const char *filename, const char *contents,
				int contents_len);
void nffs_test_copy_area(const struct nffs_area_desc *from,
			 const struct nffs_area_desc *to);
void nffs_test_util_create_subtree(const char *parent_path,
				   const struct nffs_test_file_desc *elem);
void nffs_test_util_create_tree(const struct nffs_test_file_desc *root_dir);

/*
 * Recursively descend directory structure
 */
void nffs_test_assert_file(const struct nffs_test_file_desc *file,
			   struct nffs_inode_entry *inode_entry,
			   const char *path);
void nffs_test_assert_branch_touched(struct nffs_inode_entry *inode_entry);
void nffs_test_assert_child_inode_present(struct nffs_inode_entry *child);
void nffs_test_assert_block_present(struct nffs_hash_entry *block_entry);
/*
 * Recursively verify that the children of each directory are sorted
 * on the directory children linked list by filename length
 */
void nffs_test_assert_children_sorted(struct nffs_inode_entry *inode_entry);
void nffs_test_assert_system_once(const struct nffs_test_file_desc *root_dir);
void nffs_test_assert_system(const struct nffs_test_file_desc *root_dir,
			     const struct nffs_area_desc *area_descs);
void nffs_test_assert_area_seqs(int seq1, int count1, int seq2, int count2);

#ifdef __cplusplus
}
#endif

#endif /* H_NFFS_TEST_UTILS_ */
