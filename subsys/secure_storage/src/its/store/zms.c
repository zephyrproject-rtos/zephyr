/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/uid.h>
#include <zephyr/secure_storage/zms.h>
#include <zephyr/secure_storage/its/store.h>
#include <zephyr/logging/log.h>
#include <zephyr/kvss/zms.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_DECLARE(secure_storage_its, CONFIG_SECURE_STORAGE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_SECURE_STORAGE_ITS_STORE_ZMS_SECTOR_SIZE
	     > 2 * CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE);

#define PARTITION_DT_NODE DT_CHOSEN(zephyr_secure_storage_its_partition)

static struct zms_fs s_zms = {
	.flash_device = PARTITION_NODE_DEVICE(PARTITION_DT_NODE),
	.offset = PARTITION_NODE_OFFSET(PARTITION_DT_NODE),
	.sector_size = CONFIG_SECURE_STORAGE_ITS_STORE_ZMS_SECTOR_SIZE,
	.sector_count = PARTITION_NODE_SIZE(PARTITION_DT_NODE)/
			CONFIG_SECURE_STORAGE_ITS_STORE_ZMS_SECTOR_SIZE,
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

psa_status_t secure_storage_its_store_set(secure_storage_uid_t uid,
					  size_t data_length, const void *data)
{
	return secure_storage_store_set(&s_zms, uid, data_length, data);
}

psa_status_t secure_storage_its_store_get(secure_storage_uid_t uid, size_t data_size,
					  void *data, size_t *data_length)
{
	return secure_storage_store_get(&s_zms, uid, data_size, data, data_length);
}

psa_status_t secure_storage_its_store_remove(secure_storage_uid_t uid)
{
	return secure_storage_store_remove(&s_zms, uid);
}
