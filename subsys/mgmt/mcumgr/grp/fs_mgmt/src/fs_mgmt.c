/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022 Laird Connectivity
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/fs/fs.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/grp/fs_mgmt/fs_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_hash_checksum.h>
#include <zephyr/logging/log.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_config.h>

#if defined(CONFIG_MCUMGR_GRP_FS_CHECKSUM_IEEE_CRC32)
#include <mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_hash_checksum_crc32.h>
#endif

#if defined(CONFIG_MCUMGR_GRP_FS_HASH_SHA256)
#include <mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_hash_checksum_sha256.h>
#endif

#if defined(CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS)
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#endif

#ifdef CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH
/* Define default hash/checksum */
#if defined(CONFIG_MCUMGR_GRP_FS_CHECKSUM_IEEE_CRC32)
#define MCUMGR_GRP_FS_CHECKSUM_HASH_DEFAULT "crc32"
#elif defined(CONFIG_MCUMGR_GRP_FS_HASH_SHA256)
#define MCUMGR_GRP_FS_CHECKSUM_HASH_DEFAULT "sha256"
#else
#error "Missing mcumgr fs checksum/hash algorithm selection?"
#endif

/* Define largest hach/checksum output size (bytes) */
#if defined(CONFIG_MCUMGR_GRP_FS_HASH_SHA256)
#define MCUMGR_GRP_FS_CHECKSUM_HASH_LARGEST_OUTPUT_SIZE 32
#elif defined(CONFIG_MCUMGR_GRP_FS_CHECKSUM_IEEE_CRC32)
#define MCUMGR_GRP_FS_CHECKSUM_HASH_LARGEST_OUTPUT_SIZE 4
#endif
#endif

LOG_MODULE_REGISTER(mcumgr_fs_grp, CONFIG_MCUMGR_GRP_FS_LOG_LEVEL);

#define HASH_CHECKSUM_TYPE_SIZE 8

#define HASH_CHECKSUM_SUPPORTED_COLUMNS_MAX 4

#if CONFIG_MCUMGR_GRP_FS_FILE_SEMAPHORE_TAKE_TIME == 0
#define FILE_SEMAPHORE_MAX_TAKE_TIME K_NO_WAIT
#else
#define FILE_SEMAPHORE_MAX_TAKE_TIME K_MSEC(CONFIG_MCUMGR_GRP_FS_FILE_SEMAPHORE_TAKE_TIME)
#endif

#define FILE_SEMAPHORE_MAX_TAKE_TIME_WORK_HANDLER K_MSEC(500)
#define FILE_CLOSE_IDLE_TIME K_MSEC(CONFIG_MCUMGR_GRP_FS_FILE_AUTOMATIC_IDLE_CLOSE_TIME)

enum {
	STATE_NO_UPLOAD_OR_DOWNLOAD = 0,
	STATE_UPLOAD,
	STATE_DOWNLOAD,
};

static struct {
	/** Whether an upload or download is currently in progress. */
	uint8_t state;

	/** Expected offset of next upload/download request. */
	size_t off;

	/**
	 * Total length of file currently being uploaded/downloaded. Note that for file
	 * uploads, it is possible for this to be lost in which case it is not known when
	 * the file can be closed, and the automatic close will need to close the file.
	 */
	size_t len;

	/** Path of file being accessed. */
	char path[CONFIG_MCUMGR_GRP_FS_PATH_LEN + 1];

	/** File handle. */
	struct fs_file_t file;

	/** Semaphore lock. */
	struct k_sem lock_sem;

	/** Which transport owns the lock on the on-going file transfer. */
	void *transport;

	/** Delayed workqueue used to close the file after a period of inactivity. */
	struct k_work_delayable file_close_work;
} fs_mgmt_ctxt;

static const struct mgmt_handler fs_mgmt_handlers[];

#if defined(CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH)
/* Hash/checksum iterator information passing structure */
struct fs_mgmt_hash_checksum_iterator_info {
	zcbor_state_t *zse;
	bool ok;
};
#endif

/* Clean up open file state */
static void fs_mgmt_cleanup(void)
{
	if (fs_mgmt_ctxt.state != STATE_NO_UPLOAD_OR_DOWNLOAD) {
		fs_mgmt_ctxt.state = STATE_NO_UPLOAD_OR_DOWNLOAD;
		fs_mgmt_ctxt.off = 0;
		fs_mgmt_ctxt.len = 0;
		memset(fs_mgmt_ctxt.path, 0, sizeof(fs_mgmt_ctxt.path));
		fs_close(&fs_mgmt_ctxt.file);
		fs_mgmt_ctxt.transport = NULL;
	}
}

static void file_close_work_handler(struct k_work *work)
{
	if (k_sem_take(&fs_mgmt_ctxt.lock_sem, FILE_SEMAPHORE_MAX_TAKE_TIME_WORK_HANDLER)) {
		/* Re-schedule to retry */
		k_work_reschedule(&fs_mgmt_ctxt.file_close_work, FILE_CLOSE_IDLE_TIME);
		return;
	}

	fs_mgmt_cleanup();

	k_sem_give(&fs_mgmt_ctxt.lock_sem);
}

static int fs_mgmt_filelen(const char *path, size_t *out_len)
{
	struct fs_dirent dirent;
	int rc;

	rc = fs_stat(path, &dirent);

	if (rc == -EINVAL) {
		return FS_MGMT_ERR_FILE_INVALID_NAME;
	} else if (rc == -ENOENT) {
		return FS_MGMT_ERR_FILE_NOT_FOUND;
	} else if (rc != 0) {
		return FS_MGMT_ERR_UNKNOWN;
	}

	if (dirent.type != FS_DIR_ENTRY_FILE) {
		return FS_MGMT_ERR_FILE_IS_DIRECTORY;
	}

	*out_len = dirent.size;

	return FS_MGMT_ERR_OK;
}

/**
 * Encodes a file upload response.
 */
static bool fs_mgmt_file_rsp(zcbor_state_t *zse, int rc, uint64_t off)
{
	bool ok = true;

	if (IS_ENABLED(CONFIG_MCUMGR_SMP_LEGACY_RC_BEHAVIOUR) || rc != 0) {
		ok = zcbor_tstr_put_lit(zse, "rc")	&&
		     zcbor_int32_put(zse, rc);
	}

	return ok && zcbor_tstr_put_lit(zse, "off")	&&
		     zcbor_uint64_put(zse, off);
}

/**
 * Cleans up open file handle and state when upload is finished.
 */
static void fs_mgmt_upload_download_finish_check(void)
{
	if (fs_mgmt_ctxt.len > 0 && fs_mgmt_ctxt.off >= fs_mgmt_ctxt.len) {
		/* File upload/download has finished, clean up */
		k_work_cancel_delayable(&fs_mgmt_ctxt.file_close_work);
		fs_mgmt_cleanup();
	} else {
		k_work_reschedule(&fs_mgmt_ctxt.file_close_work, FILE_CLOSE_IDLE_TIME);
	}
}

/**
 * Command handler: fs file (read)
 */
static int fs_mgmt_file_download(struct smp_streamer *ctxt)
{
	uint8_t file_data[MCUMGR_GRP_FS_DL_CHUNK_SIZE];
	char path[CONFIG_MCUMGR_GRP_FS_PATH_LEN + 1];
	uint64_t off = ULLONG_MAX;
	ssize_t bytes_read = 0;
	int rc;
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok;
	struct zcbor_string name = { 0 };
	size_t decoded;

	struct zcbor_map_decode_key_val fs_download_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("off", zcbor_uint64_decode, &off),
		ZCBOR_MAP_DECODE_KEY_DECODER("name", zcbor_tstr_decode, &name),
	};

#if defined(CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK)
	struct fs_mgmt_file_access file_access_data = {
		.access = FS_MGMT_FILE_ACCESS_READ,
		.filename = path,
	};

	enum mgmt_cb_return status;
	int32_t err_rc;
	uint16_t err_group;
#endif

	ok = zcbor_map_decode_bulk(zsd, fs_download_decode, ARRAY_SIZE(fs_download_decode),
				   &decoded) == 0;

	if (!ok || off == ULLONG_MAX || name.len == 0 || name.len > (sizeof(path) - 1)) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(path, name.value, name.len);
	path[name.len] = '\0';

	if (k_sem_take(&fs_mgmt_ctxt.lock_sem, FILE_SEMAPHORE_MAX_TAKE_TIME)) {
		return MGMT_ERR_EBUSY;
	}

	/* Check if this download is already in progress */
	if (ctxt->smpt != fs_mgmt_ctxt.transport ||
	    fs_mgmt_ctxt.state != STATE_DOWNLOAD ||
	    strcmp(path, fs_mgmt_ctxt.path)) {
#if defined(CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK)
		/* Send request to application to check if access should be allowed or not */
		status = mgmt_callback_notify(MGMT_EVT_OP_FS_MGMT_FILE_ACCESS, &file_access_data,
					      sizeof(file_access_data), &err_rc, &err_group);

		if (status != MGMT_CB_OK) {
			if (status == MGMT_CB_ERROR_RC) {
				k_sem_give(&fs_mgmt_ctxt.lock_sem);
				return err_rc;
			}

			ok = smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);
			goto end;
		}
#endif

		fs_mgmt_cleanup();
	}

	/* Open new file */
	if (fs_mgmt_ctxt.state == STATE_NO_UPLOAD_OR_DOWNLOAD) {
		rc = fs_mgmt_filelen(path, &fs_mgmt_ctxt.len);

		if (rc != FS_MGMT_ERR_OK) {
			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, rc);
			goto end;
		}

		fs_mgmt_ctxt.off = 0;
		fs_file_t_init(&fs_mgmt_ctxt.file);
		rc = fs_open(&fs_mgmt_ctxt.file, path, FS_O_READ);

		if (rc != 0) {
			if (rc == -EINVAL) {
				rc = FS_MGMT_ERR_FILE_INVALID_NAME;
			} else if (rc == -ENOENT) {
				rc = FS_MGMT_ERR_FILE_NOT_FOUND;
			} else {
				rc = FS_MGMT_ERR_UNKNOWN;
			}

			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, rc);
			goto end;
		}

		strcpy(fs_mgmt_ctxt.path, path);
		fs_mgmt_ctxt.state = STATE_DOWNLOAD;
		fs_mgmt_ctxt.transport = ctxt->smpt;
	}

	/* Seek to desired offset */
	if (off != fs_mgmt_ctxt.off) {
		rc = fs_seek(&fs_mgmt_ctxt.file, off, FS_SEEK_SET);

		if (rc != 0) {
			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS,
					     FS_MGMT_ERR_FILE_SEEK_FAILED);
			fs_mgmt_cleanup();
			goto end;
		}

		fs_mgmt_ctxt.off = off;
	}

	/* Only the response to the first download request contains the total file
	 * length.
	 */

	/* Read the requested chunk from the file. */
	bytes_read = fs_read(&fs_mgmt_ctxt.file, file_data, MCUMGR_GRP_FS_DL_CHUNK_SIZE);

	if (bytes_read < 0) {
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, FS_MGMT_ERR_FILE_READ_FAILED);
		fs_mgmt_cleanup();
		goto end;
	}

	/* Increment offset */
	fs_mgmt_ctxt.off += bytes_read;

	/* Encode the response. */
	ok = fs_mgmt_file_rsp(zse, MGMT_ERR_EOK, off)				&&
	     zcbor_tstr_put_lit(zse, "data")					&&
	     zcbor_bstr_encode_ptr(zse, file_data, bytes_read)			&&
	     ((off != 0)							||
		(zcbor_tstr_put_lit(zse, "len") && zcbor_uint64_put(zse, fs_mgmt_ctxt.len)));

	fs_mgmt_upload_download_finish_check();

end:
	rc = (ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE);
	k_sem_give(&fs_mgmt_ctxt.lock_sem);

	return rc;
}

/**
 * Command handler: fs file (write)
 */
static int fs_mgmt_file_upload(struct smp_streamer *ctxt)
{
	char file_name[CONFIG_MCUMGR_GRP_FS_PATH_LEN + 1];
	unsigned long long len = ULLONG_MAX;
	unsigned long long off = ULLONG_MAX;
	bool ok;
	int rc;
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	struct zcbor_string name = { 0 };
	struct zcbor_string file_data = { 0 };
	size_t decoded = 0;
	ssize_t existing_file_size = 0;

	struct zcbor_map_decode_key_val fs_upload_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("off", zcbor_uint64_decode, &off),
		ZCBOR_MAP_DECODE_KEY_DECODER("name", zcbor_tstr_decode, &name),
		ZCBOR_MAP_DECODE_KEY_DECODER("data", zcbor_bstr_decode, &file_data),
		ZCBOR_MAP_DECODE_KEY_DECODER("len", zcbor_uint64_decode, &len),
	};

#if defined(CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK)
	struct fs_mgmt_file_access file_access_data = {
		.access = FS_MGMT_FILE_ACCESS_WRITE,
		.filename = file_name,
	};

	enum mgmt_cb_return status;
	int32_t err_rc;
	uint16_t err_group;
#endif

	ok = zcbor_map_decode_bulk(zsd, fs_upload_decode, ARRAY_SIZE(fs_upload_decode),
				   &decoded) == 0;

	if (!ok || off == ULLONG_MAX || name.len == 0 || name.len > (sizeof(file_name) - 1) ||
	    (off == 0 && len == ULLONG_MAX)) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(file_name, name.value, name.len);
	file_name[name.len] = '\0';

	if (k_sem_take(&fs_mgmt_ctxt.lock_sem, FILE_SEMAPHORE_MAX_TAKE_TIME)) {
		return MGMT_ERR_EBUSY;
	}

	/* Check if this upload is already in progress */
	if (ctxt->smpt != fs_mgmt_ctxt.transport ||
	    fs_mgmt_ctxt.state != STATE_UPLOAD ||
	    strcmp(file_name, fs_mgmt_ctxt.path)) {
#if defined(CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK)
		/* Send request to application to check if access should be allowed or not */
		status = mgmt_callback_notify(MGMT_EVT_OP_FS_MGMT_FILE_ACCESS, &file_access_data,
					      sizeof(file_access_data), &err_rc, &err_group);

		if (status != MGMT_CB_OK) {
			if (status == MGMT_CB_ERROR_RC) {
				k_sem_give(&fs_mgmt_ctxt.lock_sem);
				return err_rc;
			}

			ok = smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);
			goto end;
		}
#endif

		fs_mgmt_cleanup();
	}

	/* Open new file */
	if (fs_mgmt_ctxt.state == STATE_NO_UPLOAD_OR_DOWNLOAD) {
		fs_mgmt_ctxt.off = 0;
		fs_file_t_init(&fs_mgmt_ctxt.file);
		rc = fs_open(&fs_mgmt_ctxt.file, file_name, FS_O_CREATE | FS_O_WRITE);

		if (rc != 0) {
			if (rc == -EINVAL) {
				rc = FS_MGMT_ERR_FILE_INVALID_NAME;
			} else if (rc == -ENOENT) {
				rc = FS_MGMT_ERR_MOUNT_POINT_NOT_FOUND;
			} else if (rc == -EROFS) {
				rc = FS_MGMT_ERR_READ_ONLY_FILESYSTEM;
			} else {
				rc = FS_MGMT_ERR_UNKNOWN;
			}

			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, rc);
			goto end;
		}

		strcpy(fs_mgmt_ctxt.path, file_name);
		fs_mgmt_ctxt.state = STATE_UPLOAD;
		fs_mgmt_ctxt.transport = ctxt->smpt;
	}

	if (off == 0) {
		/* Store the uploaded file size from the first packet, this will allow
		 * closing the file when the full upload has finished, however the file
		 * will remain opened if the upload state is lost. It will, however,
		 * still be closed automatically after a timeout.
		 */
		fs_mgmt_ctxt.len = len;
		rc = fs_mgmt_filelen(file_name, &existing_file_size);

		if (rc != 0) {
			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, rc);
			fs_mgmt_cleanup();
			goto end;
		}
	} else if (fs_mgmt_ctxt.off == 0) {
		rc = fs_mgmt_filelen(file_name, &fs_mgmt_ctxt.off);

		if (rc != 0) {
			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, rc);
			fs_mgmt_cleanup();
			goto end;
		}
	}

	/* Verify that the data offset matches the expected offset (i.e. current size of file) */
	if (off > 0 && off != fs_mgmt_ctxt.off) {
		/* Offset mismatch, send file length, client needs to handle this */
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, FS_MGMT_ERR_FILE_OFFSET_NOT_VALID);
		ok = zcbor_tstr_put_lit(zse, "len")		&&
		     zcbor_uint64_put(zse, fs_mgmt_ctxt.off);

		/* Because the client would most likely decide to abort and transfer and start
		 * again, clean everything up and release the file handle so it can be used
		 * elsewhere (if needed).
		 */
		fs_mgmt_cleanup();
		goto end;
	}

	if (file_data.len > 0) {
		/* Write the data chunk to the file. */
		if (off == 0 && existing_file_size != 0) {
			/* Offset is 0 and existing file exists with data, attempt to truncate
			 * the file size to 0
			 */
			rc = fs_seek(&fs_mgmt_ctxt.file, 0, FS_SEEK_SET);

			if (rc != 0) {
				ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS,
						     FS_MGMT_ERR_FILE_SEEK_FAILED);
				fs_mgmt_cleanup();
				goto end;
			}

			rc = fs_truncate(&fs_mgmt_ctxt.file, 0);

			if (rc == -ENOTSUP) {
				/* Truncation not supported by filesystem, therefore close file,
				 * delete it then re-open it
				 */
				fs_close(&fs_mgmt_ctxt.file);

				rc = fs_unlink(file_name);
				if (rc < 0 && rc != -ENOENT) {
					ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS,
							     FS_MGMT_ERR_FILE_DELETE_FAILED);
					fs_mgmt_cleanup();
					goto end;
				}

				rc = fs_open(&fs_mgmt_ctxt.file, file_name, FS_O_CREATE |
					     FS_O_WRITE);
			}

			if (rc < 0) {
				/* Failed to truncate file */
				ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS,
						     FS_MGMT_ERR_FILE_TRUNCATE_FAILED);
				fs_mgmt_cleanup();
				goto end;
			}
		} else if (fs_tell(&fs_mgmt_ctxt.file) != off) {
			/* The offset has been validated to be file size previously, seek to
			 * the end of the file to write the new data.
			 */
			rc = fs_seek(&fs_mgmt_ctxt.file, 0, FS_SEEK_END);

			if (rc < 0) {
				/* Failed to seek in file */
				ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS,
						     FS_MGMT_ERR_FILE_SEEK_FAILED);
				fs_mgmt_cleanup();
				goto end;
			}
		}

		rc = fs_write(&fs_mgmt_ctxt.file, file_data.value, file_data.len);

		if (rc < 0) {
			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS,
					     FS_MGMT_ERR_FILE_WRITE_FAILED);
			fs_mgmt_cleanup();
			goto end;
		}

		fs_mgmt_ctxt.off += file_data.len;
	}

	/* Send the response. */
	ok = fs_mgmt_file_rsp(zse, MGMT_ERR_EOK, fs_mgmt_ctxt.off);
	fs_mgmt_upload_download_finish_check();

end:
	rc = (ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE);
	k_sem_give(&fs_mgmt_ctxt.lock_sem);

	return rc;
}

#if defined(CONFIG_MCUMGR_GRP_FS_FILE_STATUS)
/**
 * Command handler: fs stat (read)
 */
static int fs_mgmt_file_status(struct smp_streamer *ctxt)
{
	char path[CONFIG_MCUMGR_GRP_FS_PATH_LEN + 1];
	size_t file_len;
	int rc;
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok;
	struct zcbor_string name = { 0 };
	size_t decoded;

	struct zcbor_map_decode_key_val fs_status_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("name", zcbor_tstr_decode, &name),
	};

#if defined(CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK)
	struct fs_mgmt_file_access file_access_data = {
		.access = FS_MGMT_FILE_ACCESS_STATUS,
		.filename = path,
	};

	enum mgmt_cb_return status;
	int32_t err_rc;
	uint16_t err_group;
#endif

	ok = zcbor_map_decode_bulk(zsd, fs_status_decode,
		ARRAY_SIZE(fs_status_decode), &decoded) == 0;

	if (!ok || name.len == 0 || name.len > (sizeof(path) - 1)) {
		return MGMT_ERR_EINVAL;
	}

	/* Copy path and ensure it is null-teminated */
	memcpy(path, name.value, name.len);
	path[name.len] = '\0';

#if defined(CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK)
	/* Send request to application to check if access should be allowed or not */
	status = mgmt_callback_notify(MGMT_EVT_OP_FS_MGMT_FILE_ACCESS, &file_access_data,
				      sizeof(file_access_data), &err_rc, &err_group);

	if (status != MGMT_CB_OK) {
		if (status == MGMT_CB_ERROR_RC) {
			return err_rc;
		}

		ok = smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);
		return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
	}
#endif

	/* Retrieve file size */
	rc = fs_mgmt_filelen(path, &file_len);

	if (rc != 0) {
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, rc);
		goto end;
	}

	/* Encode the response. */
	if (IS_ENABLED(CONFIG_MCUMGR_SMP_LEGACY_RC_BEHAVIOUR)) {
		ok = zcbor_tstr_put_lit(zse, "rc")	&&
		     zcbor_int32_put(zse, rc);
	}

	ok = ok && zcbor_tstr_put_lit(zse, "len")	&&
		   zcbor_uint64_put(zse, file_len);

end:
	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	return MGMT_ERR_EOK;
}
#endif

#if defined(CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH)
/**
 * Command handler: fs hash/checksum (read)
 */
static int fs_mgmt_file_hash_checksum(struct smp_streamer *ctxt)
{
	char path[CONFIG_MCUMGR_GRP_FS_PATH_LEN + 1];
	char type_arr[HASH_CHECKSUM_TYPE_SIZE + 1] = MCUMGR_GRP_FS_CHECKSUM_HASH_DEFAULT;
	char output[MCUMGR_GRP_FS_CHECKSUM_HASH_LARGEST_OUTPUT_SIZE];
	uint64_t len = ULLONG_MAX;
	uint64_t off = 0;
	size_t file_len;
	int rc;
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok;
	struct zcbor_string type = { 0 };
	struct zcbor_string name = { 0 };
	size_t decoded;
	struct fs_file_t file;
	const struct fs_mgmt_hash_checksum_group *group = NULL;

	struct zcbor_map_decode_key_val fs_hash_checksum_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("type", zcbor_tstr_decode, &type),
		ZCBOR_MAP_DECODE_KEY_DECODER("name", zcbor_tstr_decode, &name),
		ZCBOR_MAP_DECODE_KEY_DECODER("off", zcbor_uint64_decode, &off),
		ZCBOR_MAP_DECODE_KEY_DECODER("len", zcbor_uint64_decode, &len),
	};

#if defined(CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK)
	struct fs_mgmt_file_access file_access_data = {
		.access = FS_MGMT_FILE_ACCESS_HASH_CHECKSUM,
		.filename = path,
	};

	enum mgmt_cb_return status;
	int32_t err_rc;
	uint16_t err_group;
#endif

	ok = zcbor_map_decode_bulk(zsd, fs_hash_checksum_decode,
		ARRAY_SIZE(fs_hash_checksum_decode), &decoded) == 0;

	if (!ok || name.len == 0 || name.len > (sizeof(path) - 1) ||
	    type.len > (sizeof(type_arr) - 1) || len == 0) {
		return MGMT_ERR_EINVAL;
	}

	/* Copy strings and ensure they are null-teminated */
	memcpy(path, name.value, name.len);
	path[name.len] = '\0';

	if (type.len != 0) {
		memcpy(type_arr, type.value, type.len);
		type_arr[type.len] = '\0';
	}

	/* Search for supported hash/checksum */
	group = fs_mgmt_hash_checksum_find_handler(type_arr);

	if (group == NULL) {
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS,
				     FS_MGMT_ERR_CHECKSUM_HASH_NOT_FOUND);
		goto end;
	}

#if defined(CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK)
	/* Send request to application to check if access should be allowed or not */
	status = mgmt_callback_notify(MGMT_EVT_OP_FS_MGMT_FILE_ACCESS, &file_access_data,
				      sizeof(file_access_data), &err_rc, &err_group);

	if (status != MGMT_CB_OK) {
		if (status == MGMT_CB_ERROR_RC) {
			return err_rc;
		}

		ok = smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);
		return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
	}
#endif

	/* Check provided offset is valid for target file */
	rc = fs_mgmt_filelen(path, &file_len);

	if (rc != 0) {
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, rc);
		goto end;
	}

	if (file_len <= off) {
		/* Requested offset is larger than target file size or file length is 0, which
		 * means no hash/checksum can be performed
		 */
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS,
				     (file_len == 0 ? FS_MGMT_ERR_FILE_EMPTY :
						      FS_MGMT_ERR_FILE_OFFSET_LARGER_THAN_FILE));
		goto end;
	}

	/* Open file for reading and pass to hash/checksum generation function */
	fs_file_t_init(&file);
	rc = fs_open(&file, path, FS_O_READ);

	if (rc != 0) {
		if (rc == -EINVAL) {
			rc = FS_MGMT_ERR_FILE_INVALID_NAME;
		} else if (rc == -ENOENT) {
			rc = FS_MGMT_ERR_FILE_NOT_FOUND;
		} else {
			rc = FS_MGMT_ERR_UNKNOWN;
		}

		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, rc);
		goto end;
	}

	/* Seek to file's desired offset, if parameter was provided */
	if (off != 0) {
		rc = fs_seek(&file, off, FS_SEEK_SET);

		if (rc != 0) {
			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS,
					     FS_MGMT_ERR_FILE_SEEK_FAILED);
			fs_close(&file);
			goto end;
		}
	}

	/* Calculate hash/checksum using function */
	file_len = 0;
	rc = group->function(&file, output, &file_len, len);

	fs_close(&file);

	/* Encode the response */
	if (rc != 0) {
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_FS, rc);
		goto end;
	}

	ok &= zcbor_tstr_put_lit(zse, "type")	&&
	      zcbor_tstr_put_term(zse, type_arr, sizeof(type_arr));

	if (off != 0) {
		ok &= zcbor_tstr_put_lit(zse, "off")	&&
		      zcbor_uint64_put(zse, off);
	}

	ok &= zcbor_tstr_put_lit(zse, "len")	&&
	      zcbor_uint64_put(zse, file_len)	&&
	      zcbor_tstr_put_lit(zse, "output");

	if (group->byte_string == true) {
		/* Output is a byte string */
		ok &= zcbor_bstr_encode_ptr(zse, output, group->output_size);
	} else {
		/* Output is a number */
		uint64_t tmp_val = 0;

		if (group->output_size == sizeof(uint8_t)) {
			tmp_val = (uint64_t)(*(uint8_t *)output);
#if MCUMGR_GRP_FS_CHECKSUM_HASH_LARGEST_OUTPUT_SIZE > 1
		} else if (group->output_size == sizeof(uint16_t)) {
			tmp_val = (uint64_t)(*(uint16_t *)output);
#if MCUMGR_GRP_FS_CHECKSUM_HASH_LARGEST_OUTPUT_SIZE > 2
		} else if (group->output_size == sizeof(uint32_t)) {
			tmp_val = (uint64_t)(*(uint32_t *)output);
#if MCUMGR_GRP_FS_CHECKSUM_HASH_LARGEST_OUTPUT_SIZE > 4
		} else if (group->output_size == sizeof(uint64_t)) {
			tmp_val = (*(uint64_t *)output);
#endif
#endif
#endif
		} else {
			LOG_ERR("Unable to handle numerical checksum size %u",
				group->output_size);

			return MGMT_ERR_EUNKNOWN;
		}

		ok &= zcbor_uint64_put(zse, tmp_val);
	}

end:
	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	return MGMT_ERR_EOK;
}

#if defined(CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH_SUPPORTED_CMD)
/* Callback for supported hash/checksum types to encode details on one type into CBOR map */
static void fs_mgmt_supported_hash_checksum_callback(
					const struct fs_mgmt_hash_checksum_group *group,
					void *user_data)
{
	struct fs_mgmt_hash_checksum_iterator_info *ctx =
			(struct fs_mgmt_hash_checksum_iterator_info *)user_data;

	if (!ctx->ok) {
		return;
	}

	ctx->ok = zcbor_tstr_encode_ptr(ctx->zse, group->group_name, strlen(group->group_name))	&&
		  zcbor_map_start_encode(ctx->zse, HASH_CHECKSUM_SUPPORTED_COLUMNS_MAX)		&&
		  zcbor_tstr_put_lit(ctx->zse, "format")					&&
		  zcbor_uint32_put(ctx->zse, (uint32_t)group->byte_string)			&&
		  zcbor_tstr_put_lit(ctx->zse, "size")						&&
		  zcbor_uint32_put(ctx->zse, (uint32_t)group->output_size)			&&
		  zcbor_map_end_encode(ctx->zse, HASH_CHECKSUM_SUPPORTED_COLUMNS_MAX);
}

/**
 * Command handler: fs supported hash/checksum (read)
 */
static int
fs_mgmt_supported_hash_checksum(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	struct fs_mgmt_hash_checksum_iterator_info itr_ctx = {
		.zse = zse,
	};

	itr_ctx.ok = zcbor_tstr_put_lit(zse, "types") &&
	    zcbor_map_start_encode(zse, CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH_SUPPORTED_MAX_TYPES);

	if (!itr_ctx.ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	fs_mgmt_hash_checksum_find_handlers(fs_mgmt_supported_hash_checksum_callback, &itr_ctx);

	if (!itr_ctx.ok ||
	    !zcbor_map_end_encode(zse, CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH_SUPPORTED_MAX_TYPES)) {
		return MGMT_ERR_EMSGSIZE;
	}

	return MGMT_ERR_EOK;
}
#endif
#endif

/**
 * Command handler: fs opened file (write)
 */
static int fs_mgmt_close_opened_file(struct smp_streamer *ctxt)
{
	if (k_sem_take(&fs_mgmt_ctxt.lock_sem, FILE_SEMAPHORE_MAX_TAKE_TIME)) {
		return MGMT_ERR_EBUSY;
	}

	fs_mgmt_cleanup();

	k_sem_give(&fs_mgmt_ctxt.lock_sem);

	return MGMT_ERR_EOK;
}

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/*
 * @brief	Translate FS mgmt group error code into MCUmgr error code
 *
 * @param ret	#fs_mgmt_err_code_t error code
 *
 * @return	#mcumgr_err_t error code
 */
static int fs_mgmt_translate_error_code(uint16_t err)
{
	int rc;

	switch (err) {
	case FS_MGMT_ERR_FILE_INVALID_NAME:
	case FS_MGMT_ERR_CHECKSUM_HASH_NOT_FOUND:
		rc = MGMT_ERR_EINVAL;
		break;

	case FS_MGMT_ERR_FILE_NOT_FOUND:
	case FS_MGMT_ERR_MOUNT_POINT_NOT_FOUND:
		rc = MGMT_ERR_ENOENT;
		break;

	case FS_MGMT_ERR_UNKNOWN:
	case FS_MGMT_ERR_FILE_IS_DIRECTORY:
	case FS_MGMT_ERR_FILE_OPEN_FAILED:
	case FS_MGMT_ERR_FILE_SEEK_FAILED:
	case FS_MGMT_ERR_FILE_READ_FAILED:
	case FS_MGMT_ERR_FILE_TRUNCATE_FAILED:
	case FS_MGMT_ERR_FILE_DELETE_FAILED:
	case FS_MGMT_ERR_FILE_WRITE_FAILED:
	case FS_MGMT_ERR_FILE_OFFSET_NOT_VALID:
	case FS_MGMT_ERR_FILE_OFFSET_LARGER_THAN_FILE:
	case FS_MGMT_ERR_READ_ONLY_FILESYSTEM:
	default:
		rc = MGMT_ERR_EUNKNOWN;
	}

	return rc;
}
#endif

static const struct mgmt_handler fs_mgmt_handlers[] = {
	[FS_MGMT_ID_FILE] = {
		.mh_read = fs_mgmt_file_download,
		.mh_write = fs_mgmt_file_upload,
	},
#if defined(CONFIG_MCUMGR_GRP_FS_FILE_STATUS)
	[FS_MGMT_ID_STAT] = {
		.mh_read = fs_mgmt_file_status,
		.mh_write = NULL,
	},
#endif
#if defined(CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH)
	[FS_MGMT_ID_HASH_CHECKSUM] = {
		.mh_read = fs_mgmt_file_hash_checksum,
		.mh_write = NULL,
	},
#if defined(CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH_SUPPORTED_CMD)
	[FS_MGMT_ID_SUPPORTED_HASH_CHECKSUM] = {
		.mh_read = fs_mgmt_supported_hash_checksum,
		.mh_write = NULL,
	},
#endif
#endif
	[FS_MGMT_ID_OPENED_FILE] = {
		.mh_read = NULL,
		.mh_write = fs_mgmt_close_opened_file,
	},
};

#define FS_MGMT_HANDLER_CNT ARRAY_SIZE(fs_mgmt_handlers)

static struct mgmt_group fs_mgmt_group = {
	.mg_handlers = fs_mgmt_handlers,
	.mg_handlers_count = FS_MGMT_HANDLER_CNT,
	.mg_group_id = MGMT_GROUP_ID_FS,
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	.mg_translate_error = fs_mgmt_translate_error_code,
#endif
};

static void fs_mgmt_register_group(void)
{
	/* Initialise state variables */
	fs_mgmt_ctxt.state = STATE_NO_UPLOAD_OR_DOWNLOAD;
	k_sem_init(&fs_mgmt_ctxt.lock_sem, 1, 1);
	k_work_init_delayable(&fs_mgmt_ctxt.file_close_work, file_close_work_handler);

	mgmt_register_group(&fs_mgmt_group);

#if defined(CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH)
	/* Register any supported hash or checksum functions */
#if defined(CONFIG_MCUMGR_GRP_FS_CHECKSUM_IEEE_CRC32)
	fs_mgmt_hash_checksum_register_crc32();
#endif

#if defined(CONFIG_MCUMGR_GRP_FS_HASH_SHA256)
	fs_mgmt_hash_checksum_register_sha256();
#endif
#endif
}

MCUMGR_HANDLER_DEFINE(fs_mgmt, fs_mgmt_register_group);
