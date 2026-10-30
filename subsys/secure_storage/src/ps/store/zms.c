/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/ps/store.h>
#include <zephyr/logging/log.h>
#include <zephyr/kvss/zms.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_DECLARE(secure_storage_ps, CONFIG_SECURE_STORAGE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_SECURE_STORAGE_PS_STORE_ZMS_SECTOR_SIZE
	     > 2 * SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE);

#define PARTITION_DT_NODE DT_CHOSEN(zephyr_secure_storage_ps_partition)

#ifdef CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS
BUILD_ASSERT(!DT_SAME_NODE(DT_CHOSEN(zephyr_secure_storage_ps_partition),
			   DT_CHOSEN(zephyr_secure_storage_its_partition)),
	     "ITS and PS must use different storage partitions");
#endif /* CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS */

static struct zms_fs s_zms = {
	.flash_device = PARTITION_NODE_DEVICE(PARTITION_DT_NODE),
	.offset = PARTITION_NODE_OFFSET(PARTITION_DT_NODE),
	.sector_size = CONFIG_SECURE_STORAGE_PS_STORE_ZMS_SECTOR_SIZE,
	.sector_count = PARTITION_NODE_SIZE(PARTITION_DT_NODE) /
			CONFIG_SECURE_STORAGE_PS_STORE_ZMS_SECTOR_SIZE,
};

static int init_zms(void)
{
	int ret;

	ret = zms_mount(&s_zms);
	if (ret) {
		LOG_DBG("Failed. (%d)", ret);
	}
	return ret;
}
SYS_INIT(init_zms, APPLICATION, CONFIG_SECURE_STORAGE_INIT_PRIORITY);

#ifdef CONFIG_ZMS_ID_64BIT
#define ZMS_ID_FMT "%#llx"
#define ZMS_ID_ARG(zms_id) (unsigned long long)zms_id
#else
#define ZMS_ID_FMT "%#lx"
#define ZMS_ID_ARG(zms_id) (unsigned long)zms_id
#endif /* CONFIG_ZMS_ID_64BIT */

static zms_id_t zms_id_from(psa_storage_uid_t uid)
{
#if defined(CONFIG_SECURE_STORAGE_64_BIT_UID) && !defined(CONFIG_ZMS_ID_64BIT)
	if (uid & GENMASK64(63, 32)) {
		LOG_DBG("Refusing UID %#llx with upper bits set.", (unsigned long long)uid);
		return 0;
	}
#else
	BUILD_ASSERT(sizeof(uid) <= sizeof(zms_id_t));
#endif

	return (zms_id_t)uid;
}

psa_status_t secure_storage_ps_store_set(psa_storage_uid_t uid,
					  size_t data_length, const void *data)
{
	psa_status_t psa_ret;
	ssize_t zms_ret;
	const zms_id_t zms_id = zms_id_from(uid);

	if (zms_id == 0) {
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
	LOG_DBG("%s " ZMS_ID_FMT " with %zu bytes. (%zd)", (psa_ret == PSA_SUCCESS) ?
		"Wrote" : "Failed to write", ZMS_ID_ARG(zms_id), data_length, zms_ret);
	return psa_ret;
}

psa_status_t secure_storage_ps_store_get(psa_storage_uid_t uid, size_t data_size,
					  void *data, size_t *data_length)
{
	psa_status_t psa_ret;
	ssize_t zms_ret;
	const zms_id_t zms_id = zms_id_from(uid);

	if (zms_id == 0) {
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
	LOG_DBG("%s " ZMS_ID_FMT " for up to %zu bytes. (%zd)",
		(psa_ret != PSA_ERROR_STORAGE_FAILURE) ? "Read" : "Failed to read",
		ZMS_ID_ARG(zms_id), data_size, zms_ret);
	return psa_ret;
}

psa_status_t secure_storage_ps_store_remove(psa_storage_uid_t uid)
{
	int ret;
	const zms_id_t zms_id = zms_id_from(uid);

	if (zms_id == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = zms_delete(&s_zms, zms_id);
	LOG_DBG("%s " ZMS_ID_FMT ". (%d)", ret ? "Failed to delete" : "Deleted",
		ZMS_ID_ARG(zms_id), ret);

	return ret ? PSA_ERROR_STORAGE_FAILURE : PSA_SUCCESS;
}
