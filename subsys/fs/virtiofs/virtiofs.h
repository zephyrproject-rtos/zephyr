/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_FS_VIRTIOFS_VIRTIOFS_H_
#define ZEPHYR_SUBSYS_FS_VIRTIOFS_VIRTIOFS_H_
#include <zephyr/device.h>
#include <../fuse_client/fuse_client.h>

int virtiofs_init(const struct device *dev, struct fuse_init_out *response);
int virtiofs_lookup(
	const struct device *dev, uint64_t inode, const char *name, struct fuse_entry_out *response,
	uint64_t *parent_inode);
int virtiofs_open(
	const struct device *dev, uint64_t inode, uint32_t flags, struct fuse_open_out *response,
	enum fuse_object_type type);
int virtiofs_read(
	const struct device *dev, uint64_t inode, uint64_t fh,
	uint64_t offset, uint32_t size, uint8_t *buf);
int virtiofs_release(const struct device *dev, uint64_t inode, uint64_t fh,
	enum fuse_object_type type);
int virtiofs_destroy(const struct device *dev);
int virtiofs_create(
	const struct device *dev, uint64_t inode, const char *fname, uint32_t flags,
	uint32_t mode, struct fuse_create_out *response);
int virtiofs_write(
	const struct device *dev, uint64_t inode, uint64_t fh, uint64_t offset,
	uint32_t size, const uint8_t *write_buf);
int virtiofs_lseek(
	const struct device *dev, uint64_t inode, uint64_t fh, uint64_t offset,
	uint32_t whence, struct fuse_lseek_out *response);
int virtiofs_setattr(
	const struct device *dev, uint64_t inode, struct fuse_setattr_in *in,
	struct fuse_attr_out *response);
int virtiofs_fsync(const struct device *dev, uint64_t inode, uint64_t fh);
int virtiofs_mkdir(const struct device *dev, uint64_t inode, const char *dirname, uint32_t mode);
int virtiofs_unlink(const struct device *dev, const char *fname, enum fuse_object_type type);
int virtiofs_rename(
	const struct device *dev, uint64_t old_dir_inode, const char *old_name,
	uint64_t new_dir_inode, const char *new_name);
int virtiofs_statfs(const struct device *dev, struct fuse_kstatfs *response);
int virtiofs_readdir(
	const struct device *dev, uint64_t inode, uint64_t fh, uint64_t offset,
	uint8_t *dirent_buf, uint32_t dirent_size, uint8_t *name_buf, uint32_t name_size);
void virtiofs_forget(const struct device *dev, uint64_t inode, uint64_t nlookup);

#endif /* ZEPHYR_SUBSYS_FS_VIRTIOFS_VIRTIOFS_H_ */
