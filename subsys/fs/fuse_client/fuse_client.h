/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This header provides helper functions to pack FUSE client requests. Note the client is the
 * side which initiates these requests, and that in a typical FUSE usage the client would be
 * the Linux kernel. While in Zephyr's case this is to enable functionality like the embedded
 * virtiofs client connecting to a virtiofsd daemon running in the host.
 */

#ifndef ZEPHYR_SUBSYS_FS_FUSE_CLIENT_H_
#define ZEPHYR_SUBSYS_FS_FUSE_CLIENT_H_
#include <stdint.h>
#include "fuse_abi.h"

/*
 * requests are put into structs to leverage the fact that they are contiguous in memory and can
 * be passed to virtqueue as smaller amount of buffers, e.g. in_header + init_in can be sent as
 * a single buffer containing both of them instead of two separate buffers
 */

struct fuse_init_req {
	struct fuse_in_header in_header;
	struct fuse_init_in init_in;
	struct fuse_out_header out_header;
	struct fuse_init_out init_out;
};

struct fuse_open_req {
	struct fuse_in_header in_header;
	struct fuse_open_in open_in;
	struct fuse_out_header out_header;
	struct fuse_open_out open_out;
};

struct fuse_create_req {
	struct fuse_in_header in_header;
	struct fuse_create_in create_in;
	struct fuse_out_header out_header;
	struct fuse_create_out create_out;
};

struct fuse_write_req {
	struct fuse_in_header in_header;
	struct fuse_write_in write_in;
	struct fuse_out_header out_header;
	struct fuse_write_out write_out;
};

struct fuse_lseek_req {
	struct fuse_in_header in_header;
	struct fuse_lseek_in lseek_in;
	struct fuse_out_header out_header;
	struct fuse_lseek_out lseek_out;
};

struct fuse_mkdir_req {
	struct fuse_in_header in_header;
	struct fuse_mkdir_in mkdir_in;
	struct fuse_out_header out_header;
	struct fuse_entry_out entry_out;
};

struct fuse_lookup_req {
	struct fuse_in_header in_header;
	struct fuse_out_header out_header;
	struct fuse_entry_out entry_out;
};

struct fuse_read_req {
	struct fuse_in_header in_header;
	struct fuse_read_in read_in;
	struct fuse_out_header out_header;
};

struct fuse_release_req {
	struct fuse_in_header in_header;
	struct fuse_release_in release_in;
	struct fuse_out_header out_header;
};

struct fuse_destroy_req {
	struct fuse_in_header in_header;
	struct fuse_out_header out_header;
};

struct fuse_setattr_req {
	struct fuse_in_header in_header;
	struct fuse_out_header out_header;
};

struct fuse_fsync_req {
	struct fuse_in_header in_header;
	struct fuse_fsync_in fsync_in;
	struct fuse_out_header out_header;
};

struct fuse_unlink_req {
	struct fuse_in_header in_header;
	struct fuse_out_header out_header;
};

struct fuse_rename_req {
	struct fuse_in_header in_header;
	struct fuse_rename_in rename_in;
	struct fuse_out_header out_header;
};

struct fuse_kstatfs_req {
	struct fuse_in_header in_header;
	struct fuse_out_header out_header;
	struct fuse_kstatfs kstatfs_out;
};

struct fuse_forget_req {
	struct fuse_in_header in_header;
	struct fuse_forget_in forget_in;
};

enum fuse_object_type {
	FUSE_FILE,
	FUSE_DIR
};

void fuse_fill_header(struct fuse_in_header *hdr, uint32_t len, uint32_t opcode, uint64_t nodeid);

void fuse_create_init_req(struct fuse_init_req *req);
void fuse_create_open_req(struct fuse_open_req *req, uint64_t inode, uint32_t flags,
	enum fuse_object_type type);
void fuse_create_lookup_req(struct fuse_lookup_req *req, uint64_t inode, uint32_t fname_len);
void fuse_create_read_req(
	struct fuse_read_req *req, uint64_t inode, uint64_t fh, uint64_t offset, uint32_t size,
	enum fuse_object_type type);
void fuse_create_release_req(struct fuse_release_req *req, uint64_t inode, uint64_t fh,
	enum fuse_object_type type);
void fuse_create_destroy_req(struct fuse_destroy_req *req);
void fuse_create_create_req(
	struct fuse_create_req *req, uint64_t inode, uint32_t fname_len, uint32_t flags,
	uint32_t mode);
void fuse_create_write_req(
	struct fuse_write_req *req, uint64_t inode, uint64_t fh,  uint64_t offset, uint32_t size);
void fuse_create_lseek_req(
	struct fuse_lseek_req *req, uint64_t inode, uint64_t fh, uint64_t offset, uint32_t whence);
void fuse_create_setattr_req(struct fuse_setattr_req *req, uint64_t inode);
void fuse_create_fsync_req(struct fuse_fsync_req *req, uint64_t inode, uint64_t fh);
void fuse_create_mkdir_req(
	struct fuse_mkdir_req *req, uint64_t inode, uint32_t dirname_len, uint32_t mode);
void fuse_create_unlink_req(
	struct fuse_unlink_req *req, uint32_t fname_len, enum fuse_object_type type);
void fuse_create_rename_req(
	struct fuse_rename_req *req, uint64_t old_dir_nodeid, uint32_t old_len,
	uint64_t new_dir_nodeid, uint32_t new_len);

const char *fuse_opcode_to_string(uint32_t opcode);

void fuse_dump_init_req_out(struct fuse_init_req *req);
void fuse_dump_entry_out(struct fuse_entry_out *eo);
void fuse_dump_open_req_out(struct fuse_open_req *req);
void fuse_dump_create_req_out(struct fuse_create_out *req);
void fuse_dump_write_out(struct fuse_write_out *wo);
void fuse_dump_lseek_out(struct fuse_lseek_out *lo);
void fuse_dump_attr_out(struct fuse_attr_out *ao);
void fuse_dump_kstafs(struct fuse_kstatfs *ks);

#endif /* ZEPHYR_SUBSYS_FS_FUSE_CLIENT_H_ */
