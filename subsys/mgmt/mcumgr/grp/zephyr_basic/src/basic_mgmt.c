/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/grp/zephyr/zephyr_basic.h>

LOG_MODULE_REGISTER(mcumgr_zbasic_grp, CONFIG_MCUMGR_GRP_ZBASIC_LOG_LEVEL);

#define ERASE_TARGET		storage_partition
#define ERASE_TARGET_ID		FIXED_PARTITION_ID(ERASE_TARGET)

static int storage_erase(void)
{
	const struct flash_area *fa;
	int rc = flash_area_open(ERASE_TARGET_ID, &fa);

	if (rc < 0) {
		LOG_ERR("Failed to open flash area");
		rc = ZEPHYRBASIC_MGMT_ERR_FLASH_OPEN_FAILED;
	} else {
		if (flash_area_get_device(fa) == NULL) {
			LOG_ERR("Failed to get flash area device");
			rc = ZEPHYRBASIC_MGMT_ERR_FLASH_CONFIG_QUERY_FAIL;
		} else {
			rc = flash_area_erase(fa, 0, fa->fa_size);

			if (rc < 0) {
				LOG_ERR("Failed to erase flash area");
				rc = ZEPHYRBASIC_MGMT_ERR_FLASH_ERASE_FAILED;
			}
		}

		flash_area_close(fa);
	}

	return rc;
}

static int storage_erase_handler(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	int rc;
	bool ok = true;

	rc = storage_erase();

	if (rc != ZEPHYRBASIC_MGMT_ERR_OK) {
		ok = smp_add_cmd_err(zse, ZEPHYR_MGMT_GRP_BASIC, rc);
	}

	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	return MGMT_ERR_EOK;
}

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/*
 * @brief	Translate zephyr basic group error code into MCUmgr error code
 *
 * @param ret	#zephyr_basic_group_err_code_t error code
 *
 * @return	#mcumgr_err_t error code
 */
static int zephyr_basic_group_translate_error_code(uint16_t ret)
{
	int rc;

	switch (ret) {
	case ZEPHYRBASIC_MGMT_ERR_FLASH_OPEN_FAILED:
		rc = MGMT_ERR_ENOENT;
		break;

	case ZEPHYRBASIC_MGMT_ERR_FLASH_CONFIG_QUERY_FAIL:
	case ZEPHYRBASIC_MGMT_ERR_FLASH_ERASE_FAILED:
		rc = MGMT_ERR_EOK;
		break;

	default:
		rc = MGMT_ERR_EUNKNOWN;
	}

	return rc;
}
#endif

static const struct mgmt_handler zephyr_mgmt_basic_handlers[] = {
	[ZEPHYR_MGMT_GRP_BASIC_CMD_ERASE_STORAGE] = {
		.mh_read  = NULL,
		.mh_write = storage_erase_handler,
	},
};

static struct mgmt_group zephyr_basic_mgmt_group = {
	.mg_handlers = (struct mgmt_handler *)zephyr_mgmt_basic_handlers,
	.mg_handlers_count = ARRAY_SIZE(zephyr_mgmt_basic_handlers),
	.mg_group_id = (ZEPHYR_MGMT_GRP_BASIC),
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	.mg_translate_error = zephyr_basic_group_translate_error_code,
#endif
};

static void zephyr_basic_mgmt_init(void)
{
	mgmt_register_group(&zephyr_basic_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(zephyr_basic_mgmt, zephyr_basic_mgmt_init);
