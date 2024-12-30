/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/store.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/zms.h>
#include <zephyr/storage/flash_map.h>
#ifdef CONFIG_SECURE_STORAGE_ITS_IMPLEMENTATION_ZEPHYR
#include <zephyr/secure_storage/its/transform.h>
#endif

LOG_MODULE_DECLARE(secure_storage, CONFIG_SECURE_STORAGE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_SECURE_STORAGE_ITS_STORE_ZMS_SECTOR_SIZE
	     > 2 * CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE);

#define PARTITION_DT_NODE DT_CHOSEN(secure_storage_its_partition)

static struct zms_fs s_zms = {
	.flash_device = FIXED_PARTITION_NODE_DEVICE(PARTITION_DT_NODE),
	.offset = FIXED_PARTITION_NODE_OFFSET(PARTITION_DT_NODE),
	.sector_size = CONFIG_SECURE_STORAGE_ITS_STORE_ZMS_SECTOR_SIZE,
};

static int init_zms(void)
{
	int ret;

	s_zms.sector_count = FIXED_PARTITION_NODE_SIZE(PARTITION_DT_NODE) / s_zms.sector_size;

	ret = zms_mount(&s_zms);
	if (ret) {
		LOG_DBG("Failed. (%d)", ret);
	}
	return ret;
}
SYS_INIT(init_zms, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/* Bit position of the ITS caller ID in the ZMS entry ID. */
#define ITS_CALLER_ID_POS 30
/* Make sure that every ITS caller ID fits in ZMS entry IDs at the defined position. */
BUILD_ASSERT(1 << (32 - ITS_CALLER_ID_POS) >= SECURE_STORAGE_ITS_CALLER_COUNT);

static bool has_forbidden_bits_set(secure_storage_its_uid_t uid)
{
	if (uid.uid & GENMASK64(63, ITS_CALLER_ID_POS)) {
		LOG_DBG("UID %u/0x%llx cannot be used as it has bits set past "
			"the first " STRINGIFY(ITS_CALLER_ID_POS) " ones.",
			uid.caller_id, (unsigned long long)uid.uid);
		return true;
	}
	return false;
}

static uint32_t zms_id_from(secure_storage_its_uid_t uid)
{
	return (uint32_t)uid.uid | (uid.caller_id << ITS_CALLER_ID_POS);
}

psa_status_t secure_storage_its_store_set(secure_storage_its_uid_t uid,
					  size_t data_length, const void *data)
{
	psa_status_t psa_ret;
	ssize_t zms_ret;
	const uint32_t zms_id = zms_id_from(uid);

	if (has_forbidden_bits_set(uid)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	zms_ret = zms_write(&s_zms, zms_id, data, data_length);
	if (zms_ret == data_length) {
		psa_ret = PSA_SUCCESS;
	} else if (zms_ret == -ENOSPC) {
		psa_ret = PSA_ERROR_INSUFFICIENT_STORAGE;
	} else {
		psa_ret = PSA_ERROR_STORAGE_FAILURE;
	}
	LOG_DBG("%s 0x%x with %zu bytes. (%zd)", (psa_ret == PSA_SUCCESS) ?
		"Wrote" : "Failed to write", zms_id, data_length, zms_ret);
	return psa_ret;
}

psa_status_t secure_storage_its_store_get(secure_storage_its_uid_t uid, size_t data_size,
					  void *data, size_t *data_length)
{
	psa_status_t psa_ret;
	ssize_t zms_ret;
	const uint32_t zms_id = zms_id_from(uid);

	if (has_forbidden_bits_set(uid)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	zms_ret = zms_read(&s_zms, zms_id, data, data_size);
	if (zms_ret > 0) {
		*data_length = zms_ret;
		psa_ret = PSA_SUCCESS;
	} else if (zms_ret == -ENOENT) {
		psa_ret = PSA_ERROR_DOES_NOT_EXIST;
	} else {
		psa_ret = PSA_ERROR_STORAGE_FAILURE;
	}
	LOG_DBG("%s 0x%x for up to %zu bytes. (%zd)", (psa_ret != PSA_ERROR_STORAGE_FAILURE) ?
		"Read" : "Failed to read", zms_id, data_size, zms_ret);
	return psa_ret;
}

psa_status_t secure_storage_its_store_remove(secure_storage_its_uid_t uid)
{
	int zms_ret;
	const uint32_t zms_id = zms_id_from(uid);

	if (has_forbidden_bits_set(uid)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	zms_ret = zms_delete(&s_zms, zms_id);
	LOG_DBG("%s 0x%x. (%d)", zms_ret ? "Failed to delete" : "Deleted", zms_id, zms_ret);
	BUILD_ASSERT(PSA_SUCCESS == 0);
	return zms_ret;
}
