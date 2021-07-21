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
#include <sys/util_macro.h>
#include "bootutil/image.h"
#include "bootutil/bootutil_public.h"

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


#if defined(CONFIG_MCUMGR_GRP_BASIC_CMD_IMAGE_LIST)
static int image_status(int id, char *buffer, ssize_t len)
{
	const struct flash_area *fa;
	struct image_header hdr;
	struct boot_swap_state bss;
	int rc = flash_area_open(id, &fa);

	if (rc < 0) {
		LOG_ERR("Flash area %d open failed", id);
		return -1;
	}

	rc = flash_area_read(fa, 0, &hdr, sizeof(hdr));
	if (rc < 0) {
		flash_area_close(fa);
		LOG_ERR("Flash area %d read failed", id);
		return -1;
	}

	rc = boot_read_swap_state(fa, &bss);
	flash_area_close(fa);
	if (rc < 0) {
		LOG_ERR("Boot swap state %d read failed", id);
		return -1;
	}
	if (hdr.ih_magic == IMAGE_MAGIC) {
		snprintf(buffer, len, "ver=%d.%d.%d.%d%s", hdr.ih_ver.iv_major,
			 hdr.ih_ver.iv_minor, hdr.ih_ver.iv_revision,
			 hdr.ih_ver.iv_build_num,
			 bss.copy_done == BOOT_FLAG_SET ? ",copy_done" : "");
		return 1;
	}
	/* Not an application, not an error */
	return 0;
}

/* Only defined images are probed */
#define IMAGE_LIST_ITEM(n, node_label)					\
	COND_CODE_1(DT_HAS_FIXED_PARTITION_LABEL(node_label),		\
		({							\
			.num = n,					\
			.fa_id = FLASH_AREA_ID(node_label),		\
		},), ())

static int image_list(struct mgmt_ctxt *ctxt)
{
	static const struct {
		int num; int fa_id;
	} images[] = {
		IMAGE_LIST_ITEM(1, image_0)
		IMAGE_LIST_ITEM(2, image_1)
		IMAGE_LIST_ITEM(3, image_2)
		IMAGE_LIST_ITEM(4, image_3)
	};
	CborError cbor_err = 0;
	char buffer[64];	/* Buffer should fit version and flags */

	for (int i = 0; cbor_err == 0 && i < ARRAY_SIZE(images); i++) {
		if (image_status(images[i].num, buffer, sizeof(buffer)) > 0) {
			cbor_err |= cbor_encode_int(&ctxt->encoder, images[i].num);
			cbor_err |= cbor_encode_text_stringz(&ctxt->encoder, buffer);
		}
	}

	return cbor_err;
}
static int image_list_handler(struct mgmt_ctxt *ctxt)
{
	CborError cbor_err = 0;
	int rc = image_list(ctxt);

	cbor_err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
	cbor_err |= cbor_encode_int(&ctxt->encoder, rc);
	if (cbor_err != 0) {
		return MGMT_ERR_ENOMEM;
	}

	return MGMT_ERR_EOK;
}
#endif

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
#if defined(CONFIG_MCUMGR_GRP_BASIC_CMD_IMAGE_LIST)
	[ZEPHYR_MGMT_GRP_BASIC_CMD_IMAGE_LIST] = {
		.mh_read = image_list_handler,
		.mh_write = NULL,
	},
#endif
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
