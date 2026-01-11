/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>
#include "virtiofs.h"
#include <zephyr/drivers/virtio.h>

LOG_MODULE_REGISTER(virtiofs, CONFIG_VIRTIOFS_LOG_LEVEL);

/*
 * According to 5.11.2 of virtio specification v1.3 the virtiofs queues are indexed as
 * follows:
 * - idx 0 - hiprio
 * - idx 1 - notification queue
 * - idx 2..n - request queues
 * notification queue is available only if VIRTIO_FS_F_NOTIFICATION is present and
 * there is no mention that in its absence the request queues will be shifted and start
 * at idx 1, so the request queues shall start at idx 2. However in case of qemu+virtiofsd
 * who don't support VIRTIO_FS_F_NOTIFICATION, the last available queue is at idx 1 and
 * virtio_fs_config.num_request_queues states that there is a single request queue present
 * which must be the one at idx 1
 */
#ifdef CONFIG_VIRTIOFS_NO_NOTIFICATION_QUEUE_SLOT
#define REQUEST_QUEUE 1
#else
#define REQUEST_QUEUE 2
#endif

/*
 * Currently we are using only one request queue, so we don't have to initialize queues
 * after that one
 */
#define QUEUE_COUNT (REQUEST_QUEUE + 1)


struct virtio_fs_config {
	char tag[36];
	uint32_t num_request_queues;
};

static int virtiofs_validate_response(
	const struct fuse_out_header *header, uint32_t opcode, uint32_t used_len,
	uint32_t expected_len)
{
	if (used_len < sizeof(*header)) {
		LOG_ERR("used length is smaller than size of fuse_out_header");
		return -EIO;
	}

	if (header->error != 0) {
		LOG_ERR(
			"%s error %d (%s)",
			fuse_opcode_to_string(opcode),
			-header->error,
			strerror(-header->error)
		);
		return header->error;
	}

	if (expected_len != -1 && header->len != expected_len) {
		LOG_ERR(
			"%s return message has invalid length (0x%x), expected 0x%x",
			fuse_opcode_to_string(opcode),
			header->len,
			expected_len
		);
		return -EIO;
	}

	return 0;
}

struct recv_cb_param {
	struct k_sem sem;
	uint32_t used_len;
};

void virtiofs_recv_cb(void *opaque, uint32_t used_len)
{
	struct recv_cb_param *arg = opaque;

	arg->used_len = used_len;
	k_sem_give(&arg->sem);
}

static uint32_t virtiofs_send_receive(
	const struct device *dev, uint16_t virtq, struct virtq_buf *bufs,
	uint16_t bufs_size, uint16_t device_readable)
{
	struct virtq *virtqueue = virtio_get_virtqueue(dev, virtq);
	struct recv_cb_param cb_arg;

	k_sem_init(&cb_arg.sem, 0, 1);

	virtq_add_buffer_chain(
		virtqueue, bufs, bufs_size, device_readable, virtiofs_recv_cb, &cb_arg,
		K_FOREVER
	);
	virtio_notify_virtqueue(dev, virtq);

	k_sem_take(&cb_arg.sem, K_FOREVER);

	return cb_arg.used_len;
}

static uint16_t virtiofs_queue_enum_cb(uint16_t queue_idx, uint16_t max_size, void *unused)
{
	if (queue_idx == REQUEST_QUEUE) {
		return MIN(CONFIG_VIRTIOFS_MAX_VQUEUE_SIZE, max_size);
	} else {
		return 0;
	}
}

int virtiofs_init(const struct device *dev, struct fuse_init_out *response)
{
	struct virtio_fs_config *fs_config = virtio_get_device_specific_config(dev);
	struct fuse_init_req req;
	int ret = 0;

	if (!fs_config) {
		LOG_ERR("no virtio_fs_config present");
		return -ENXIO;
	}
	if (fs_config->num_request_queues < 1) {
		/* this shouldn't ever happen */
		LOG_ERR("no request queue present");
		return -ENODEV;
	}

	ret = virtio_commit_feature_bits(dev);
	if (ret != 0) {
		return ret;
	}

	ret = virtio_init_virtqueues(dev, QUEUE_COUNT, virtiofs_queue_enum_cb, NULL);
	if (ret != 0) {
		LOG_ERR("failed to initialize fs virtqueues");
		return ret;
	}

	virtio_finalize_init(dev);

	fuse_create_init_req(&req);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = sizeof(req.in_header) + sizeof(req.init_in) },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) + sizeof(req.init_out) }
	};

	LOG_INF("sending FUSE_INIT, unique=%" PRIu64, req.in_header.unique);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 2, 1);

	LOG_INF("received FUSE_INIT response, unique=%" PRIu64, req.out_header.unique);

	int valid_ret = virtiofs_validate_response(
		&req.out_header, FUSE_INIT, used_len, buf[1].len
	);

	if (valid_ret != 0) {
		return valid_ret;
	}

	if (req.init_out.major != FUSE_MAJOR_VERSION) {
		LOG_ERR(
			"FUSE_INIT major version mismatch (%d), version %d is supported",
			req.init_out.major,
			FUSE_MAJOR_VERSION
		);
		return -ENOTSUP;
	}

	if (req.init_out.minor < FUSE_MINOR_VERSION) {
		LOG_ERR(
			"FUSE_INIT minor version is too low (%d), version %d is supported",
			req.init_out.minor,
			FUSE_MINOR_VERSION
		);
		return -ENOTSUP;
	}

	*response = req.init_out;

#ifdef CONFIG_VIRTIOFS_DEBUG
	fuse_dump_init_req_out(&req.init_out);
#endif

	return 0;
}

/**
 * @brief lookups object in the virtiofs filesystem
 *
 * @param dev virtio device its used on
 * @param inode inode to start from
 * @param path path to object we are looking for
 * @param response virtiofs response for object
 * @param parent_inode will be set to immediate parent inode of object that we are looking for.
 * If immediate parent doesn't exist it will be set to 0. If not 0 it has to be FUSE_FORGET by
 * caller. Can be NULL.
 * @return 0 or error code on failure
 */
int virtiofs_lookup(
	const struct device *dev, uint64_t inode, const char *path, struct fuse_entry_out *response,
	uint64_t *parent_inode)
{
	uint32_t path_len = strlen(path) + 1;
	const char *curr = path;
	uint32_t curr_len = 0;
	uint64_t curr_inode = inode;
	struct fuse_lookup_req req;

	/*
	 * we have to split path and lookup it dir by dir, because FUSE_LOOKUP doesn't work with
	 * full paths like abc/xyz/file. We have to lookup abc, then lookup xyz with abc's inode
	 * as a base and then lookup file with xyz's inode as a base
	 */
	while (curr < path + path_len) {
		curr_len = 0;
		for (const char *c = curr; c < path + path_len - 1 && *c != '/'; c++) {
			curr_len++;
		}

		fuse_create_lookup_req(&req, curr_inode, curr_len + 1);

		struct virtq_buf buf[] = {
			{ .addr = &req.in_header, .len = sizeof(struct fuse_in_header) },
			{ .addr = (void *)curr, .len = curr_len },
			/*
			 * despite length being part of in_header this still has to be null
			 * terminated
			 */
			{ .addr = "", .len = 1},
			{ .addr = &req.out_header,
			  .len = sizeof(struct fuse_out_header) + sizeof(struct fuse_entry_out) }
		};

		LOG_INF(
			"sending FUSE_LOOKUP for \"%s\", nodeid=%" PRIu64 ", unique=%" PRIu64,
			curr, curr_inode, req.in_header.unique
		);
		uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 4, 3);

		LOG_INF("received FUSE_LOOKUP response, unique=%" PRIu64, req.out_header.unique);

		int valid_ret = virtiofs_validate_response(
			&req.out_header, FUSE_LOOKUP, used_len,
			sizeof(struct fuse_out_header) + sizeof(struct fuse_entry_out)
		);

		if (parent_inode) {
			*parent_inode = curr_inode;
		}

		*response = req.entry_out;
		if (valid_ret != 0) {
			if (parent_inode && (curr + curr_len + 1 != path + path_len)) {
				/* there is no immediate parent */
				if (*parent_inode != inode) {
					virtiofs_forget(dev, *parent_inode, 1);
				}
				*parent_inode = 0;
			}
			return valid_ret;
		}

#ifdef CONFIG_VIRTIOFS_DEBUG
		fuse_dump_entry_out(&req.entry_out);
#endif
		bool is_curr_parent = true;

		for (const char *c = curr; c < path + path_len; c++) {
			if (*c == '/') {
				is_curr_parent = false;
			}
		}

		/*
		 * unless its inode param passed to this function or a parent of object we
		 * are looking for, curr_inode won't be used anymore so we can forget it
		 */
		if (curr_inode != inode && (!parent_inode || !is_curr_parent)) {
			virtiofs_forget(dev, curr_inode, 1);
		}

		curr_inode = req.entry_out.nodeid;
		curr += curr_len + 1;
	}

	return 0;
}

int virtiofs_open(
	const struct device *dev, uint64_t inode, uint32_t flags, struct fuse_open_out *response,
	enum fuse_object_type type)
{
	struct fuse_open_req req;

	fuse_create_open_req(&req, inode, flags, type);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = req.in_header.len },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) + sizeof(req.open_out) }
	};

	LOG_INF(
		"sending %s, nodeid=%" PRIu64 ", flags=0%" PRIo32 ", unique=%" PRIu64,
		type == FUSE_DIR ? "FUSE_OPENDIR" : "FUSE_OPEN", inode, flags, req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 2, 1);

	LOG_INF(
		"received %s response, unique=%" PRIu64,
		type == FUSE_DIR ? "FUSE_OPENDIR" : "FUSE_OPEN", req.out_header.unique
	);

	int valid_ret = virtiofs_validate_response(
		&req.out_header, type == FUSE_DIR ? FUSE_OPENDIR : FUSE_OPEN, used_len, buf[1].len
	);

	if (valid_ret != 0) {
		return valid_ret;
	}

	*response = req.open_out;

#ifdef CONFIG_VIRTIOFS_DEBUG
	fuse_dump_open_req_out(&req.open_out);
#endif

	return 0;
}

int virtiofs_read(
	const struct device *dev, uint64_t inode, uint64_t fh,
	uint64_t offset, uint32_t size, uint8_t *readbuf)
{
	struct fuse_read_req req;

	fuse_create_read_req(&req, inode, fh, offset, size, FUSE_FILE);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = req.in_header.len },
		{ .addr = &req.out_header, .len = sizeof(struct fuse_out_header) },
		{ .addr = readbuf, .len = size }
	};

	LOG_INF(
		"sending FUSE_READ, nodeid=%" PRIu64 ", fh=%" PRIu64 ", offset=%" PRIu64
		", size=%" PRIu32 ", unique=%" PRIu64,
		inode, fh, offset, size, req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 3, 1);

	LOG_INF("received FUSE_READ response, unique=%" PRIu64, req.out_header.unique);

	int valid_ret = virtiofs_validate_response(&req.out_header, FUSE_READ, used_len, -1);

	if (valid_ret != 0) {
		return valid_ret;
	}

	return req.out_header.len - sizeof(req.out_header);
}

int virtiofs_release(const struct device *dev, uint64_t inode, uint64_t fh,
	enum fuse_object_type type)
{
	struct fuse_release_req req;

	fuse_create_release_req(&req, inode, fh, type);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = req.in_header.len },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) }
	};

	LOG_INF(
		"sending %s, inode=%" PRIu64 ", fh=%" PRIu64 ", unique=%" PRIu64,
		type == FUSE_DIR ? "FUSE_RELEASEDIR" : "FUSE_RELEASE", inode, fh,
		req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 2, 1);

	LOG_INF(
		"received %s response, unique=%" PRIu64,
		type == FUSE_DIR ? "FUSE_RELEASEDIR" : "FUSE_RELEASE", req.out_header.unique
	);

	return virtiofs_validate_response(
		&req.out_header, type == FUSE_DIR ? FUSE_RELEASEDIR : FUSE_RELEASE, used_len, -1
	);
}

int virtiofs_destroy(const struct device *dev)
{
	struct fuse_destroy_req req;

	fuse_create_destroy_req(&req);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = sizeof(req.in_header) },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) }
	};

	LOG_INF("sending FUSE_DESTROY, unique=%" PRIu64, req.in_header.unique);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 2, 1);

	LOG_INF("received FUSE_DESTROY response, unique=%" PRIu64, req.in_header.unique);

	return virtiofs_validate_response(&req.out_header, FUSE_DESTROY, used_len, -1);
}

int virtiofs_create(
	const struct device *dev, uint64_t inode, const char *fname, uint32_t flags,
	uint32_t mode, struct fuse_create_out *response)
{
	uint32_t fname_len = strlen(fname) + 1;
	struct fuse_create_req req;

	fuse_create_create_req(&req, inode, fname_len, flags, mode);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = sizeof(req.in_header) + sizeof(req.create_in) },
		{ .addr = (void *)fname, .len = fname_len },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) + sizeof(req.create_out) }
	};

	LOG_INF(
		"sending FUSE_CREATE for \"%s\", nodeid=%" PRIu64 ", flags=0%" PRIo32
		", unique=%" PRIu64,
		fname, inode, flags, req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 3, 2);

	LOG_INF("received FUSE_CREATE response, unique=%" PRIu64, req.out_header.unique);

	int valid_ret = virtiofs_validate_response(
		&req.out_header, FUSE_CREATE, used_len, buf[2].len
	);

	if (valid_ret != 0) {
		return valid_ret;
	}

	*response = req.create_out;

#ifdef CONFIG_VIRTIOFS_DEBUG
	fuse_dump_create_req_out(&req.create_out);
#endif

	return 0;
}

int virtiofs_write(
	const struct device *dev,  uint64_t inode, uint64_t fh, uint64_t offset,
	uint32_t size, const uint8_t *write_buf)
{
	struct fuse_write_req req;

	fuse_create_write_req(&req, inode, fh, offset, size);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = sizeof(req.in_header) + sizeof(req.write_in) },
		{ .addr = (void *)write_buf, .len = size },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) + sizeof(req.write_out) }
	};

	LOG_INF(
		"sending FUSE_WRITE, nodeid=%" PRIu64", fh=%" PRIu64 ", offset=%" PRIu64
		", size=%" PRIu32 ", unique=%" PRIu64,
		inode, fh, offset, size, req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 3, 2);

	LOG_INF("received FUSE_WRITE response, unique=%" PRIu64, req.out_header.unique);

	int valid_ret = virtiofs_validate_response(
		&req.out_header, FUSE_WRITE, used_len, buf[2].len
	);

	if (valid_ret != 0) {
		return valid_ret;
	}

#ifdef CONFIG_VIRTIOFS_DEBUG
	fuse_dump_write_out(&req.write_out);
#endif

	return req.write_out.size;
}

int virtiofs_lseek(
	const struct device *dev,  uint64_t inode, uint64_t fh, uint64_t offset,
	uint32_t whence, struct fuse_lseek_out *response)
{
	struct fuse_lseek_req req;

	fuse_create_lseek_req(&req, inode, fh, offset, whence);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = req.in_header.len },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) + sizeof(req.lseek_out) }
	};

	LOG_INF(
		"sending FUSE_LSEEK, nodeid=%" PRIu64 ", fh=%" PRIu64 ", offset=%" PRIu64
		", whence=%" PRIu32 ", unique=%" PRIu64,
		inode, fh, offset, whence, req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 2, 1);

	LOG_INF("received FUSE_LSEEK response, unique=%" PRIu64, req.out_header.unique);

	int valid_ret = virtiofs_validate_response(
		&req.out_header, FUSE_LSEEK, used_len, buf[1].len
	);

	if (valid_ret != 0) {
		return valid_ret;
	}

	*response = req.lseek_out;

#ifdef CONFIG_VIRTIOFS_DEBUG
	fuse_dump_lseek_out(&req.lseek_out);
#endif

	return 0;
}

int virtiofs_setattr(
	const struct device *dev, uint64_t inode, struct fuse_setattr_in *in,
	struct fuse_attr_out *response)
{
	struct fuse_setattr_req req;

	fuse_create_setattr_req(&req, inode);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = sizeof(req.in_header) },
		{ .addr = in, .len = sizeof(*in) },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) },
		{ .addr = response, .len = sizeof(*response) },
	};

	LOG_INF("sending FUSE_SETATTR, unique=%" PRIu64, req.in_header.unique);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 4, 2);

	LOG_INF("received FUSE_SETATTR response, unique=%" PRIu64, req.out_header.unique);

	int valid_ret = virtiofs_validate_response(
		&req.out_header, FUSE_SETATTR, used_len, sizeof(req.out_header) + sizeof(*response)
	);

	if (valid_ret != 0) {
		return valid_ret;
	}

#ifdef CONFIG_VIRTIOFS_DEBUG
	fuse_dump_attr_out(response);
#endif

	return 0;
}

int virtiofs_fsync(const struct device *dev, uint64_t inode, uint64_t fh)
{
	struct fuse_fsync_req req;

	fuse_create_fsync_req(&req, inode, fh);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header,
		  .len = sizeof(req.in_header) + sizeof(req.fsync_in) },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) }
	};

	LOG_INF(
		"sending FUSE_FSYNC, nodeid=%" PRIu64 ", fh=%" PRIu64 ", unique=%" PRIu64,
		inode, fh, req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 2, 1);

	LOG_INF("received FUSE_FSYNC response, unique=%" PRIu64, req.out_header.unique);

	return virtiofs_validate_response(
		&req.out_header, FUSE_FSYNC, used_len, sizeof(req.out_header)
	);
}

int virtiofs_mkdir(const struct device *dev, uint64_t inode, const char *dirname, uint32_t mode)
{
	struct fuse_mkdir_req req;
	uint32_t dirname_len = strlen(dirname) + 1;

	fuse_create_mkdir_req(&req, inode, dirname_len, mode);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = sizeof(req.in_header) + sizeof(req.mkdir_in) },
		{ .addr = (void *)dirname, .len = dirname_len },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) + sizeof(req.entry_out) }
	};

	LOG_INF(
		"sending FUSE_MKDIR %s, inode=%" PRIu64 ", unique=%" PRIu64,
		dirname, inode, req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 3, 2);

	LOG_INF("received FUSE_MKDIR response, unique=%" PRIu64, req.out_header.unique);

	int valid_ret = virtiofs_validate_response(
		&req.out_header, FUSE_MKDIR, used_len, buf[2].len
	);

	if (valid_ret != 0) {
		return valid_ret;
	}

	return 0;
}

int virtiofs_unlink(const struct device *dev, const char *fname, enum fuse_object_type type)
{
	struct fuse_unlink_req req;
	uint32_t fname_len = strlen(fname) + 1;

	fuse_create_unlink_req(&req, fname_len, type);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = sizeof(req.in_header) },
		{ .addr = (void *)fname, .len = fname_len },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) }
	};

	LOG_INF(
		"sending %s for %s, unique=%" PRIu64,
		type == FUSE_DIR ? "FUSE_RMDIR" : "FUSE_UNLINK", fname, req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 3, 2);

	LOG_INF(
		"received %s response, unique=%" PRIu64,
		type == FUSE_DIR ? "FUSE_RMDIR" : "FUSE_UNLINK", req.out_header.unique
	);

	return virtiofs_validate_response(
		&req.out_header, type == FUSE_DIR ? FUSE_RMDIR : FUSE_UNLINK, used_len,
		sizeof(req.out_header)
	);
}

int virtiofs_rename(
	const struct device *dev, uint64_t old_dir_inode, const char *old_name,
	uint64_t new_dir_inode, const char *new_name)
{
	struct fuse_rename_req req;
	uint32_t old_len = strlen(old_name) + 1;
	uint32_t new_len = strlen(new_name) + 1;

	fuse_create_rename_req(&req, old_dir_inode, old_len, new_dir_inode, new_len);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = sizeof(req.in_header) + sizeof(req.rename_in) },
		{ .addr = (void *)old_name, .len = old_len },
		{ .addr = (void *)new_name, .len = new_len },
		{ .addr = &req.out_header, .len = sizeof(req.out_header) }
	};

	LOG_INF(
		"sending FUSE_RENAME %s to %s, unique=%" PRIu64,
		old_name, new_name, req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 4, 3);

	LOG_INF("received FUSE_RENAME response, unique=%" PRIu64, req.out_header.unique);

	return virtiofs_validate_response(
		&req.out_header, FUSE_RENAME, used_len, sizeof(req.out_header)
	);
}

int virtiofs_statfs(const struct device *dev, struct fuse_kstatfs *response)
{
	struct fuse_kstatfs_req req;

	fuse_fill_header(&req.in_header, sizeof(req.in_header), FUSE_STATFS, FUSE_ROOT_INODE);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = sizeof(req.in_header) },
		{ .addr = &req.out_header,
		  .len = sizeof(req.out_header) + sizeof(req.kstatfs_out) }
	};

	LOG_INF("sending FUSE_STATFS, unique=%" PRIu64, req.in_header.unique);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 2, 1);

	LOG_INF("received FUSE_STATFS response, unique=%" PRIu64, req.out_header.unique);

	int valid_ret = virtiofs_validate_response(
		&req.out_header, FUSE_STATFS, used_len, buf[1].len
	);

	if (valid_ret != 0) {
		return valid_ret;
	}

#ifdef CONFIG_VIRTIOFS_DEBUG
	fuse_dump_kstafs(&req.kstatfs_out);
#endif

	*response = req.kstatfs_out;

	return 0;
}

int virtiofs_readdir(
	const struct device *dev, uint64_t inode, uint64_t fh, uint64_t offset,
	uint8_t *dirent_buf, uint32_t dirent_size, uint8_t *name_buf, uint32_t name_size)
{
	struct fuse_read_req req;

	fuse_create_read_req(&req, inode, fh, offset, dirent_size + name_size, FUSE_DIR);

	struct virtq_buf buf[] = {
		{ .addr = &req.in_header, .len = req.in_header.len },
		{ .addr = &req.out_header, .len = sizeof(struct fuse_out_header) },
		{ .addr = dirent_buf, .len = dirent_size },
		{ .addr = name_buf, .len = name_size }
	};

	LOG_INF(
		"sending FUSE_READDIR, nodeid=%" PRIu64 ", fh=%" PRIu64 ", offset=%" PRIu64
		", size=%" PRIu32 ", unique=%" PRIu64,
		inode, fh, offset, dirent_size + name_size, req.in_header.unique
	);
	uint32_t used_len = virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 4, 1);

	LOG_INF("received FUSE_READDIR response, unique=%" PRIu64, req.out_header.unique);

	int valid_ret = virtiofs_validate_response(&req.out_header, FUSE_READDIR, used_len, -1);

	if (valid_ret != 0) {
		return valid_ret;
	}

	return req.out_header.len - sizeof(req.out_header);
}

void virtiofs_forget(const struct device *dev, uint64_t inode, uint64_t nlookup)
{
	if (inode == FUSE_ROOT_INODE) {
		return;
	}

	struct fuse_forget_req req;

	fuse_fill_header(&req.in_header, sizeof(req.in_header), FUSE_FORGET, inode);
	req.forget_in.nlookup = nlookup; /* refcount will be decreased by this value */

	struct virtq_buf buf[] = {
		{ .addr = &req, .len = sizeof(req.in_header) + sizeof(req.forget_in) }
	};

	LOG_INF(
		"sending FUSE_FORGET nodeid=%" PRIu64 ", nlookup=%" PRIu64 ", unique=%" PRIu64,
		inode, nlookup, req.in_header.unique
	);
	virtiofs_send_receive(dev, REQUEST_QUEUE, buf, 1, 1);
	LOG_INF("received FUSE_FORGET response, unique=%" PRIu64, req.in_header.unique);

	/*
	 * In comparison to other fuse operations this one doesn't return fuse_out_header,
	 * despite virtio spec v1.3 5.11.6.1 saying that out header is common to all
	 * types of fuse requests (comment in include/uapi/linux/fuse.h states otherwise that
	 * FUSE_FORGET has no reply), so there is no error code to return
	 */
}
