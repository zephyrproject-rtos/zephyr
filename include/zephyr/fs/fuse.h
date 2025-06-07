/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_FS_FUSE_H_
#define ZEPHYR_SUBSYS_FS_FUSE_H_
#include <stdint.h>
#include <zephyr/kernel.h>

/* adapted from Linux: include/uapi/linux/fuse.h */

#define FUSE_MAJOR_VERSION 7
#define FUSE_MINOR_VERSION 31

#define FUSE_LOOKUP 1
#define FUSE_FORGET 2
#define FUSE_SETATTR 4
#define FUSE_MKDIR 9
#define FUSE_UNLINK 10
#define FUSE_RENAME 12
#define FUSE_OPEN 14
#define FUSE_READ 15
#define FUSE_WRITE 16
#define FUSE_STATFS 17
#define FUSE_RELEASE 18
#define FUSE_FSYNC 20
#define FUSE_INIT 26
#define FUSE_OPENDIR 27
#define FUSE_READDIR 28
#define FUSE_RELEASEDIR 29
#define FUSE_CREATE 35
#define FUSE_DESTROY 38
#define FUSE_LSEEK 46

#define FUSE_ROOT_INODE 1

struct fuse_in_header {
	uint32_t len;
	uint32_t opcode;
	uint64_t unique;
	uint64_t nodeid;
	uint32_t uid;
	uint32_t gid;
	uint32_t pid;
	uint16_t total_extlen;
	uint16_t padding;
};

struct fuse_out_header {
	uint32_t len;
	int32_t	error;
	uint64_t unique;
};

struct fuse_init_in {
	uint32_t major;
	uint32_t minor;
	uint32_t max_readahead;
	uint32_t flags;
	uint32_t flags2;
	uint32_t unused[11];
};

struct fuse_init_out {
	uint32_t major;
	uint32_t minor;
	uint32_t max_readahead;
	uint32_t flags;
	uint16_t max_background;
	uint16_t congestion_threshold;
	uint32_t max_write;
	uint32_t time_gran;
	uint16_t max_pages;
	uint16_t map_alignment;
	uint32_t flags2;
	uint32_t max_stack_depth;
	uint32_t unused[6];
};

struct fuse_open_in {
	uint32_t flags;
	uint32_t open_flags;
};

struct fuse_open_out {
	uint64_t fh;
	uint32_t open_flags;
	int32_t backing_id;
};

struct fuse_attr {
	uint64_t ino;
	uint64_t size;
	uint64_t blocks;
	uint64_t atime;
	uint64_t mtime;
	uint64_t ctime;
	uint32_t atimensec;
	uint32_t mtimensec;
	uint32_t ctimensec;
	uint32_t mode;
	uint32_t nlink;
	uint32_t uid;
	uint32_t gid;
	uint32_t rdev;
	uint32_t blksize;
	uint32_t flags;
};

struct fuse_entry_out {
	uint64_t nodeid;
	uint64_t generation;
	uint64_t entry_valid;
	uint64_t attr_valid;
	uint32_t entry_valid_nsec;
	uint32_t attr_valid_nsec;
	struct fuse_attr attr;
};

struct fuse_read_in {
	uint64_t fh;
	uint64_t offset;
	uint32_t size;
	uint32_t read_flags;
	uint64_t lock_owner;
	uint32_t flags;
	uint32_t padding;
};

struct fuse_release_in {
	uint64_t fh;
	uint32_t flags;
	uint32_t release_flags;
	uint64_t lock_owner;
};

struct fuse_create_in {
	uint32_t flags;
	uint32_t mode;
	uint32_t umask;
	uint32_t open_flags;
};

struct fuse_create_out {
	struct fuse_entry_out entry_out;
	struct fuse_open_out open_out;
};

struct fuse_write_in {
	uint64_t fh;
	uint64_t offset;
	uint32_t size;
	uint32_t write_flags;
	uint64_t lock_owner;
	uint32_t flags;
	uint32_t padding;
};

struct fuse_write_out {
	uint32_t size;
	uint32_t padding;
};

struct fuse_lseek_in {
	uint64_t fh;
	uint64_t offset;
	uint32_t whence;
	uint32_t padding;
};

struct fuse_lseek_out {
	uint64_t offset;
};

/* mask used to set file size, used in fuse_setattr_in::valid */
#define FATTR_SIZE	(1 << 3)

struct fuse_setattr_in {
	uint32_t valid;
	uint32_t padding;
	uint64_t fh;
	uint64_t size;
	uint64_t lock_owner;
	uint64_t atime;
	uint64_t mtime;
	uint64_t ctime;
	uint32_t atimensec;
	uint32_t mtimensec;
	uint32_t ctimensec;
	uint32_t mode;
	uint32_t unused4;
	uint32_t uid;
	uint32_t gid;
	uint32_t unused5;
};

struct fuse_attr_out {
	uint64_t attr_valid;
	uint32_t attr_valid_nsec;
	uint32_t dummy;
	struct fuse_attr attr;
};

struct fuse_fsync_in {
	uint64_t fh;
	uint32_t fsync_flags;
	uint32_t padding;
};

struct fuse_mkdir_in {
	uint32_t mode;
	uint32_t umask;
};

struct fuse_rename_in {
	uint64_t	newdir;
};

struct fuse_kstatfs {
	uint64_t	blocks;
	uint64_t	bfree;
	uint64_t	bavail;
	uint64_t	files;
	uint64_t	ffree;
	uint32_t	bsize;
	uint32_t	namelen;
	uint32_t	frsize;
	uint32_t	padding;
	uint32_t	spare[6];
};

struct fuse_dirent {
	uint64_t	ino;
	uint64_t	off;
	uint32_t	namelen;
	uint32_t	type;
	char name[];
};

struct fuse_forget_in {
	uint64_t	nlookup;
};

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
void fuse_create_unlink_req(struct fuse_unlink_req *req, uint32_t fname_len);
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

#endif /* ZEPHYR_SUBSYS_FS_FUSE_H_ */
