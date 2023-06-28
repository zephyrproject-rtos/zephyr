/*
 * Copyright (c) 2023 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <mbedtls/sha1.h>
#include <zephyr/net/cert_store.h>
#include <zephyr/log.h>
#include <errno.h>

#if CONFIG_FILE_SYSTEM_LITTLEFS
#define MAX_PATH_LEN 255
#else
#if CONFIG_FAT_FILESYSTEM_ELM
#define MAX_PATH_LEN 128
#else
#define MAX_PATH_LEN 100
#endif /* CONFIG_FAT_FILESYSTEM_ELM */
#endif /* CONFIG_FILE_SYSTEM_LITTLEFS */

int cert_store_open(const char *store_path, struct cert_store *store)
{
	int ret;

	store->store_path = store_path;
	fs_dir_t_init(&store->store_dir);
	ret = fs_opendir(&store->store_dir, store_path);

	if (ret == -EINVAL){
		ret = fs_mkdir(store_path);
		if (ret) {
			return ret;
		}
	}
	else if (ret)
	{
		return ret;
	}

	return 0;
}

int cert_store_close(struct cert_store *store)
{
	return fs_closedir(&store->store_dir);
}

int cert_store_fingerprint(const uint8_t *cert_buf, size_t cert_sz, uint8_t *fingerprint_buf)
{
	mbedtls_sha1_context sha1_ctx;
	unsigned char sha1_output[FINGERPRINT_SHA_SIZE];
	int rc;

	mbedtls_sha1_init(&sha1_ctx);

	memset(sha1_output, 0, sizeof(sha1_output));

	rc = mbedtls_sha1_starts(&sha1_ctx);
	if (rc) {
		return rc;
	}

	rc = mbedtls_sha1_update(&sha1_ctx, (unsigned char *)cert_buf, cert_sz);
	if (rc) {
		return rc;
	}

	rc = mbedtls_sha1_finish(&sha1_ctx, sha1_output);
	if (rc) {
		return rc;
	}

	bin2hex(sha1_output, sizeof(sha1_output), fingerprint_buf, FINGERPRINT_HEX_SIZE);

	return 0;
}

int cert_store_mgmt_store(struct cert_store *store, const uint8_t *cert_buf, size_t cert_sz)
{
	char fingerprint_buf[FINGERPRINT_HEX_SIZE];
	char path[MAX_PATH_LEN + 1];
	struct fs_file_t zfp;
	int ret = cert_store_fingerprint(cert_buf, cert_sz, fingerprint_buf);

	if (ret) {
		return ret;
	}

	/* Check if certificate already exists */
	fs_file_t_init(&zfp);
	snprintk(path, MAX_PATH_LEN, "%s/%s", store->store_path, fingerprint_buf);
	ret = fs_open(&zfp, path, 0);

	if (ret != EINVAL) {
		if (ret == 0) {
			return -EEXIST;
		}
		LOG_ERR("Failed to open file for existance check, err = %d", ret);
		return ret;
	}
	fs_close(&zfp);

	/* Write certificate */
	ret = fs_open(&zfp, path, FS_O_CREATE | FS_O_WRITE);
	if (ret) {
		LOG_ERR("Failed to open file for read, err = %d", ret);
		return ret;
	}

	ret = fs_write(&zfp, cert_buf, cert_sz);
	if (ret) {
		LOG_ERR("Failed to write certificate to flash, err = %d", ret);
		return ret;
	}
	fs_close(&zfp);

	return 0;
}

int cert_store_mgmt_delete(const struct cert_store *store, const uint8_t *cert_finger)
{
	char fingerprint_buf[FINGERPRINT_HEX_SIZE];
	char path[MAX_PATH_LEN + 1];

	/* Convert fingerprint to hex */
	bin2hex(cert_finger, FINGERPRINT_SHA_SIZE, fingerprint_buf, FINGERPRINT_HEX_SIZE);

	/* Try to delete */
	snprintk(path, MAX_PATH_LEN, "%s/%s", store->store_path, fingerprint_buf);

	return fs_unlink(path);
}

int cert_store_mgmt_load(const struct cert_store *store, const uint8_t *cert_finger,
			 const uint8_t *cert_buf, size_t buf_sz, size_t *cert_sz)
{
	char fingerprint_buf[FINGERPRINT_HEX_SIZE];
	char path[MAX_PATH_LEN + 1];
	struct fs_file_t zfp;
	int ret;

	/* Convert fingerprint to hex */
	bin2hex(cert_finger, FINGERPRINT_SHA_SIZE, fingerprint_buf, FINGERPRINT_HEX_SIZE);

	/* Try to read */
	snprintk(path, MAX_PATH_LEN, "%s/%s", store->store_path, fingerprint_buf);
	fs_file_t_init(&zfp);
	ret = fs_open(&zfp, path, FS_O_READ);
	if (ret) {
		if (ret == -EINVAL) {
			return -ENOENT;
		}
		return ret;
	}

	/* Check size */
	fs_seek(&zfp, 0, FS_SEEK_END);
	*cert_sz = fs_tell(&zfp);

	if (*cert_sz > buf_sz) {
		fs_close(&zfp);
		return -EFBIG;
	}

	/* Read to buffer */
	fs_seek(&zfp, 0, FS_SEEK_SET);

	ret = fs_read(&zfp, cert_buf, buf_sz);

	if (ret < 0) {
		fs_close(&zfp);
		LOG_ERR("Certificate read error, err = %d", ret);
		return ret;
	}

	fs_close(&zfp);

	return 0;
}
