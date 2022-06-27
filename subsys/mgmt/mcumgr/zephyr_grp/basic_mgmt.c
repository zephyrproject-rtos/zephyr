/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>
#include <mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/zephyr_groups.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(mcumgr_zephyr_grp);

static int storage_erase(void)
{
	const struct flash_area *fa;
	int rc = flash_area_open(FLASH_AREA_ID(storage), &fa);

	if (rc < 0) {
		LOG_ERR("failed to open flash area");
	} else {
		if (flash_area_get_device(fa) == NULL ||
		    flash_area_erase(fa, 0, FLASH_AREA_SIZE(storage) < 0)) {
			LOG_ERR("failed to erase flash area");
		}
		flash_area_close(fa);
	}

	return rc;
}

static int storage_erase_handler(struct mgmt_ctxt *ctxt)
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

static int zephyr_basic_mgmt_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_INF("Registering Zephyr basic mgmt group");
	mgmt_register_group(&zephyr_basic_mgmt_group);
	return 0;
}

SYS_INIT(zephyr_basic_mgmt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
