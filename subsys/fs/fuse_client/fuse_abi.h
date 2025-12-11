/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

/*
 * This file is based on include/uapi/linux/fuse.h from Linux, and is used
 * under the BSD-2-Clause license, as per the dual-license option
 */
/*
 * This file defines the kernel interface of FUSE
 * This -- and only this -- header file may also be distributed under
 * the terms of the BSD Licence as follows:
 *
 * Copyright (C) 2001-2007 Miklos Szeredi. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ZEPHYR_SUBSYS_FS_FUSE_ABI_H_
#define ZEPHYR_SUBSYS_FS_FUSE_ABI_H_
#include <stdint.h>

#define FUSE_MAJOR_VERSION 7
#define FUSE_MINOR_VERSION 31

#define FUSE_LOOKUP 1
#define FUSE_FORGET 2
#define FUSE_SETATTR 4
#define FUSE_MKDIR 9
#define FUSE_UNLINK 10
#define FUSE_RMDIR 11
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
	int32_t error;
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
	uint64_t newdir;
};

struct fuse_kstatfs {
	uint64_t blocks;
	uint64_t bfree;
	uint64_t bavail;
	uint64_t files;
	uint64_t ffree;
	uint32_t bsize;
	uint32_t namelen;
	uint32_t frsize;
	uint32_t padding;
	uint32_t spare[6];
};

struct fuse_dirent {
	uint64_t ino;
	uint64_t off;
	uint32_t namelen;
	uint32_t type;
	char name[];
};

struct fuse_forget_in {
	uint64_t nlookup;
};

#endif /* ZEPHYR_SUBSYS_FS_FUSE_ABI_H_ */
