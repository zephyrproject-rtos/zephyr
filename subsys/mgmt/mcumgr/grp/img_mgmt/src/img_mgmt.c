/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/image.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <mgmt/mcumgr/grp/img_mgmt/img_mgmt_config.h>
#include <mgmt/mcumgr/grp/img_mgmt/img_mgmt_impl.h>
#include <mgmt/mcumgr/grp/img_mgmt/img_mgmt_priv.h>

#ifdef CONFIG_IMG_ENABLE_IMAGE_CHECK
#include <zephyr/dfu/flash_img.h>
#endif

#ifdef CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#endif

LOG_MODULE_REGISTER(mcumgr_img_grp, CONFIG_MCUMGR_GRP_IMG_LOG_LEVEL);

struct img_mgmt_state g_img_mgmt_state;

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
#endif

/**
 * Finds the TLVs in the specified image slot, if any.
 */
static int
img_mgmt_find_tlvs(int slot, size_t *start_off, size_t *end_off,
				   uint16_t magic)
{
	struct image_tlv_info tlv_info;
	int rc;

	rc = img_mgmt_read(slot, *start_off, &tlv_info, sizeof(tlv_info));
	if (rc != 0) {
		/* Read error. */
		return MGMT_ERR_EUNKNOWN;
	}

	if (tlv_info.it_magic != magic) {
		/* No TLVs. */
		return MGMT_ERR_ENOENT;
	}

	*start_off += sizeof(tlv_info);
	*end_off = *start_off + tlv_info.it_tlv_tot;

	return 0;
}

/*
 * Reads the version and build hash from the specified image slot.
 */
int
img_mgmt_read_info(int image_slot, struct image_version *ver, uint8_t *hash,
				   uint32_t *flags)
{

#ifdef CONFIG_MCUMGR_GRP_IMG_DUMMY_HDR
	uint8_t dummy_hash[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x00, 0x11, 0x22,
				0x33, 0x44, 0x55, 0x66, 0x77};

	if (!hash && !ver && !flags) {
		return 0;
	}

	if (hash) {
		memcpy(hash, dummy_hash, IMG_MGMT_HASH_LEN);
	}

	if (ver) {
		memset(ver, 0xff, sizeof(*ver));
	}

	if (flags) {
		*flags = 0;
	}

	return 0;
#endif

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
		return MGMT_ERR_EUNKNOWN;
	}

	rc = img_mgmt_read(image_slot, 0, &hdr, sizeof(hdr));
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
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
		return MGMT_ERR_ENOENT;
	} else {
		return MGMT_ERR_EUNKNOWN;
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
		return MGMT_ERR_EUNKNOWN;
	}

	hash_found = false;
	while (data_off + sizeof(tlv) <= data_end) {
		rc = img_mgmt_read(image_slot, data_off, &tlv, sizeof(tlv));
		if (rc != 0) {
			return MGMT_ERR_EUNKNOWN;
		}
		if (tlv.it_type == 0xff && tlv.it_len == 0xffff) {
			return MGMT_ERR_EUNKNOWN;
		}
		if (tlv.it_type != IMAGE_TLV_SHA256 || tlv.it_len != IMAGE_HASH_LEN) {
			/* Non-hash TLV.  Skip it. */
			data_off += sizeof(tlv) + tlv.it_len;
			continue;
		}

		if (hash_found) {
			/* More than one hash. */
			return MGMT_ERR_EUNKNOWN;
		}
		hash_found = true;

		data_off += sizeof(tlv);
		if (hash != NULL) {
			if (data_off + IMAGE_HASH_LEN > data_end) {
				return MGMT_ERR_EUNKNOWN;
			}
			rc = img_mgmt_read(image_slot, data_off, hash,
									IMAGE_HASH_LEN);
			if (rc != 0) {
				return MGMT_ERR_EUNKNOWN;
			}
		}
	}

	if (!hash_found) {
		return MGMT_ERR_EUNKNOWN;
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
void img_mgmt_reset_upload(void)
{
	memset(&g_img_mgmt_state, 0, sizeof(g_img_mgmt_state));
	g_img_mgmt_state.area_id = -1;
}

/**
 * Command handler: image erase
 */
static int
img_mgmt_erase(struct smp_streamer *ctxt)
{
	struct image_version ver;
	int rc;
	zcbor_state_t *zsd = ctxt->reader->zs;
	zcbor_state_t *zse = ctxt->writer->zs;
	bool ok;
	uint32_t slot = 1;
	size_t decoded = 0;

	struct zcbor_map_decode_key_val image_erase_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("slot", zcbor_uint32_decode, &slot),
	};

	ok = zcbor_map_decode_bulk(zsd, image_erase_decode,
		ARRAY_SIZE(image_erase_decode), &decoded) == 0;

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	/*
	 * First check if image info is valid.
	 * This check is done incase the flash area has a corrupted image.
	 */
	rc = img_mgmt_read_info(slot, &ver, NULL, NULL);

	if (rc == 0) {
		/* Image info is valid. */
		if (img_mgmt_slot_in_use(slot)) {
			/* No free slot. */
			return MGMT_ERR_EBADSTATE;
		}
	}

	rc = img_mgmt_erase_slot(slot);
	img_mgmt_reset_upload();

	if (rc != 0) {
#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
		(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED, NULL, 0);
#endif
		return rc;
	}

	if (IS_ENABLED(CONFIG_MCUMGR_SMP_LEGACY_RC_BEHAVIOUR) && zcbor_tstr_put_lit(zse, "rc") &&
	    zcbor_int32_put(zse, 0)) {
		return MGMT_ERR_EOK;
	}

	return MGMT_ERR_EMSGSIZE;
}

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
	zcbor_state_t *zse = ctxt->writer->zs;
	bool data_match = false;
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

	/* Determine what actions to take as a result of this request. */
	rc = img_mgmt_upload_inspect(&req, &action);
	if (rc != 0) {
#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
		(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED, NULL, 0);
#endif

		MGMT_CTXT_SET_RC_RSN(ctxt, IMG_MGMT_UPLOAD_ACTION_RC_RSN(&action));
		LOG_ERR("Image upload inspect failed: %d", rc);
		return rc;
	}

	if (!action.proceed) {
		/* Request specifies incorrect offset.  Respond with a success code and
		 * the correct offset.
		 */
		return img_mgmt_upload_good_rsp(ctxt);
	}

#if defined(CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK)
	/* Request is valid.  Give the application a chance to reject this upload
	 * request.
	 */
	rc = mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK, &upload_check_data,
				  sizeof(upload_check_data));

	if (rc != MGMT_ERR_EOK) {
		IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action, img_mgmt_err_str_app_reject);
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
		(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_STARTED, NULL, 0);
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
		/* Check if the existing image hash matches the hash of the underlying data */
		fic.match = g_img_mgmt_state.data_sha;
		fic.clen = g_img_mgmt_state.size;

		if (flash_img_check(&ctx, &fic, g_img_mgmt_state.area_id) == 0) {
			/* Underlying data already matches, no need to upload any more, set offset
			 * to image size so client knows upload has finished.
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
#endif

#ifndef CONFIG_IMG_ERASE_PROGRESSIVELY
		/* erase the entire req.size all at once */
		if (action.erase) {
			rc = img_mgmt_erase_image_data(0, req.size);
			if (rc != 0) {
				IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action,
					img_mgmt_err_str_flash_erase_failed);
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
			(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_PENDING, NULL, 0);
#endif
		}
	}
end:

	img_mgmt_upload_log(req.off == 0, g_img_mgmt_state.off == g_img_mgmt_state.size, rc);

#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
	(void)mgmt_callback_notify(MGMT_EVT_OP_CMD_STATUS, &cmd_status_arg,
				   sizeof(cmd_status_arg));
#endif

	if (rc != 0) {
#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
		(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED, NULL, 0);
#endif

		img_mgmt_reset_upload();

		return rc;
	}

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

	return rc;
}

int
img_mgmt_my_version(struct image_version *ver)
{
	return img_mgmt_read_info(IMG_MGMT_BOOT_CURR_SLOT, ver, NULL, NULL);
}

static const struct mgmt_handler img_mgmt_handlers[] = {
	[IMG_MGMT_ID_STATE] = {
		.mh_read = img_mgmt_state_read,
		.mh_write = img_mgmt_state_write,
	},
	[IMG_MGMT_ID_UPLOAD] = {
		.mh_read = NULL,
		.mh_write = img_mgmt_upload
	},
	[IMG_MGMT_ID_ERASE] = {
		.mh_read = NULL,
		.mh_write = img_mgmt_erase
	},
};

static const struct mgmt_handler img_mgmt_handlers[];

#define IMG_MGMT_HANDLER_CNT ARRAY_SIZE(img_mgmt_handlers)

static struct mgmt_group img_mgmt_group = {
	.mg_handlers = (struct mgmt_handler *)img_mgmt_handlers,
	.mg_handlers_count = IMG_MGMT_HANDLER_CNT,
	.mg_group_id = MGMT_GROUP_ID_IMAGE,
};

static void img_mgmt_register_group(void)
{
	mgmt_register_group(&img_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(img_mgmt, img_mgmt_register_group);
