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
		LOG_ERR("failed to open flash area");
	} else {
		if (flash_area_get_device(fa) == NULL ||
		    flash_area_erase(fa, 0, fa->fa_size) < 0) {
			LOG_ERR("failed to erase flash area");
		}
		flash_area_close(fa);
	}

	return rc;
}

static int storage_erase_handler(struct smp_streamer *ctxt)
{
	int rc = storage_erase();

	/* No point to self encode "rc" here, the SMP can do that for us */
	/* TODO: Decent error reporting for subsystems instead of using the
	 * "rc" from SMP.
	 */
	return rc;
}

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
};

static void zephyr_basic_mgmt_init(void)
{
	mgmt_register_group(&zephyr_basic_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(zephyr_basic_mgmt, zephyr_basic_mgmt_init);
