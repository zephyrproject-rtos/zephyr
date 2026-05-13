/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openamp/rpmsg.h>
#include <zephyr/fs/rpmsgfs.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(rpmsgfs, CONFIG_FS_LOG_LEVEL);

#include <stdio.h>
#include <string.h>

#include "rpmsgfs_fs_internal.h"

#define RPMSGFS_SERVICE_NAME_PREFIX "rpmsgfs-"
/* Service name is prefix + <pointer hex>. eg: rpmsgfs-0x200041b0 */
#define RPMSGFS_SERVICE_NAME_MAX_SIZE                                                              \
	(sizeof(RPMSGFS_SERVICE_NAME_PREFIX) + sizeof(uint32_t) * 2UL + 2UL)

#if (MAX_FILE_NAME == 12)
#warning MAX_FILE_NAME has been assigned with a short length (12). \
This is probably not what you want. Please check configuration.
#endif

#define MAX_PATH_LEN    255
#define ENOTEMPTY_POSIX 39

/* This structure represents the overall mountpoint state.  An instance of
 * this structure is retained as inode private data on each mountpoint that
 * is mounted with a rpmsgfs filesystem.
 */

struct rpmsgfs_s {
#ifndef CONFIG_FILE_SYSTEM_RPMSGFS_SINGLE_CHANNEL
	struct rpmsg_endpoint ept;
#endif
	int crefs;
	char remote_root[MAX_PATH_LEN + 1];
	size_t remote_root_size;
};

K_MEM_SLAB_DEFINE_STATIC_TYPE(rpmsgfs_slab, struct rpmsgfs_s,
			      CONFIG_FILE_SYSTEM_RPMSGFS_MAX_MOUNTS);

static struct rpmsg_device *g_rpmsg_dev;

#ifdef CONFIG_FILE_SYSTEM_RPMSGFS_SINGLE_CHANNEL
static struct rpmsg_endpoint rpmsgfs_ept;
static bool ept_connected;
#endif

/* When using single channel (CONFIG_FILE_SYSTEM_RPMSGFS_SINGLE_CHANNEL),
 * return shared global definition of RPMsg ept, otherwise return ept assigned
 * to rpmsgfs_s context.
 */
static inline struct rpmsg_endpoint *get_ept(struct rpmsgfs_s *ctx)
{
#ifdef CONFIG_FILE_SYSTEM_RPMSGFS_SINGLE_CHANNEL
	ARG_UNUSED(ctx);
	return &rpmsgfs_ept;
#else
	return &ctx->ept;
#endif
}

static int rpmsgfs_send_nocopy(struct rpmsgfs_s *priv, uint32_t command,
			       struct rpmsgfs_header_s *msg, int len)
{
	struct rpmsg_endpoint *ept = get_ept(priv);
	int ret;

	msg->command = command;
	msg->result = -ENXIO;
	msg->cookie = 0;

	ret = rpmsg_send_nocopy(ept, msg, len);

	if (ret < 0) {
		rpmsg_release_tx_buffer(ept, msg);
	}

	return ret;
}

static int rpmsgfs_send_recv_ext(struct rpmsgfs_s *priv, uint32_t command, bool copy,
				 struct rpmsgfs_header_s *msg, int len, void *data,
				 struct rpmsgfs_cookie_s *cookie)
{
	struct rpmsg_endpoint *ept = get_ept(priv);
	int ret;

	ret = k_sem_init(&cookie->sem, 0, 1);
	if (ret < 0) {
		goto err_out_release_tx;
	}

	if (data) {
		cookie->data = data;
	} else if (copy) {
		cookie->data = msg;
	} else {
		cookie->data = NULL;
	}

	msg->command = command;
	msg->result = -ENXIO;
	msg->cookie = (uintptr_t)cookie;

	if (copy) {
		ret = rpmsg_send(ept, msg, len);
	} else {
		ret = rpmsg_send_nocopy(ept, msg, len);
	}

	if (ret < 0) {
		LOG_ERR("rpmsg_send failed: %d", ret);
		goto err_out_release_tx;
	}

	ret = k_sem_take(&cookie->sem, K_FOREVER);
	if (ret < 0) {
		goto err_out;
	}

	/* Adapt to Zephyr err result codes */
	if (cookie->result == -ENOTEMPTY_POSIX) {
		return -ENOTEMPTY;
	}

	return cookie->result;

err_out_release_tx:
	if (copy == false) {
		rpmsg_release_tx_buffer(ept, msg);
	}

err_out:
	return ret;
}

static int rpmsgfs_send_recv(struct rpmsgfs_s *priv, uint32_t command, bool copy,
			     struct rpmsgfs_header_s *msg, int len, void *data)
{
	struct rpmsgfs_cookie_s cookie;

	return rpmsgfs_send_recv_ext(priv, command, copy, msg, len, data, &cookie);
}

static ssize_t rpmsgfs_get_remote_path(const char *absolute_path, const struct fs_mount_t *mp,
				       char remote_path[MAX_PATH_LEN + 1])
{
	struct rpmsgfs_s *priv = mp->storage_dev;
	const char *relative_path = absolute_path + mp->mountp_len;
	size_t relative_path_size = strnlen(relative_path, MAX_PATH_LEN + 1);

	if (relative_path_size > MAX_PATH_LEN) {
		return -ENOMEM;
	}

	if (MAX_PATH_LEN - priv->remote_root_size < relative_path_size) {
		return -ENOMEM;
	}

	memcpy(remote_path, priv->remote_root, priv->remote_root_size);
	memcpy(remote_path + priv->remote_root_size, relative_path, relative_path_size + 1);
	return priv->remote_root_size + relative_path_size;
}

static int rpmsgfs_convert_to_rpmsgfs_flags(fs_mode_t zephyr_mode)
{
	int flags = 0;

	if (zephyr_mode & FS_O_READ) {
		flags |= RPMSGFS_O_RDONLY;
	}
	if (zephyr_mode & FS_O_WRITE) {
		flags |= RPMSGFS_O_WRONLY;
	}
	if (zephyr_mode & FS_O_CREATE) {
		flags |= RPMSGFS_O_CREAT;
	}
	if (zephyr_mode & FS_O_APPEND) {
		flags |= RPMSGFS_O_APPEND;
	}
	if (zephyr_mode & FS_O_TRUNC) {
		flags |= RPMSGFS_O_TRUNC;
	}

	return flags;
}

static int rpmsgfs_open(struct fs_file_t *zfp, const char *file_name, fs_mode_t mode)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_s *priv = zfp->mp->storage_dev;
	char path[MAX_PATH_LEN + 1];
	ssize_t path_size = rpmsgfs_get_remote_path(file_name, zfp->mp, path);
	int ret = 0;

	if (path_size < 0) {
		return path_size;
	}

	struct rpmsgfs_open_s *msg;
	size_t msg_size = sizeof(*msg) + path_size + 1;
	uint32_t space;

	priv->crefs++;

	msg = rpmsg_get_tx_payload_buffer(get_ept(priv), &space, true);
	if ((!msg) || (msg_size > space)) {
		ret = -ENOMEM;
		goto err_out_cref;
	}

	msg->flags = rpmsgfs_convert_to_rpmsgfs_flags(mode);
	msg->mode = RPMSGFS_DEFAULT_FILE_MODE;
	strncpy(msg->pathname, path, path_size);
	msg->pathname[path_size] = 0;

	int fd = rpmsgfs_send_recv(priv, RPMSGFS_OPEN, false, (struct rpmsgfs_header_s *)msg,
				   msg_size, NULL);

	if (fd < 0) {
		ret = fd;
		goto err_out_cref;
	}

	zfp->filep = (void *)fd;
	return 0;

err_out_cref:
	priv->crefs--;
	return ret;
}

static int rpmsgfs_close(struct fs_file_t *zfp)
{
	LOG_DBG("%s", __func__);

	int ret = 0;
	struct rpmsgfs_s *priv = zfp->mp->storage_dev;

	struct rpmsgfs_file_descriptor_s msg = {
		.fd = (int)zfp->filep,
	};

	ret = rpmsgfs_send_recv(priv, RPMSGFS_CLOSE, true, (struct rpmsgfs_header_s *)&msg,
				sizeof(msg), NULL);

	if (ret >= 0) {
		priv->crefs--;
	}

	return ret;
}

static int rpmsgfs_send_recv_path(struct rpmsgfs_s *priv, uint32_t command, const char *path,
				  const size_t path_size)
{
	struct rpmsgfs_pathname_s *msg;
	size_t msg_size = sizeof(*msg) + path_size + 1;
	uint32_t space;

	msg = rpmsg_get_tx_payload_buffer(get_ept(priv), &space, true);
	if ((!msg) || (msg_size > space)) {
		return -ENOMEM;
	}

	strncpy(msg->pathname, path, path_size);
	msg->pathname[path_size] = 0;

	return rpmsgfs_send_recv(priv, command, false, (struct rpmsgfs_header_s *)msg, msg_size,
				 NULL);
}

static int rpmsgfs_send_recv_absolute_path(const struct fs_mount_t *mountp, uint32_t command,
					   const char *absolute_path)
{
	char path[MAX_PATH_LEN + 1];
	ssize_t path_size = rpmsgfs_get_remote_path(absolute_path, mountp, path);

	if (path_size < 0) {
		return path_size;
	}
	return rpmsgfs_send_recv_path(mountp->storage_dev, command, path, path_size);
}

static int rpmsgfs_unlink(struct fs_mount_t *mountp, const char *absolute_path)
{
	LOG_DBG("%s", __func__);

	int ret = 0;

	ret = rpmsgfs_send_recv_absolute_path(mountp, RPMSGFS_UNLINK, absolute_path);
	if (ret == -EISDIR) {
		ret = rpmsgfs_send_recv_absolute_path(mountp, RPMSGFS_RMDIR, absolute_path);
	}

	return ret;
}

static int rpmsgfs_rename(struct fs_mount_t *mountp, const char *from, const char *to)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_s *priv = mountp->storage_dev;
	char from_path[MAX_PATH_LEN + 1];
	char to_path[MAX_PATH_LEN + 1];

	ssize_t from_path_size = rpmsgfs_get_remote_path(from, mountp, from_path);

	if (from_path_size < 0) {
		LOG_DBG("%s error on getting 'from' remote path: %s", __func__, from);
		return from_path_size;
	}
	from_path_size++;

	ssize_t to_path_size = rpmsgfs_get_remote_path(to, mountp, to_path);

	if (to_path_size < 0) {
		LOG_DBG("%s error on getting 'to' remote path: %s", __func__, to);
		return to_path_size;
	}
	to_path_size++;

	/* to_path is aligned to the next 8 bytes boundary */
	size_t to_path_offset = ROUND_UP(from_path_size, 8);
	struct rpmsgfs_pathname_s *msg;
	size_t msg_size = sizeof(*msg) + to_path_offset + to_path_size;
	uint32_t space;

	msg = rpmsg_get_tx_payload_buffer(get_ept(priv), &space, true);
	if ((!msg) || (msg_size > space)) {
		return -ENOMEM;
	}

	memcpy(msg->pathname, from_path, from_path_size);
	memcpy(msg->pathname + to_path_offset, to_path, to_path_size);

	return rpmsgfs_send_recv(priv, RPMSGFS_RENAME, false, (struct rpmsgfs_header_s *)msg,
				 msg_size, NULL);
}

static ssize_t rpmsgfs_read(struct fs_file_t *zfp, void *ptr, size_t size)
{
	LOG_DBG("%s", __func__);
	int ret = 0;

	struct rpmsgfs_s *priv = zfp->mp->storage_dev;
	struct rpmsgfs_cookie_s cookie;

	struct iovec read = {
		.iov_base = ptr,
		.iov_len = 0,
	};

	struct rpmsgfs_file_content_s msg = {
		.fd = (int32_t)zfp->filep,
		.count = size,
	};

	ret = rpmsgfs_send_recv_ext(priv, RPMSGFS_READ, true, (struct rpmsgfs_header_s *)&msg,
				    sizeof(msg), (void *)&read, &cookie);

	if (ret < 0) {

		/* Adapt to Zephyr err result codes */
		if (ret == -EBADF) {
			ret = -EACCES;
		}

		return ret;
	}

	return read.iov_len > 0 ? read.iov_len : cookie.result;
}

static ssize_t rpmsgfs_write(struct fs_file_t *zfp, const void *ptr, size_t size)
{
	LOG_DBG("%s", __func__);
	struct rpmsgfs_s *priv = zfp->mp->storage_dev;
	int ret = 0;
	size_t written = 0;

	while (written < size) {
		uint32_t space;
		struct rpmsgfs_file_content_s *msg;
		bool last = false;

		msg = rpmsg_get_tx_payload_buffer(get_ept(priv), &space, true);
		if (!msg) {
			return -ENOMEM;
		}

		space -= sizeof(*msg);
		if (space >= size - written) {
			space = size - written;
			last = true;
		}

		msg->fd = (int32_t)zfp->filep;
		msg->count = space;
		memcpy(msg->buf, (const uint8_t *)ptr + written, space);

		if (last) {
			ret = rpmsgfs_send_recv(priv, RPMSGFS_WRITE, false,
						(struct rpmsgfs_header_s *)msg,
						sizeof(*msg) + space, NULL);
		} else {
			ret = rpmsgfs_send_nocopy(priv, RPMSGFS_WRITE,
						  (struct rpmsgfs_header_s *)msg,
						  sizeof(*msg) + space);
		}

		if (ret < 0) {
			break;
		}

		written += space;
	}

	/* Adapt to Zephyr err result codes */
	if (ret == -EBADF) {
		ret = -EACCES;
	}

	return ret < 0 ? ret : size;
}

static int rpmsgfs_seek(struct fs_file_t *zfp, off_t offset, int whence)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_lseek_s msg = {
		.fd = (int32_t)zfp->filep,
		.offset = offset,
		.whence = whence,
	};

	int ret = rpmsgfs_send_recv(zfp->mp->storage_dev, RPMSGFS_LSEEK, true,
				    (struct rpmsgfs_header_s *)&msg, sizeof(msg), NULL);

	return ret < 0 ? ret : 0;
}

static off_t rpmsgfs_tell(struct fs_file_t *zfp)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_lseek_s msg = {
		.fd = (int32_t)zfp->filep,
		.offset = 0,
		.whence = FS_SEEK_CUR,
	};

	return rpmsgfs_send_recv(zfp->mp->storage_dev, RPMSGFS_LSEEK, true,
				 (struct rpmsgfs_header_s *)&msg, sizeof(msg), NULL);
}

static int rpmsgfs_truncate(struct fs_file_t *zfp, off_t length)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_ftruncate_s msg = {
		.fd = (int32_t)zfp->filep,
		.length = length,
	};

	return rpmsgfs_send_recv(zfp->mp->storage_dev, RPMSGFS_FTRUNCATE, true,
				 (struct rpmsgfs_header_s *)&msg, sizeof(msg), NULL);
}

static int rpmsgfs_sync(struct fs_file_t *zfp)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_s *priv = zfp->mp->storage_dev;

	struct rpmsgfs_file_descriptor_s msg = {
		.fd = (int)zfp->filep,
	};

	return rpmsgfs_send_recv(priv, RPMSGFS_SYNC, true, (struct rpmsgfs_header_s *)&msg,
				 sizeof(msg), NULL);
}

static int rpmsgfs_mkdir(struct fs_mount_t *mountp, const char *absolute_path)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_s *priv = mountp->storage_dev;
	struct rpmsgfs_mkdir_s *msg;
	uint32_t space;
	char path[MAX_PATH_LEN + 1];
	ssize_t path_size = rpmsgfs_get_remote_path(absolute_path, mountp, path);

	if (path_size < 0) {
		return path_size;
	}
	size_t msg_size = sizeof(*msg) + path_size + 1;

	msg = rpmsg_get_tx_payload_buffer(get_ept(priv), &space, true);
	if ((!msg) || (msg_size > space)) {
		return -ENOMEM;
	}

	strncpy(msg->pathname, path, path_size);
	msg->pathname[path_size] = 0;
	msg->mode = RPMSGFS_DEFAULT_DIR_MODE;

	return rpmsgfs_send_recv(priv, RPMSGFS_MKDIR, false, (struct rpmsgfs_header_s *)msg,
				 msg_size, NULL);
}

static int rpmsgfs_opendir(struct fs_dir_t *zdp, const char *absolute_path)
{
	LOG_DBG("%s", __func__);

	int ret = 0;
	struct rpmsgfs_s *priv = zdp->mp->storage_dev;

	priv->crefs++;

	ret = rpmsgfs_send_recv_absolute_path(zdp->mp, RPMSGFS_OPENDIR, absolute_path);
	if (ret < 0) {
		goto err_out_cref;
	}

	zdp->dirp = (void *)ret;
	return 0;

err_out_cref:
	priv->crefs--;
	return ret;
}

static int rpmsgfs_readdir(struct fs_dir_t *zdp, struct fs_dirent *entry)
{
	LOG_DBG("%s", __func__);
	int ret;

	struct rpmsgfs_s *priv = zdp->mp->storage_dev;

	int dir = (int)zdp->dirp;

	struct rpmsgfs_readdir_s msg = {
		.fd = dir,
	};

	ret = rpmsgfs_send_recv(priv, RPMSGFS_READDIR, true, (struct rpmsgfs_header_s *)&msg,
				sizeof(msg), entry);

	if (ret == -ENOENT) {
		entry->name[0] = 0;
		return 0;
	}

	return ret;
}

static int rpmsgfs_closedir(struct fs_dir_t *zdp)
{
	LOG_DBG("%s", __func__);

	int ret;

	struct rpmsgfs_s *priv = zdp->mp->storage_dev;

	int dir = (int)zdp->dirp;

	struct rpmsgfs_file_descriptor_s msg = {
		.fd = dir,
	};

	ret = rpmsgfs_send_recv(priv, RPMSGFS_CLOSEDIR, true, (struct rpmsgfs_header_s *)&msg,
				sizeof(msg), NULL);

	priv->crefs--;

	return ret;
}

static int rpmsgfs_stat(struct fs_mount_t *mountp, const char *absolute_path,
			struct fs_dirent *entry)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_s *priv = mountp->storage_dev;
	struct rpmsgfs_fstat_s *msg;
	uint32_t space;
	size_t msg_size;
	char path[MAX_PATH_LEN + 1];
	ssize_t path_size = rpmsgfs_get_remote_path(absolute_path, mountp, path);

	if (path_size < 0) {
		return path_size;
	}
	msg_size = sizeof(*msg) + path_size + 1;

	msg = rpmsg_get_tx_payload_buffer(get_ept(priv), &space, true);
	if ((!msg) || (msg_size > space)) {
		return -ENOMEM;
	}

	strncpy(msg->pathname, path, path_size);
	msg->pathname[path_size] = 0;

	return rpmsgfs_send_recv(priv, RPMSGFS_STAT, false, (struct rpmsgfs_header_s *)msg,
				 msg_size, entry);
}

static int rpmsgfs_statvfs(struct fs_mount_t *mountp, const char *absolute_path,
			   struct fs_statvfs *stat)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_s *priv = mountp->storage_dev;
	struct rpmsgfs_statfs_s *msg;
	uint32_t space;
	size_t msg_size;
	char path[MAX_PATH_LEN + 1];
	ssize_t path_size = rpmsgfs_get_remote_path(absolute_path, mountp, path);

	if (path_size < 0) {
		return path_size;
	}
	msg_size = sizeof(*msg) + path_size + 1;

	msg = rpmsg_get_tx_payload_buffer(get_ept(priv), &space, true);
	if ((!msg) || (msg_size > space)) {
		return -ENOMEM;
	}

	strncpy(msg->pathname, path, path_size);
	msg->pathname[path_size] = 0;

	return rpmsgfs_send_recv(priv, RPMSGFS_STATFS, false, (struct rpmsgfs_header_s *)msg,
				 msg_size, stat);
}

/*
 * Callback handlers
 *
 */

static int rpmsgfs_default_handler(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
				   void *priv)
{
	struct rpmsgfs_header_s *header = data;
	struct rpmsgfs_cookie_s *cookie = (struct rpmsgfs_cookie_s *)(uintptr_t)header->cookie;

	cookie->result = header->result;
	if (cookie->result >= 0 && cookie->data) {
		memcpy(cookie->data, data, len);
	}

	k_sem_give(&cookie->sem);

	return 0;
}

static int rpmsgfs_readdir_handler(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
				   void *priv)
{
	struct rpmsgfs_header_s *header = data;
	struct rpmsgfs_cookie_s *cookie = (struct rpmsgfs_cookie_s *)(uintptr_t)header->cookie;
	struct rpmsgfs_readdir_s *rsp = data;
	struct fs_dirent *entry = cookie->data;

	cookie->result = header->result;
	if (cookie->result >= 0) {
		strncpy(entry->name, rsp->name, sizeof(entry->name) - 1);
		entry->name[sizeof(entry->name) - 1] = 0;
		entry->type = rsp->type == RPMSGFS_DT_DIR ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE;
		entry->size = 0; /* cannot fill in size, sorry.. */
	}

	k_sem_give(&cookie->sem);

	return 0;
}

static int rpmsgfs_stat_handler(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
				void *priv)
{
	struct rpmsgfs_header_s *header = data;
	struct rpmsgfs_cookie_s *cookie = (struct rpmsgfs_cookie_s *)(uintptr_t)header->cookie;
	struct rpmsgfs_fstat_s *rsp = data;
	struct fs_dirent *entry = cookie->data;

	cookie->result = header->result;
	if (cookie->result >= 0) {
		entry->size = rsp->buf.size;
		entry->type =
			rsp->buf.mode & RPMSGFS_S_IFDIR ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE;
		strncpy(entry->name, rsp->pathname, sizeof(entry->name) - 1);
		entry->name[sizeof(entry->name) - 1] = 0;
	}

	k_sem_give(&cookie->sem);

	return 0;
}

static int rpmsgfs_statfs_handler(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
				  void *priv)
{
	struct rpmsgfs_header_s *header = data;
	struct rpmsgfs_cookie_s *cookie = (struct rpmsgfs_cookie_s *)(uintptr_t)header->cookie;
	struct rpmsgfs_statfs_s *rsp = data;
	struct fs_statvfs *stat = cookie->data;

	cookie->result = header->result;
	if (cookie->result >= 0) {
		stat->f_bsize = rsp->bsize;
		stat->f_frsize = rsp->bsize;
		stat->f_blocks = rsp->blocks;
		stat->f_bfree = rsp->bfree;
	}

	k_sem_give(&cookie->sem);

	return 0;
}

static int rpmsgfs_read_handler(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
				void *priv)
{
	struct rpmsgfs_header_s *header = data;
	struct rpmsgfs_cookie_s *cookie = (struct rpmsgfs_cookie_s *)(uintptr_t)header->cookie;
	struct rpmsgfs_file_content_s *rsp = data;
	struct iovec *read = cookie->data;

	cookie->result = header->result;
	if (cookie->result > 0) {
		memcpy((uint8_t *)read->iov_base + read->iov_len, rsp->buf, cookie->result);
		read->iov_len += cookie->result;
	}

	if (cookie->result <= 0 || read->iov_len >= rsp->count) {
		k_sem_give(&cookie->sem);
	}

	return 0;
}

static int rpmsgfs_unused_handler(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
				  void *priv)
{
	return 0;
}

static const rpmsg_ept_cb g_rpmsgfs_handler[] = {
	[RPMSGFS_INIT] = rpmsgfs_unused_handler, /* Not required */
	[RPMSGFS_OPEN] = rpmsgfs_default_handler,
	[RPMSGFS_CLOSE] = rpmsgfs_default_handler,
	[RPMSGFS_READ] = rpmsgfs_read_handler,
	[RPMSGFS_WRITE] = rpmsgfs_default_handler,
	[RPMSGFS_LSEEK] = rpmsgfs_default_handler,
	[RPMSGFS_IOCTL] = rpmsgfs_unused_handler, /* Not supported in Zephyr */
	[RPMSGFS_SYNC] = rpmsgfs_default_handler,
	[RPMSGFS_DUP] = rpmsgfs_unused_handler,   /* Not supported in Zephyr */
	[RPMSGFS_FSTAT] = rpmsgfs_unused_handler, /* Not supported in Zephyr */
	[RPMSGFS_FTRUNCATE] = rpmsgfs_default_handler,
	[RPMSGFS_OPENDIR] = rpmsgfs_default_handler,
	[RPMSGFS_READDIR] = rpmsgfs_readdir_handler,
	[RPMSGFS_REWINDDIR] = rpmsgfs_unused_handler, /* Not supported in Zephyr */
	[RPMSGFS_CLOSEDIR] = rpmsgfs_default_handler,
	[RPMSGFS_STATFS] = rpmsgfs_statfs_handler,
	[RPMSGFS_UNLINK] = rpmsgfs_default_handler,
	[RPMSGFS_MKDIR] = rpmsgfs_default_handler,
	[RPMSGFS_RMDIR] = rpmsgfs_default_handler,
	[RPMSGFS_RENAME] = rpmsgfs_default_handler,
	[RPMSGFS_STAT] = rpmsgfs_stat_handler,
	[RPMSGFS_FCHSTAT] = rpmsgfs_unused_handler, /* Not supported in Zephyr */
	[RPMSGFS_CHSTAT] = rpmsgfs_unused_handler,  /* Not supported in Zephyr */
};

static int rpmsgfs_callback(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
			    void *priv)
{
	struct rpmsgfs_header_s *header = data;
	uint32_t command = header->command;

	LOG_DBG("%s: command: %u", __func__, command);

	if (command < ARRAY_SIZE(g_rpmsgfs_handler)) {
		return g_rpmsgfs_handler[command](ept, data, len, src, priv);
	}

	return RPMSG_SUCCESS;
}

static int connect_to_endpoint(struct rpmsgfs_s *priv)
{
	struct rpmsg_endpoint *ept = get_ept(priv);
	char buf[RPMSGFS_SERVICE_NAME_MAX_SIZE];

	snprintf(buf, sizeof(buf), "%s%p", RPMSGFS_SERVICE_NAME_PREFIX, priv);
	int ret = rpmsg_create_ept(ept, g_rpmsg_dev, buf, RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
				   rpmsgfs_callback, NULL);

	if (ret >= 0) {
		/* wait for an updated dest_addr by the announcement msg or RPMSGFS_INIT cmd */
		while (ept->dest_addr == RPMSG_ADDR_ANY) {
			k_sleep(K_USEC(10));
		}
	}

	return ret;
}

static int rpmsgfs_mount(struct fs_mount_t *mountp)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_s *priv;
	int ret = 0;

	if (k_mem_slab_alloc(&rpmsgfs_slab, (void **)&priv, K_NO_WAIT) < 0) {
		return -ENOMEM;
	}

	priv->remote_root_size = strnlen(mountp->fs_data, MAX_PATH_LEN + 1);
	if (priv->remote_root_size > MAX_PATH_LEN) {
		ret = -ENOMEM;
		goto _error;
	}

	strncpy(priv->remote_root, mountp->fs_data, MAX_PATH_LEN);
	priv->remote_root[MAX_PATH_LEN] = 0;

	/* remote_root: first char must be '/' */
	if (priv->remote_root_size && priv->remote_root[0] != '/') {
		ret = -EINVAL;
		goto _error;
	}

	/* remote_root: remove slash at the end */
	if ((priv->remote_root_size > 1) &&
	    (priv->remote_root[priv->remote_root_size - 1] == '/')) {
		priv->remote_root[priv->remote_root_size - 1] = 0;
		priv->remote_root_size--;
	}

	priv->crefs = 0;

#ifdef CONFIG_FILE_SYSTEM_RPMSGFS_SINGLE_CHANNEL
	if (!ept_connected) {
		ret = connect_to_endpoint(priv);
		ept_connected = true;
	}
#else
	ret = connect_to_endpoint(priv);
#endif

	if (ret < 0) {
		goto _error;
	}

	LOG_INF("%s is mapped to %s on rpmsg ept: 0x%04x", mountp->mnt_point, priv->remote_root,
		get_ept(priv)->dest_addr);

	mountp->flags |= FS_MOUNT_FLAG_USE_DISK_ACCESS;
	mountp->storage_dev = priv;
	return 0;

_error:
	k_mem_slab_free(&rpmsgfs_slab, priv);
	return ret;
}

static int rpmsgfs_unmount(struct fs_mount_t *mountp)
{
	LOG_DBG("%s", __func__);

	struct rpmsgfs_s *priv = mountp->storage_dev;

	/* Fail if there are open files or dirs */
	if (priv->crefs) {
		return -EBUSY;
	}

#ifndef CONFIG_FILE_SYSTEM_RPMSGFS_SINGLE_CHANNEL
	rpmsg_destroy_ept(&priv->ept);
#endif
	k_mem_slab_free(&rpmsgfs_slab, priv);
	priv = NULL;

	return 0;
}

/* File system interface */
static const struct fs_file_system_t rpmsgfs_fs = {
	.open = rpmsgfs_open,
	.close = rpmsgfs_close,
	.read = rpmsgfs_read,
	.write = rpmsgfs_write,
	.lseek = rpmsgfs_seek,
	.tell = rpmsgfs_tell,
	.truncate = rpmsgfs_truncate,
	.sync = rpmsgfs_sync,
	.opendir = rpmsgfs_opendir,
	.readdir = rpmsgfs_readdir,
	.closedir = rpmsgfs_closedir,
	.mount = rpmsgfs_mount,
	.unmount = rpmsgfs_unmount,
	.unlink = rpmsgfs_unlink,
	.rename = rpmsgfs_rename,
	.mkdir = rpmsgfs_mkdir,
	.stat = rpmsgfs_stat,
	.statvfs = rpmsgfs_statvfs,
};

static int rpmsgfs_init(void)
{
	return fs_register(FS_RPMSGFS, &rpmsgfs_fs);
}

void rpmsgfs_init_rpmsg(struct rpmsg_device *rpmsg_dev)
{
	g_rpmsg_dev = rpmsg_dev;
}

SYS_INIT(rpmsgfs_init, POST_KERNEL, CONFIG_FILE_SYSTEM_INIT_PRIORITY);
