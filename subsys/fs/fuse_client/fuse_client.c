/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fuse_client.h>
#include <zephyr/logging/log.h>
#include <inttypes.h>

LOG_MODULE_REGISTER(fuse, CONFIG_FUSE_CLIENT_LOG_LEVEL);

static uint64_t unique = 1; /* with unique==0 older virtiofsd asserts, so we are starting from 1 */

static uint64_t fuse_get_unique(void)
{
	return unique++;
}

void fuse_fill_header(struct fuse_in_header *hdr, uint32_t len, uint32_t opcode, uint64_t nodeid)
{
	hdr->len = len;
	hdr->opcode = opcode;
	hdr->unique = fuse_get_unique();
	hdr->nodeid = nodeid;
	hdr->uid = CONFIG_FUSE_CLIENT_UID_VALUE;
	hdr->gid = CONFIG_FUSE_CLIENT_GID_VALUE;
	hdr->pid = CONFIG_FUSE_CLIENT_PID_VALUE;
	hdr->total_extlen = 0;
}

void fuse_create_init_req(struct fuse_init_req *req)
{
	fuse_fill_header(
		&req->in_header, sizeof(struct fuse_in_header) + sizeof(struct fuse_init_in),
		FUSE_INIT, 0
	);
	req->init_in.major = FUSE_MAJOR_VERSION;
	req->init_in.minor = FUSE_MINOR_VERSION;
	req->init_in.max_readahead = 0;
	req->init_in.flags = 0;
	req->init_in.flags2 = 0;
}

void fuse_create_open_req(
	struct fuse_open_req *req, uint64_t inode, uint32_t flags, enum fuse_object_type type)
{
	fuse_fill_header(
		&req->in_header, sizeof(struct fuse_in_header) + sizeof(struct fuse_open_in),
		type == FUSE_DIR ? FUSE_OPENDIR : FUSE_OPEN, inode
	);
	req->open_in.flags = flags;
	req->open_in.open_flags = 0;
}

void fuse_create_lookup_req(struct fuse_lookup_req *req, uint64_t inode, uint32_t fname_len)
{
	fuse_fill_header(
		&req->in_header, sizeof(struct fuse_in_header) + fname_len, FUSE_LOOKUP,
		inode
	);
}

void fuse_create_read_req(
	struct fuse_read_req *req, uint64_t inode, uint64_t fh, uint64_t offset, uint32_t size,
	enum fuse_object_type type)
{
	fuse_fill_header(
		&req->in_header, sizeof(struct fuse_in_header) + sizeof(struct fuse_read_in),
		type == FUSE_FILE ? FUSE_READ : FUSE_READDIR, inode
	);
	req->read_in.fh = fh;
	req->read_in.offset = offset;
	req->read_in.size = size;
	req->read_in.read_flags = 0;
	req->read_in.lock_owner = 0;
	req->read_in.flags = 0;
}

void fuse_create_release_req(struct fuse_release_req *req, uint64_t inode, uint64_t fh,
	enum fuse_object_type type)
{
	fuse_fill_header(
		&req->in_header, sizeof(struct fuse_in_header) + sizeof(struct fuse_release_in),
		type == FUSE_DIR ? FUSE_RELEASEDIR : FUSE_RELEASE, inode
	);
	req->release_in.fh = fh;
	req->release_in.flags = 0;
	req->release_in.release_flags = 0;
	req->release_in.lock_owner = 0;
}

void fuse_create_destroy_req(struct fuse_destroy_req *req)
{
	fuse_fill_header(&req->in_header, sizeof(struct fuse_in_header), FUSE_DESTROY, 0);
}

void fuse_create_create_req(
	struct fuse_create_req *req, uint64_t inode, uint32_t fname_len, uint32_t flags,
	uint32_t mode)
{
	fuse_fill_header(
		&req->in_header, sizeof(req->in_header) + sizeof(req->create_in) + fname_len,
		FUSE_CREATE, inode
	);
	req->create_in.flags = flags;
	req->create_in.mode = mode;
	req->create_in.open_flags = 0;
	req->create_in.umask = 0;
}

void fuse_create_write_req(
	struct fuse_write_req *req, uint64_t inode, uint64_t fh, uint64_t offset, uint32_t size)
{
	fuse_fill_header(
		&req->in_header, sizeof(req->in_header) + sizeof(req->write_in) + size, FUSE_WRITE,
		inode
	);
	req->write_in.fh = fh;
	req->write_in.offset = offset;
	req->write_in.size = size;
	req->write_in.write_flags = 0;
	req->write_in.lock_owner = 0;
	req->write_in.flags = 0;
}

void fuse_create_lseek_req(
	struct fuse_lseek_req *req, uint64_t inode, uint64_t fh, uint64_t offset, uint32_t whence)
{
	fuse_fill_header(
		&req->in_header, sizeof(req->in_header) + sizeof(req->lseek_in), FUSE_LSEEK, inode
	);
	req->lseek_in.fh = fh;
	req->lseek_in.offset = offset;
	req->lseek_in.whence = whence;
}

void fuse_create_setattr_req(struct fuse_setattr_req *req, uint64_t inode)
{
	fuse_fill_header(
		&req->in_header, sizeof(req->in_header) + sizeof(struct fuse_setattr_in),
		FUSE_SETATTR, inode
	);
}

void fuse_create_fsync_req(struct fuse_fsync_req *req, uint64_t inode, uint64_t fh)
{
	fuse_fill_header(
		&req->in_header, sizeof(req->in_header) + sizeof(req->fsync_in), FUSE_FSYNC,
		inode
	);
	req->fsync_in.fh = fh;
	req->fsync_in.fsync_flags = 0;
}

void fuse_create_mkdir_req(
	struct fuse_mkdir_req *req, uint64_t inode, uint32_t dirname_len, uint32_t mode)
{
	fuse_fill_header(
		&req->in_header, sizeof(req->in_header) + sizeof(req->mkdir_in) + dirname_len,
		FUSE_MKDIR, inode
	);

	req->mkdir_in.mode = mode;
	req->mkdir_in.umask = 0;
}

void fuse_create_unlink_req(
	struct fuse_unlink_req *req, uint32_t fname_len, enum fuse_object_type type)
{
	fuse_fill_header(
		&req->in_header, sizeof(req->in_header) + fname_len,
		type == FUSE_DIR ? FUSE_RMDIR : FUSE_UNLINK, FUSE_ROOT_INODE
	);
}

void fuse_create_rename_req(
	struct fuse_rename_req *req, uint64_t old_dir_nodeid, uint32_t old_len,
	uint64_t new_dir_nodeid, uint32_t new_len)
{
	fuse_fill_header(
		&req->in_header,
		sizeof(req->in_header) + sizeof(req->rename_in) + old_len + new_len,
		FUSE_RENAME, old_dir_nodeid
	);
	req->rename_in.newdir = new_dir_nodeid;
}

const char *fuse_opcode_to_string(uint32_t opcode)
{
	switch (opcode) {
	case FUSE_LOOKUP:
		return "FUSE_LOOKUP";
	case FUSE_FORGET:
		return "FUSE_FORGET";
	case FUSE_SETATTR:
		return "FUSE_SETATTR";
	case FUSE_MKDIR:
		return "FUSE_MKDIR";
	case FUSE_UNLINK:
		return "FUSE_UNLINK";
	case FUSE_RMDIR:
		return "FUSE_RMDIR";
	case FUSE_RENAME:
		return "FUSE_RENAME";
	case FUSE_OPEN:
		return "FUSE_OPEN";
	case FUSE_READ:
		return "FUSE_READ";
	case FUSE_WRITE:
		return "FUSE_WRITE";
	case FUSE_STATFS:
		return "FUSE_STATFS";
	case FUSE_RELEASE:
		return "FUSE_RELEASE";
	case FUSE_FSYNC:
		return "FUSE_FSYNC";
	case FUSE_INIT:
		return "FUSE_INIT";
	case FUSE_OPENDIR:
		return "FUSE_OPENDIR";
	case FUSE_READDIR:
		return "FUSE_READDIR";
	case FUSE_RELEASEDIR:
		return "FUSE_RELEASEDIR";
	case FUSE_CREATE:
		return "FUSE_CREATE";
	case FUSE_DESTROY:
		return "FUSE_DESTROY";
	case FUSE_LSEEK:
		return "FUSE_LSEEK";
	default:
		return "";
	}
}

void fuse_dump_init_req_out(struct fuse_init_req *req)
{
	LOG_INF(
		"FUSE_INIT response:\n"
		"major=%" PRIu32 "\n"
		"minor=%" PRIu32 "\n"
		"max_readahead=%" PRIu32 "\n"
		"flags=%" PRIu32 "\n"
		"max_background=%" PRIu16 "\n"
		"congestion_threshold=%" PRIu16 "\n"
		"max_write=%" PRIu32 "\n"
		"time_gran=%" PRIu32 "\n"
		"max_pages=%" PRIu16 "\n"
		"map_alignment=%" PRIu16 "\n"
		"flags2=%" PRIu32 "\n"
		"max_stack_depth=%" PRIu32,
		req->init_out.major,
		req->init_out.minor,
		req->init_out.max_readahead,
		req->init_out.flags,
		req->init_out.max_background,
		req->init_out.congestion_threshold,
		req->init_out.max_write,
		req->init_out.time_gran,
		req->init_out.max_pages,
		req->init_out.map_alignment,
		req->init_out.flags2,
		req->init_out.max_stack_depth
	);
}

void fuse_dump_entry_out(struct fuse_entry_out *eo)
{
	LOG_INF(
		"FUSE LOOKUP response:\n"
		"nodeid=%" PRIu64 "\n"
		"generation=%" PRIu64 "\n"
		"entry_valid=%" PRIu64 "\n"
		"attr_valid=%" PRIu64 "\n"
		"entry_valid_nsec=%" PRIu32 "\n"
		"attr_valid_nsec=%" PRIu32 "\n"
		"attr.ino=%" PRIu64 "\n"
		"attr.size=%" PRIu64 "\n"
		"attr.blocks=%" PRIu64 "\n"
		"attr.atime=%" PRIu64 "\n"
		"attr.mtime=%" PRIu64 "\n"
		"attr.ctime=%" PRIu64 "\n"
		"attr.atimensec=%" PRIu32 "\n"
		"attr.mtimensec=%" PRIu32 "\n"
		"attr.ctimensec=%" PRIu32 "\n"
		"attr.mode=%" PRIu32 "\n"
		"attr.nlink=%" PRIu32 "\n"
		"attr.uid=%" PRIu32 "\n"
		"attr.gid=%" PRIu32 "\n"
		"attr.rdev=%" PRIu32 "\n"
		"attr.blksize=%" PRIu32 "\n"
		"attr.flags=%" PRIu32,
		eo->nodeid,
		eo->generation,
		eo->entry_valid,
		eo->attr_valid,
		eo->entry_valid_nsec,
		eo->attr_valid_nsec,
		eo->attr.ino,
		eo->attr.size,
		eo->attr.blocks,
		eo->attr.atime,
		eo->attr.mtime,
		eo->attr.ctime,
		eo->attr.atimensec,
		eo->attr.mtimensec,
		eo->attr.ctimensec,
		eo->attr.mode,
		eo->attr.nlink,
		eo->attr.uid,
		eo->attr.gid,
		eo->attr.rdev,
		eo->attr.blksize,
		eo->attr.flags
	);
}

void fuse_dump_open_req_out(struct fuse_open_req *req)
{
	LOG_INF(
		"FUSE OPEN response:\n"
		"fh=%" PRIu64 "\n"
		"open_flags=%" PRIu32 "\n"
		"backing_id=%" PRIi32,
		req->open_out.fh,
		req->open_out.open_flags,
		req->open_out.backing_id
	);
}

void fuse_dump_create_req_out(struct fuse_create_out *req)
{
	LOG_INF(
		"FUSE CREATE response:\n"
		"nodeid=%" PRIu64 "\n"
		"generation=%" PRIu64 "\n"
		"entry_valid=%" PRIu64 "\n"
		"attr_valid=%" PRIu64 "\n"
		"entry_valid_nsec=%" PRIu32 "\n"
		"attr_valid_nsec=%" PRIu32 "\n"
		"attr.ino=%" PRIu64 "\n"
		"attr.size=%" PRIu64 "\n"
		"attr.blocks=%" PRIu64 "\n"
		"attr.atime=%" PRIu64 "\n"
		"attr.mtime=%" PRIu64 "\n"
		"attr.ctime=%" PRIu64 "\n"
		"attr.atimensec=%" PRIu32 "\n"
		"attr.mtimensec=%" PRIu32 "\n"
		"attr.ctimensec=%" PRIu32 "\n"
		"attr.mode=%" PRIu32 "\n"
		"attr.nlink=%" PRIu32 "\n"
		"attr.uid=%" PRIu32 "\n"
		"attr.gid=%" PRIu32 "\n"
		"attr.rdev=%" PRIu32 "\n"
		"attr.blksize=%" PRIu32 "\n"
		"attr.flags=%" PRIu32 "\n"
		"fh=%" PRIu64 "\n"
		"open_flags=%" PRIu32 "\n"
		"backing_id=%" PRIi32,
		req->entry_out.nodeid,
		req->entry_out.generation,
		req->entry_out.entry_valid,
		req->entry_out.attr_valid,
		req->entry_out.entry_valid_nsec,
		req->entry_out.attr_valid_nsec,
		req->entry_out.attr.ino,
		req->entry_out.attr.size,
		req->entry_out.attr.blocks,
		req->entry_out.attr.atime,
		req->entry_out.attr.mtime,
		req->entry_out.attr.ctime,
		req->entry_out.attr.atimensec,
		req->entry_out.attr.mtimensec,
		req->entry_out.attr.ctimensec,
		req->entry_out.attr.mode,
		req->entry_out.attr.nlink,
		req->entry_out.attr.uid,
		req->entry_out.attr.gid,
		req->entry_out.attr.rdev,
		req->entry_out.attr.blksize,
		req->entry_out.attr.flags,
		req->open_out.fh,
		req->open_out.open_flags,
		req->open_out.backing_id
	);
}

void fuse_dump_write_out(struct fuse_write_out *wo)
{
	LOG_INF("FUSE WRITE response:\nsize=%" PRIu32, wo->size);
}

void fuse_dump_lseek_out(struct fuse_lseek_out *lo)
{
	LOG_INF("FUSE WRITE response:\noffset=%" PRIu64, lo->offset);
}

void fuse_dump_attr_out(struct fuse_attr_out *ao)
{
	LOG_INF(
		"attr_valid=%" PRIu64 "\n"
		"attr_valid_nsec=%" PRIu32,
		ao->attr_valid,
		ao->attr_valid_nsec
	);
}

void fuse_dump_kstafs(struct fuse_kstatfs *ks)
{
	LOG_INF(
		"blocks=%" PRIu64 "\n"
		"bfree=%" PRIu64 "\n"
		"bavail=%" PRIu64 "\n"
		"files=%" PRIu64 "\n"
		"ffree=%" PRIu64 "\n"
		"bsize=%" PRIu32 "\n"
		"namelen=%" PRIu32 "\n"
		"frsize=%" PRIu32,
		ks->blocks,
		ks->bfree,
		ks->bavail,
		ks->files,
		ks->ffree,
		ks->bsize,
		ks->namelen,
		ks->frsize
	);
}
