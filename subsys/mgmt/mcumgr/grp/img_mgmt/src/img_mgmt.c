/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <mgmt/mcumgr/grp/img_mgmt/img_mgmt_priv.h>

#ifdef CONFIG_IMG_ENABLE_IMAGE_CHECK
#include <zephyr/dfu/flash_img.h>
#endif

#ifdef CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#endif

#ifndef CONFIG_FLASH_LOAD_OFFSET
#error MCUmgr requires application to be built with CONFIG_FLASH_LOAD_OFFSET set \
	to be able to figure out application running slot.
#endif

#define FIXED_PARTITION_IS_RUNNING_APP_PARTITION(label)	\
	 (FIXED_PARTITION_OFFSET(label) == CONFIG_FLASH_LOAD_OFFSET)

BUILD_ASSERT(sizeof(struct image_header) == IMAGE_HEADER_SIZE,
	     "struct image_header not required size");

#if CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER >= 2
#if FIXED_PARTITION_EXISTS(slot0_ns_partition) &&			\
	FIXED_PARTITION_IS_RUNNING_APP_PARTITION(slot0_ns_partition)
#define ACTIVE_IMAGE_IS 0
#elif FIXED_PARTITION_EXISTS(slot0_partition) &&			\
	FIXED_PARTITION_IS_RUNNING_APP_PARTITION(slot0_partition)
#define ACTIVE_IMAGE_IS 0
#elif FIXED_PARTITION_EXISTS(slot1_partition) &&			\
	FIXED_PARTITION_IS_RUNNING_APP_PARTITION(slot1_partition)
#define ACTIVE_IMAGE_IS 0
#elif FIXED_PARTITION_EXISTS(slot2_partition) &&			\
	FIXED_PARTITION_IS_RUNNING_APP_PARTITION(slot2_partition)
#define ACTIVE_IMAGE_IS 1
#elif FIXED_PARTITION_EXISTS(slot3_partition) &&			\
	FIXED_PARTITION_IS_RUNNING_APP_PARTITION(slot3_partition)
#define ACTIVE_IMAGE_IS 1
#elif FIXED_PARTITION_EXISTS(slot4_partition) &&			\
	FIXED_PARTITION_IS_RUNNING_APP_PARTITION(slot4_partition)
#define ACTIVE_IMAGE_IS 2
#elif FIXED_PARTITION_EXISTS(slot5_partition) &&			\
	FIXED_PARTITION_IS_RUNNING_APP_PARTITION(slot5_partition)
#define ACTIVE_IMAGE_IS 2
#else
#define ACTIVE_IMAGE_IS 0
#endif
#else
#define ACTIVE_IMAGE_IS 0
#endif

#define SLOTS_PER_IMAGE 2

LOG_MODULE_REGISTER(mcumgr_img_grp, CONFIG_MCUMGR_GRP_IMG_LOG_LEVEL);

struct img_mgmt_state g_img_mgmt_state;

#ifdef CONFIG_MCUMGR_GRP_IMG_MUTEX
static K_MUTEX_DEFINE(img_mgmt_mutex);
#endif

#ifdef CONFIG_MCUMGR_GRP_IMG_VERBOSE_ERR
const char *img_mgmt_err_str_app_reject = "app reject";
const char *img_mgmt_err_str_hdr_malformed = "header malformed";
const char *img_mgmt_err_str_magic_mismatch = "magic mismatch";
const char *img_mgmt_err_str_no_slot = "no slot";
const char *img_mgmt_err_str_flash_open_failed = "fa open fail";
const char *img_mgmt_err_str_flash_erase_failed = "fa erase fail";
const char *img_mgmt_err_str_flash_write_failed = "fa write fail";
const char *img_mgmt_err_str_downgrade = "downgrade";
const char *img_mgmt_err_str_image_bad_flash_addr = "img addr mismatch";
const char *img_mgmt_err_str_image_too_large = "img too large";
const char *img_mgmt_err_str_data_overrun = "data overrun";
#endif

void img_mgmt_take_lock(void)
{
#ifdef CONFIG_MCUMGR_GRP_IMG_MUTEX
	k_mutex_lock(&img_mgmt_mutex, K_FOREVER);
#endif
}

void img_mgmt_release_lock(void)
{
#ifdef CONFIG_MCUMGR_GRP_IMG_MUTEX
	k_mutex_unlock(&img_mgmt_mutex);
#endif
}

#if defined(CONFIG_MCUMGR_GRP_IMG_SLOT_INFO_HOOKS)
static bool img_mgmt_reset_zse(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;

	/* Because there is already data in the buffer, it must be cleared first */
	net_buf_reset(ctxt->writer->nb);
	ctxt->writer->nb->len = sizeof(struct smp_hdr);
	zcbor_new_encode_state(zse, ARRAY_SIZE(ctxt->writer->zs),
			       ctxt->writer->nb->data + sizeof(struct smp_hdr),
			       net_buf_tailroom(ctxt->writer->nb), 0);

	return zcbor_map_start_encode(zse, CONFIG_MCUMGR_SMP_CBOR_MAX_MAIN_MAP_ENTRIES);
}

#if defined(CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_SYSBUILD)
static bool img_mgmt_slot_max_size(size_t *area_sizes, zcbor_state_t *zse)
{
	bool ok = true;

	if (area_sizes[0] > 0 && area_sizes[1] > 0) {
		/* Calculate maximum image size */
		size_t area_size_difference = (size_t)abs((ssize_t)area_sizes[1] -
							  (ssize_t)area_sizes[0]);

		if (CONFIG_MCUBOOT_UPDATE_FOOTER_SIZE >= area_size_difference) {
			ok = zcbor_tstr_put_lit(zse, "max_image_size") &&
			     zcbor_uint32_put(zse, (uint32_t)(area_sizes[0] -
					      CONFIG_MCUBOOT_UPDATE_FOOTER_SIZE));		}
	}

	return ok;
}
#elif defined(CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_BOOTLOADER_INFO)
static bool img_mgmt_slot_max_size(size_t *area_sizes, zcbor_state_t *zse)
{
	bool ok = true;
	int rc;
	int max_app_size;

	ARG_UNUSED(area_sizes);

	rc = blinfo_lookup(BLINFO_MAX_APPLICATION_SIZE, &max_app_size, sizeof(max_app_size))

	if (rc < 0) {
		LOG_ERR("Failed to lookup max application size: %d", rc);
	} else if (rc > 0) {
		ok = zcbor_tstr_put_lit(zse, "max_image_size") &&
		     zcbor_uint32_put(zse, (uint32_t)max_app_size);
	}

	return ok;
}
#endif
#endif

/**
 * Finds the TLVs in the specified image slot, if any.
 */
static int img_mgmt_find_tlvs(int slot, size_t *start_off, size_t *end_off, uint16_t magic)
{
	struct image_tlv_info tlv_info;
	int rc;

	rc = img_mgmt_read(slot, *start_off, &tlv_info, sizeof(tlv_info));
	if (rc != 0) {
		/* Read error. */
		return rc;
	}

	if (tlv_info.it_magic != magic) {
		/* No TLVs. */
		return IMG_MGMT_ERR_NO_TLVS;
	}

	*start_off += sizeof(tlv_info);
	*end_off = *start_off + tlv_info.it_tlv_tot;

	return IMG_MGMT_ERR_OK;
}

int img_mgmt_active_slot(int image)
{
	int slot = 0;

	/* Multi image does not support DirectXIP currently */
#if CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER > 1
	slot = (image << 1);
#else
	/* This covers single image, including DirectXiP */
	if (FIXED_PARTITION_IS_RUNNING_APP_PARTITION(slot1_partition)) {
		slot = 1;
	}
#endif
	LOG_DBG("(%d) => %d", image, slot);

	return slot;
}

int img_mgmt_active_image(void)
{
	return ACTIVE_IMAGE_IS;
}

/*
 * Reads the version and build hash from the specified image slot.
 */
int img_mgmt_read_info(int image_slot, struct image_version *ver, uint8_t *hash,
				   uint32_t *flags)
{
	struct image_header hdr;
	struct image_tlv tlv;
	size_t data_off;
	size_t data_end;
	bool hash_found;
	uint8_t erased_val;
	uint32_t erased_val_32;
	int rc;

	rc = img_mgmt_erased_val(image_slot, &erased_val);
	if (rc != 0) {
		return IMG_MGMT_ERR_FLASH_CONFIG_QUERY_FAIL;
	}

	rc = img_mgmt_read(image_slot, 0, &hdr, sizeof(hdr));
	if (rc != 0) {
		return rc;
	}

	if (ver != NULL) {
		memset(ver, erased_val, sizeof(*ver));
	}
	erased_val_32 = ERASED_VAL_32(erased_val);
	if (hdr.ih_magic == IMAGE_MAGIC) {
		if (ver != NULL) {
			memcpy(ver, &hdr.ih_ver, sizeof(*ver));
		}
	} else if (hdr.ih_magic == erased_val_32) {
		return IMG_MGMT_ERR_NO_IMAGE;
	} else {
		return IMG_MGMT_ERR_INVALID_IMAGE_HEADER_MAGIC;
	}

	if (flags != NULL) {
		*flags = hdr.ih_flags;
	}

	/* Read the image's TLVs. We first try to find the protected TLVs, if the protected
	 * TLV does not exist, we try to find non-protected TLV which also contains the hash
	 * TLV. All images are required to have a hash TLV.  If the hash is missing, the image
	 * is considered invalid.
	 */
	data_off = hdr.ih_hdr_size + hdr.ih_img_size;

	rc = img_mgmt_find_tlvs(image_slot, &data_off, &data_end, IMAGE_TLV_PROT_INFO_MAGIC);
	if (!rc) {
		/* The data offset should start after the header bytes after the end of
		 * the protected TLV, if one exists.
		 */
		data_off = data_end - sizeof(struct image_tlv_info);
	}

	rc = img_mgmt_find_tlvs(image_slot, &data_off, &data_end, IMAGE_TLV_INFO_MAGIC);
	if (rc != 0) {
		return IMG_MGMT_ERR_NO_TLVS;
	}

	hash_found = false;
	while (data_off + sizeof(tlv) <= data_end) {
		rc = img_mgmt_read(image_slot, data_off, &tlv, sizeof(tlv));
		if (rc != 0) {
			return rc;
		}
		if (tlv.it_type == 0xff && tlv.it_len == 0xffff) {
			return IMG_MGMT_ERR_INVALID_TLV;
		}
		if (tlv.it_type != IMAGE_TLV_SHA256 || tlv.it_len != IMAGE_HASH_LEN) {
			/* Non-hash TLV.  Skip it. */
			data_off += sizeof(tlv) + tlv.it_len;
			continue;
		}

		if (hash_found) {
			/* More than one hash. */
			return IMG_MGMT_ERR_TLV_MULTIPLE_HASHES_FOUND;
		}
		hash_found = true;

		data_off += sizeof(tlv);
		if (hash != NULL) {
			if (data_off + IMAGE_HASH_LEN > data_end) {
				return IMG_MGMT_ERR_TLV_INVALID_SIZE;
			}
			rc = img_mgmt_read(image_slot, data_off, hash, IMAGE_HASH_LEN);
			if (rc != 0) {
				return rc;
			}
		}
	}

	if (!hash_found) {
		return IMG_MGMT_ERR_HASH_NOT_FOUND;
	}

	return 0;
}

/*
 * Finds image given version number. Returns the slot number image is in,
 * or -1 if not found.
 */
int
img_mgmt_find_by_ver(struct image_version *find, uint8_t *hash)
{
	int i;
	struct image_version ver;

	for (i = 0; i < 2 * CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER; i++) {
		if (img_mgmt_read_info(i, &ver, hash, NULL) != 0) {
			continue;
		}
		if (!memcmp(find, &ver, sizeof(ver))) {
			return i;
		}
	}
	return -1;
}

/*
 * Finds image given hash of the image. Returns the slot number image is in,
 * or -1 if not found.
 */
int
img_mgmt_find_by_hash(uint8_t *find, struct image_version *ver)
{
	int i;
	uint8_t hash[IMAGE_HASH_LEN];

	for (i = 0; i < 2 * CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER; i++) {
		if (img_mgmt_read_info(i, ver, hash, NULL) != 0) {
			continue;
		}
		if (!memcmp(hash, find, IMAGE_HASH_LEN)) {
			return i;
		}
	}
	return -1;
}

/*
 * Resets upload status to defaults (no upload in progress)
 */
#ifdef CONFIG_MCUMGR_GRP_IMG_MUTEX
void img_mgmt_reset_upload(void)
#else
static void img_mgmt_reset_upload(void)
#endif
{
	img_mgmt_take_lock();
	memset(&g_img_mgmt_state, 0, sizeof(g_img_mgmt_state));
	g_img_mgmt_state.area_id = -1;
	img_mgmt_release_lock();
}

/**
 * Command handler: image erase
 */
static int
img_mgmt_erase(struct smp_streamer *ctxt)
{
	struct image_version ver;
	int rc;
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok;
	uint32_t slot = img_mgmt_get_opposite_slot(img_mgmt_active_slot(img_mgmt_active_image()));
	size_t decoded = 0;

	struct zcbor_map_decode_key_val image_erase_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("slot", zcbor_uint32_decode, &slot),
	};

	ok = zcbor_map_decode_bulk(zsd, image_erase_decode,
		ARRAY_SIZE(image_erase_decode), &decoded) == 0;

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	img_mgmt_take_lock();

	/*
	 * First check if image info is valid.
	 * This check is done incase the flash area has a corrupted image.
	 */
	rc = img_mgmt_read_info(slot, &ver, NULL, NULL);

	if (rc == 0) {
		/* Image info is valid. */
		if (img_mgmt_slot_in_use(slot)) {
			/* No free slot. */
			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_IMAGE,
					     IMG_MGMT_ERR_NO_FREE_SLOT);
			goto end;
		}
	}

	rc = img_mgmt_erase_slot(slot);
	img_mgmt_reset_upload();

	if (rc != 0) {
#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
		int32_t err_rc;
		uint16_t err_group;

		(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED, NULL, 0, &err_rc,
					   &err_group);
#endif
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_IMAGE, rc);
		goto end;
	}

	if (IS_ENABLED(CONFIG_MCUMGR_SMP_LEGACY_RC_BEHAVIOUR)) {
		if (!zcbor_tstr_put_lit(zse, "rc") || !zcbor_int32_put(zse, 0)) {
			img_mgmt_release_lock();
			return MGMT_ERR_EMSGSIZE;
		}
	}

end:
	img_mgmt_release_lock();

	return MGMT_ERR_EOK;
}

#if defined(CONFIG_MCUMGR_GRP_IMG_SLOT_INFO)
/**
 * Command handler: image slot info
 */
static int img_mgmt_slot_info(struct smp_streamer *ctxt)
{
	int rc;
	zcbor_state_t *zse = ctxt->writer->zs;
	bool ok;
	uint8_t i = 0;
	size_t area_sizes[SLOTS_PER_IMAGE];

#if defined(CONFIG_MCUMGR_GRP_IMG_SLOT_INFO_HOOKS)
	int32_t err_rc;
	uint16_t err_group;
	enum mgmt_cb_return status;
#endif

	img_mgmt_take_lock();

	ok = zcbor_tstr_put_lit(zse, "images") &&
	     zcbor_list_start_encode(zse, 10);

	while (i < CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER * SLOTS_PER_IMAGE) {
		const struct flash_area *fa;
		int area_id = img_mgmt_flash_area_id(i);

		if ((i % SLOTS_PER_IMAGE) == 0) {
			memset(area_sizes, 0, sizeof(area_sizes));

			ok = zcbor_map_start_encode(zse, 4) &&
			     zcbor_tstr_put_lit(zse, "image") &&
			     zcbor_uint32_put(zse, (uint32_t)(i / SLOTS_PER_IMAGE)) &&
			     zcbor_tstr_put_lit(zse, "slots") &&
			     zcbor_list_start_encode(zse, 4);

			if (!ok) {
				goto finish;
			}
		}

		ok = zcbor_map_start_encode(zse, 4) &&
		     zcbor_tstr_put_lit(zse, "slot") &&
		     zcbor_uint32_put(zse, (uint32_t)(i % SLOTS_PER_IMAGE));

		if (!ok) {
			goto finish;
		}

		rc = flash_area_open(area_id, &fa);

		if (rc) {
			/* Failed opening slot, mark as error */
			ok = zcbor_tstr_put_lit(zse, "rc") &&
			     zcbor_int32_put(zse, rc);

			LOG_ERR("Failed to open slot %d for information fetching: %d", area_id, rc);
		} else {
#if defined(CONFIG_MCUMGR_GRP_IMG_SLOT_INFO_HOOKS)
			struct img_mgmt_slot_info_slot slot_info_data = {
				.image = (i / SLOTS_PER_IMAGE),
				.slot = (i % SLOTS_PER_IMAGE),
				.fa = fa,
				.zse = zse,
			};
#endif

			if (sizeof(fa->fa_size) == sizeof(uint64_t)) {
				ok = zcbor_tstr_put_lit(zse, "size") &&
				     zcbor_uint64_put(zse, fa->fa_size);
			} else {
				ok = zcbor_tstr_put_lit(zse, "size") &&
				     zcbor_uint32_put(zse, fa->fa_size);
			}

			area_sizes[(i % SLOTS_PER_IMAGE)] = fa->fa_size;

			if (!ok) {
				goto finish;
			}

			/*
			 * Check if we support uploading to this slot and if so, return the
			 * image ID
			 */
#if defined(CONFIG_MCUMGR_GRP_IMG_DIRECT_UPLOAD)
			ok = zcbor_tstr_put_lit(zse, "upload_image_id") &&
			     zcbor_uint32_put(zse, (i + 1));
#else
			if (img_mgmt_active_slot((i / SLOTS_PER_IMAGE)) != i) {
				ok = zcbor_tstr_put_lit(zse, "upload_image_id") &&
				     zcbor_uint32_put(zse, (i / SLOTS_PER_IMAGE));
			}
#endif

			if (!ok) {
				goto finish;
			}

#if defined(CONFIG_MCUMGR_GRP_IMG_SLOT_INFO_HOOKS)
			status = mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_SLOT,
						      &slot_info_data, sizeof(slot_info_data),
						      &err_rc, &err_group);
#endif

			flash_area_close(fa);

#if defined(CONFIG_MCUMGR_GRP_IMG_SLOT_INFO_HOOKS)
			if (status != MGMT_CB_OK) {
				if (status == MGMT_CB_ERROR_RC) {
					img_mgmt_release_lock();
					return err_rc;
				}

				ok = img_mgmt_reset_zse(ctxt) &&
				     smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);

				goto finish;
			}
#endif
		}

		ok &= zcbor_map_end_encode(zse, 4);

		if (!ok) {
			goto finish;
		}

		if ((i % SLOTS_PER_IMAGE) == (SLOTS_PER_IMAGE - 1)) {
#if defined(CONFIG_MCUMGR_GRP_IMG_SLOT_INFO_HOOKS)
			struct img_mgmt_slot_info_image image_info_data = {
				.image = (i / SLOTS_PER_IMAGE),
				.zse = zse,
			};
#endif

			ok = zcbor_list_end_encode(zse, 4);

			if (!ok) {
				goto finish;
			}

#if defined(CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_SYSBUILD) || \
	defined(MCUMGR_GRP_IMG_TOO_LARGE_BOOTLOADER_INFO)
			ok = img_mgmt_slot_max_size(area_sizes, zse);

			if (!ok) {
				goto finish;
			}
#endif

#if defined(CONFIG_MCUMGR_GRP_IMG_SLOT_INFO_HOOKS)
			status = mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_IMAGE,
						      &image_info_data, sizeof(image_info_data),
						      &err_rc, &err_group);

			if (status != MGMT_CB_OK) {
				if (status == MGMT_CB_ERROR_RC) {
					img_mgmt_release_lock();
					return err_rc;
				}

				ok = img_mgmt_reset_zse(ctxt) &&
				     smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);

				goto finish;
			}
#endif

			ok = zcbor_map_end_encode(zse, 4);

			if (!ok) {
				goto finish;
			}
		}

		++i;
	}

	ok = zcbor_list_end_encode(zse, 10);

finish:
	img_mgmt_release_lock();

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}
#endif

static int
img_mgmt_upload_good_rsp(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	bool ok = true;

	if (IS_ENABLED(CONFIG_MCUMGR_SMP_LEGACY_RC_BEHAVIOUR)) {
		ok = zcbor_tstr_put_lit(zse, "rc")		&&
		     zcbor_int32_put(zse, MGMT_ERR_EOK);
	}

	ok = ok && zcbor_tstr_put_lit(zse, "off")		&&
		   zcbor_size_put(zse, g_img_mgmt_state.off);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}

/**
 * Logs an upload request if necessary.
 *
 * @param is_first	Whether the request includes the first chunk of the image.
 * @param is_last	Whether the request includes the last chunk of the image.
 * @param status	The result of processing the upload request (MGMT_ERR code).
 *
 * @return 0 on success; nonzero on failure.
 */
static int
img_mgmt_upload_log(bool is_first, bool is_last, int status)
{
	uint8_t hash[IMAGE_HASH_LEN];
	const uint8_t *hashp;
	int rc;

	if (is_last || status != 0) {
		/* Log the image hash if we know it. */
		rc = img_mgmt_read_info(1, NULL, hash, NULL);
		if (rc != 0) {
			hashp = NULL;
		} else {
			hashp = hash;
		}
	}

	return 0;
}

/**
 * Command handler: image upload
 */
static int
img_mgmt_upload(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok;
	size_t decoded = 0;
	struct img_mgmt_upload_req req = {
		.off = SIZE_MAX,
		.size = SIZE_MAX,
		.img_data = { 0 },
		.data_sha = { 0 },
		.upgrade = false,
		.image = 0,
	};
	int rc;
	struct img_mgmt_upload_action action;
	bool last = false;
	bool reset = false;

#ifdef CONFIG_IMG_ENABLE_IMAGE_CHECK
	bool data_match = false;
#endif

#if defined(CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK)
	enum mgmt_cb_return status;
#endif

#if defined(CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK) ||	\
defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS) ||		\
defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
	int32_t err_rc;
	uint16_t err_group;
#endif

	struct zcbor_map_decode_key_val image_upload_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("image", zcbor_uint32_decode, &req.image),
		ZCBOR_MAP_DECODE_KEY_DECODER("data", zcbor_bstr_decode, &req.img_data),
		ZCBOR_MAP_DECODE_KEY_DECODER("len", zcbor_size_decode, &req.size),
		ZCBOR_MAP_DECODE_KEY_DECODER("off", zcbor_size_decode, &req.off),
		ZCBOR_MAP_DECODE_KEY_DECODER("sha", zcbor_bstr_decode, &req.data_sha),
		ZCBOR_MAP_DECODE_KEY_DECODER("upgrade", zcbor_bool_decode, &req.upgrade)
	};

#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
	struct mgmt_evt_op_cmd_arg cmd_status_arg = {
		.group = MGMT_GROUP_ID_IMAGE,
		.id = IMG_MGMT_ID_UPLOAD,
	};
#endif

#if defined(CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK)
	struct img_mgmt_upload_check upload_check_data = {
		.action = &action,
		.req = &req,
	};
#endif

	ok = zcbor_map_decode_bulk(zsd, image_upload_decode,
		ARRAY_SIZE(image_upload_decode), &decoded) == 0;

	IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action, NULL);

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	img_mgmt_take_lock();

	/* Determine what actions to take as a result of this request. */
	rc = img_mgmt_upload_inspect(&req, &action);
	if (rc != 0) {
#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
		(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED, NULL, 0, &err_rc,
					   &err_group);
#endif

		MGMT_CTXT_SET_RC_RSN(ctxt, IMG_MGMT_UPLOAD_ACTION_RC_RSN(&action));
		LOG_ERR("Image upload inspect failed: %d", rc);
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_IMAGE, rc);
		goto end;
	}

	if (!action.proceed) {
		/* Request specifies incorrect offset.  Respond with a success code and
		 * the correct offset.
		 */
		rc = img_mgmt_upload_good_rsp(ctxt);
		img_mgmt_release_lock();
		return rc;
	}

#if defined(CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK)
	/* Request is valid.  Give the application a chance to reject this upload
	 * request.
	 */
	status = mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK, &upload_check_data,
				      sizeof(upload_check_data), &err_rc, &err_group);

	if (status != MGMT_CB_OK) {
		IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action, img_mgmt_err_str_app_reject);

		if (status == MGMT_CB_ERROR_RC) {
			rc = err_rc;
			ok = zcbor_tstr_put_lit(zse, "rc")	&&
			     zcbor_int32_put(zse, rc);
		} else {
			ok = smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);
		}

		goto end;
	}
#endif

	/* Remember flash area ID and image size for subsequent upload requests. */
	g_img_mgmt_state.area_id = action.area_id;
	g_img_mgmt_state.size = action.size;

	if (req.off == 0) {
		/*
		 * New upload.
		 */
#ifdef CONFIG_IMG_ENABLE_IMAGE_CHECK
		struct flash_img_context ctx;
		struct flash_img_check fic;
#endif

		g_img_mgmt_state.off = 0;

#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
		(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_STARTED, NULL, 0, &err_rc,
					   &err_group);
#endif

#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
		cmd_status_arg.status = IMG_MGMT_ID_UPLOAD_STATUS_START;
#endif

		/*
		 * We accept SHA trimmed to any length by client since it's up to client
		 * to make sure provided data are good enough to avoid collisions when
		 * resuming upload.
		 */
		g_img_mgmt_state.data_sha_len = req.data_sha.len;
		memcpy(g_img_mgmt_state.data_sha, req.data_sha.value, req.data_sha.len);
		memset(&g_img_mgmt_state.data_sha[req.data_sha.len], 0,
			   IMG_MGMT_DATA_SHA_LEN - req.data_sha.len);

#ifdef CONFIG_IMG_ENABLE_IMAGE_CHECK
		/* Check if the existing image hash matches the hash of the underlying data,
		 * this check can only be performed if the provided hash is a full SHA256 hash
		 * of the file that is being uploaded, do not attempt the check if the length
		 * of the provided hash is less.
		 */
		if (g_img_mgmt_state.data_sha_len == IMG_MGMT_DATA_SHA_LEN) {
			fic.match = g_img_mgmt_state.data_sha;
			fic.clen = g_img_mgmt_state.size;

			if (flash_img_check(&ctx, &fic, g_img_mgmt_state.area_id) == 0) {
				/* Underlying data already matches, no need to upload any more,
				 * set offset to image size so client knows upload has finished.
				 */
				g_img_mgmt_state.off = g_img_mgmt_state.size;
				reset = true;
				last = true;
				data_match = true;

#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
				cmd_status_arg.status = IMG_MGMT_ID_UPLOAD_STATUS_COMPLETE;
#endif

				goto end;
			}
		}
#endif

#ifndef CONFIG_IMG_ERASE_PROGRESSIVELY
		/* erase the entire req.size all at once */
		if (action.erase) {
			rc = img_mgmt_erase_image_data(0, req.size);
			if (rc != 0) {
				IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action,
					img_mgmt_err_str_flash_erase_failed);
				ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_IMAGE, rc);
				goto end;
			}
		}
#endif
	} else {
#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
		cmd_status_arg.status = IMG_MGMT_ID_UPLOAD_STATUS_ONGOING;
#endif
	}

	/* Write the image data to flash. */
	if (req.img_data.len != 0) {
		/* If this is the last chunk */
		if (g_img_mgmt_state.off + req.img_data.len == g_img_mgmt_state.size) {
			last = true;
		}

		rc = img_mgmt_write_image_data(req.off, req.img_data.value, action.write_bytes,
						    last);
		if (rc == 0) {
			g_img_mgmt_state.off += action.write_bytes;
		} else {
			/* Write failed, currently not able to recover from this */
#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
			cmd_status_arg.status = IMG_MGMT_ID_UPLOAD_STATUS_COMPLETE;
#endif

			IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action,
				img_mgmt_err_str_flash_write_failed);
			reset = true;
			IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action,
				img_mgmt_err_str_flash_write_failed);

			LOG_ERR("Irrecoverable error: flash write failed: %d", rc);

			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_IMAGE, rc);
			goto end;
		}

		if (g_img_mgmt_state.off == g_img_mgmt_state.size) {
			/* Done */
			reset = true;

#ifdef CONFIG_IMG_ENABLE_IMAGE_CHECK
			static struct flash_img_context ctx;

			if (flash_img_init_id(&ctx, g_img_mgmt_state.area_id) == 0) {
				struct flash_img_check fic = {
					.match = g_img_mgmt_state.data_sha,
					.clen = g_img_mgmt_state.size,
				};

				if (flash_img_check(&ctx, &fic, g_img_mgmt_state.area_id) == 0) {
					data_match = true;
				} else {
					LOG_ERR("Uploaded image sha256 hash verification failed");
				}
			} else {
				LOG_ERR("Uploaded image sha256 could not be checked");
			}
#endif

#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
			(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_PENDING, NULL, 0,
						   &err_rc, &err_group);
		} else {
			/* Notify that the write has completed */
			(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK_WRITE_COMPLETE,
						   NULL, 0, &err_rc, &err_group);
#endif
		}
	}
end:

	img_mgmt_upload_log(req.off == 0, g_img_mgmt_state.off == g_img_mgmt_state.size, rc);

#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
	(void)mgmt_callback_notify(MGMT_EVT_OP_CMD_STATUS, &cmd_status_arg,
				   sizeof(cmd_status_arg), &err_rc, &err_group);
#endif

	if (rc != 0) {
#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
		(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED, NULL, 0, &err_rc,
					   &err_group);
#endif

		img_mgmt_reset_upload();
	} else {
		rc = img_mgmt_upload_good_rsp(ctxt);

#ifdef CONFIG_IMG_ENABLE_IMAGE_CHECK
		if (last && rc == MGMT_ERR_EOK) {
			/* Append status to last packet */
			ok = zcbor_tstr_put_lit(zse, "match")	&&
			     zcbor_bool_put(zse, data_match);
		}
#endif

		if (reset) {
			/* Reset the upload state struct back to default */
			img_mgmt_reset_upload();
		}
	}

	img_mgmt_release_lock();

	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	return MGMT_ERR_EOK;
}

int img_mgmt_my_version(struct image_version *ver)
{
	return img_mgmt_read_info(img_mgmt_active_slot(img_mgmt_active_image()),
				  ver, NULL, NULL);
}

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/*
 * @brief	Translate IMG mgmt group error code into MCUmgr error code
 *
 * @param ret	#img_mgmt_err_code_t error code
 *
 * @return	#mcumgr_err_t error code
 */
static int img_mgmt_translate_error_code(uint16_t err)
{
	int rc;

	switch (err) {
	case IMG_MGMT_ERR_NO_IMAGE:
	case IMG_MGMT_ERR_NO_TLVS:
		rc = MGMT_ERR_ENOENT;
		break;

	case IMG_MGMT_ERR_NO_FREE_SLOT:
	case IMG_MGMT_ERR_CURRENT_VERSION_IS_NEWER:
	case IMG_MGMT_ERR_IMAGE_ALREADY_PENDING:
		rc = MGMT_ERR_EBADSTATE;
		break;

	case IMG_MGMT_ERR_NO_FREE_MEMORY:
		rc = MGMT_ERR_ENOMEM;
		break;

	case IMG_MGMT_ERR_INVALID_SLOT:
	case IMG_MGMT_ERR_INVALID_PAGE_OFFSET:
	case IMG_MGMT_ERR_INVALID_OFFSET:
	case IMG_MGMT_ERR_INVALID_LENGTH:
	case IMG_MGMT_ERR_INVALID_IMAGE_HEADER:
	case IMG_MGMT_ERR_INVALID_HASH:
	case IMG_MGMT_ERR_INVALID_FLASH_ADDRESS:
		rc = MGMT_ERR_EINVAL;
		break;

	case IMG_MGMT_ERR_FLASH_CONFIG_QUERY_FAIL:
	case IMG_MGMT_ERR_VERSION_GET_FAILED:
	case IMG_MGMT_ERR_TLV_MULTIPLE_HASHES_FOUND:
	case IMG_MGMT_ERR_TLV_INVALID_SIZE:
	case IMG_MGMT_ERR_HASH_NOT_FOUND:
	case IMG_MGMT_ERR_INVALID_TLV:
	case IMG_MGMT_ERR_FLASH_OPEN_FAILED:
	case IMG_MGMT_ERR_FLASH_READ_FAILED:
	case IMG_MGMT_ERR_FLASH_WRITE_FAILED:
	case IMG_MGMT_ERR_FLASH_ERASE_FAILED:
	case IMG_MGMT_ERR_FLASH_CONTEXT_ALREADY_SET:
	case IMG_MGMT_ERR_FLASH_CONTEXT_NOT_SET:
	case IMG_MGMT_ERR_FLASH_AREA_DEVICE_NULL:
	case IMG_MGMT_ERR_INVALID_IMAGE_HEADER_MAGIC:
	case IMG_MGMT_ERR_INVALID_IMAGE_VECTOR_TABLE:
	case IMG_MGMT_ERR_INVALID_IMAGE_TOO_LARGE:
	case IMG_MGMT_ERR_INVALID_IMAGE_DATA_OVERRUN:
	case IMG_MGMT_ERR_UNKNOWN:
	default:
		rc = MGMT_ERR_EUNKNOWN;
	}

	return rc;
}
#endif

static const struct mgmt_handler img_mgmt_handlers[] = {
	[IMG_MGMT_ID_STATE] = {
		.mh_read = img_mgmt_state_read,
#ifdef CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP
		.mh_write = NULL
#else
		.mh_write = img_mgmt_state_write,
#endif
	},
	[IMG_MGMT_ID_UPLOAD] = {
		.mh_read = NULL,
		.mh_write = img_mgmt_upload
	},
	[IMG_MGMT_ID_ERASE] = {
		.mh_read = NULL,
		.mh_write = img_mgmt_erase
	},
#if defined(CONFIG_MCUMGR_GRP_IMG_SLOT_INFO)
	[IMG_MGMT_ID_SLOT_INFO] = {
		.mh_read = img_mgmt_slot_info,
		.mh_write = NULL
	},
#endif
};

static const struct mgmt_handler img_mgmt_handlers[];

#define IMG_MGMT_HANDLER_CNT ARRAY_SIZE(img_mgmt_handlers)

static struct mgmt_group img_mgmt_group = {
	.mg_handlers = (struct mgmt_handler *)img_mgmt_handlers,
	.mg_handlers_count = IMG_MGMT_HANDLER_CNT,
	.mg_group_id = MGMT_GROUP_ID_IMAGE,
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	.mg_translate_error = img_mgmt_translate_error_code,
#endif
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_NAME
	.mg_group_name = "img mgmt",
#endif
};

static void img_mgmt_register_group(void)
{
	mgmt_register_group(&img_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(img_mgmt, img_mgmt_register_group);
