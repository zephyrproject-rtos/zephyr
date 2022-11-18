/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/grp/nvs_mgmt/nvs_mgmt.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#if defined(CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS)
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#endif

#define LOG_LEVEL CONFIG_MCUMGR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nvs_mgmt);

static struct nvs_fs fs = {
	.flash_device = FIXED_PARTITION_DEVICE(storage_partition),
	.offset = FIXED_PARTITION_OFFSET(storage_partition),
};

/**
 * Command handler: nvs read
 */
static int nvs_mgmt_read(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok;
	size_t decoded;
	uint32_t id = ULONG_MAX;
	uint32_t history = 0;

	struct zcbor_map_decode_key_val nvs_read_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(id, zcbor_uint32_decode, &id),
		ZCBOR_MAP_DECODE_KEY_VAL(history, zcbor_uint32_decode, &history),
	};

	ok = zcbor_map_decode_bulk(zsd, nvs_read_decode,
		ARRAY_SIZE(nvs_read_decode), &decoded) == 0;

	if (!ok || id > UINT_MAX || history > UINT_MAX) {
		return MGMT_ERR_EINVAL;
	}

	/* Read the NVS key */
uint8_t data[64];
	ssize_t rc = nvs_read_hist(&fs, (uint16_t)id, data, sizeof(data), (uint16_t)history);

	if (rc < 0) {
		return MGMT_ERR_ENOENT;
	}

	ok = zcbor_tstr_put_lit(zse, "ret")	&&
	zcbor_int32_put(zse, rc) 		&&
	zcbor_tstr_put_lit(zse, "data")		&&
	zcbor_tstr_encode_ptr(zse, data, rc);

	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	return MGMT_ERR_EOK;
}

/**
 * Command handler: nvs write
 */
static int nvs_mgmt_write(struct smp_streamer *ctxt)
{
	ssize_t rc;
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok;
	struct zcbor_string data = { 0 };
	size_t decoded;
	uint32_t id;

	struct zcbor_map_decode_key_val nvs_write_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(id, zcbor_uint32_decode, &id),
		ZCBOR_MAP_DECODE_KEY_VAL(data, zcbor_bstr_decode, &data),
	};

	ok = zcbor_map_decode_bulk(zsd, nvs_write_decode,
		ARRAY_SIZE(nvs_write_decode), &decoded) == 0;

	if (!ok || id > UINT_MAX) {
		return MGMT_ERR_EINVAL;
	}

	rc = nvs_write(&fs, (uint16_t)id, data.value, data.len);

	if (rc < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	ok = zcbor_tstr_put_lit(zse, "ret")	&&
	zcbor_int32_put(zse, rc);

	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	return MGMT_ERR_EOK;
}

/**
 * Command handler: nvs delete
 */
static int nvs_mgmt_delete(struct smp_streamer *ctxt)
{
	zcbor_state_t *zsd = ctxt->reader->zs;
	int rc;
	bool ok;
	size_t decoded;
	uint32_t id;

	struct zcbor_map_decode_key_val nvs_delete_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(id, zcbor_uint32_decode, &id),
	};

	ok = zcbor_map_decode_bulk(zsd, nvs_delete_decode,
		ARRAY_SIZE(nvs_delete_decode), &decoded) == 0;

	if (!ok || id > UINT_MAX) {
		return MGMT_ERR_EINVAL;
	}

	rc = nvs_delete(&fs, (uint16_t)id);

	if (rc < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return MGMT_ERR_EOK;
}

/**
 * Command handler: nvs free space
 */
static int nvs_mgmt_free_space(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	ssize_t size;
	bool ok;

	size = nvs_calc_free_space(&fs);

	if (size < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	ok = zcbor_tstr_put_lit(zse, "size")	&&
	zcbor_int32_put(zse, size);

	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	return MGMT_ERR_EOK;
}

/**
 * Command handler: nvs clear
 */
static int nvs_mgmt_clear(struct smp_streamer *ctxt)
{
	int rc;

	rc = nvs_clear(&fs);

	if (rc < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return MGMT_ERR_EOK;
}

static const struct mgmt_handler nvs_mgmt_handlers[] = {
	[NVS_MGMT_ID_READ_WRITE] = {
		.mh_read = nvs_mgmt_read,
		.mh_write = nvs_mgmt_write,
	},
	[NVS_MGMT_ID_DELETE] = {
		.mh_read = NULL,
		.mh_write = nvs_mgmt_delete,
	},
	[NVS_MGMT_ID_FREE_SPACE] = {
		.mh_read = nvs_mgmt_free_space,
		.mh_write = NULL,
	},
	[NVS_MGMT_ID_CLEAR] = {
		.mh_read = NULL,
		.mh_write = nvs_mgmt_clear,
	},
};

#define NVS_MGMT_HANDLER_CNT ARRAY_SIZE(nvs_mgmt_handlers)

static struct mgmt_group nvs_mgmt_group = {
	.mg_handlers = nvs_mgmt_handlers,
	.mg_handlers_count = NVS_MGMT_HANDLER_CNT,
	.mg_group_id = MGMT_GROUP_ID_NVS,
};

void nvs_mgmt_register_group(void)
{
	struct flash_pages_info flash_info;

	if (!device_is_ready(fs.flash_device)) {
		LOG_ERR("Storage partition not ready");
		return;
	}

	if (flash_get_page_info_by_offs(fs.flash_device, fs.offset, &flash_info)) {
		LOG_ERR("Storage partition information fetch failed");
		return;
	}

	fs.sector_size = flash_info.size;
	fs.sector_count = FIXED_PARTITION_SIZE(storage_partition) / fs.sector_size;

	if (nvs_mount(&fs)) {
		LOG_ERR("Could not mount storage partition");
		return;
	}

	mgmt_register_group(&nvs_mgmt_group);
}
