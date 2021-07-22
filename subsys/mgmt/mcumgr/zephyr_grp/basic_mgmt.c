/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>
#include <init.h>
#include <mgmt/mgmt.h>
#include <mgmt/mcumgr/zephyr_groups.h>
#include <storage/flash_map.h>

LOG_MODULE_REGISTER(mgmt_zephyr_basic, CONFIG_MGMT_SETTINGS_LOG_LEVEL);

#define STORAGE_MGMT_ID_ERASE 6

int storage_erase(void)
{
	const struct flash_area *fa;
	int rc = flash_area_open(FLASH_AREA_ID(storage), &fa);

	if (rc < 0) {
		LOG_ERR("failed to open flash area");
	} else {
		rc = flash_area_erase(fa, 0, FLASH_AREA_SIZE(storage));
		if (rc < 0) {
			LOG_ERR("failed to erase flash area");
		}
		flash_area_close(fa);
	}

	return rc;
}

static int storage_erase_handler(struct mgmt_ctxt *ctxt)
{
	CborError cbor_err = 0;
	int rc = storage_erase();

	cbor_err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
	cbor_err |= cbor_encode_int(&ctxt->encoder, rc);
	if (cbor_err != 0) {
		return MGMT_ERR_ENOMEM;
	}

	return MGMT_ERR_EOK;
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
