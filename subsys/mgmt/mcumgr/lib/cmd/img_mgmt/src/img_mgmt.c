/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <zephyr/toolchain.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <zephyr/mgmt/mcumgr/buf.h>
#include "zcbor_bulk/zcbor_bulk_priv.h"
#include "mgmt/mgmt.h"

#include "img_mgmt/image.h"
#include "img_mgmt/img_mgmt.h"
#include "img_mgmt/img_mgmt_impl.h"
#include "img_mgmt_priv.h"
#include "img_mgmt/img_mgmt_config.h"

static img_mgmt_upload_fn img_mgmt_upload_cb;

const struct img_mgmt_dfu_callbacks_t *img_mgmt_dfu_callbacks_fn;

struct img_mgmt_state g_img_mgmt_state;

#if SIZE_MAX == UINT32_MAX
#define zcbor_size_decode	zcbor_uint32_decode
#define zcbor_size_put		zcbor_uint32_put
#elif SIZE_MAX == UINT64_MAX
#define zcbor_size_decode	zcbor_uint64_decode
#define zcbor_size_put		zcbor_uint64_put
#else
#error "Unsupported size_t encoding"
#endif

#if CONFIG_IMG_MGMT_VERBOSE_ERR
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

	rc = img_mgmt_impl_read(slot, *start_off, &tlv_info, sizeof(tlv_info));
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

#if CONFIG_IMG_MGMT_DUMMY_HDR
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

	rc = img_mgmt_impl_erased_val(image_slot, &erased_val);
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	rc = img_mgmt_impl_read(image_slot, 0, &hdr, sizeof(hdr));
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
		rc = img_mgmt_impl_read(image_slot, data_off, &tlv, sizeof(tlv));
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
			rc = img_mgmt_impl_read(image_slot, data_off, hash,
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

	for (i = 0; i < 2 * CONFIG_IMG_MGMT_UPDATABLE_IMAGE_NUMBER; i++) {
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

	for (i = 0; i < 2 * CONFIG_IMG_MGMT_UPDATABLE_IMAGE_NUMBER; i++) {
		if (img_mgmt_read_info(i, ver, hash, NULL) != 0) {
			continue;
		}
		if (!memcmp(hash, find, IMAGE_HASH_LEN)) {
			return i;
		}
	}
	return -1;
}

/**
 * Command handler: image erase
 */
static int
img_mgmt_erase(struct mgmt_ctxt *ctxt)
{
	struct image_version ver;
	int rc;
	zcbor_state_t *zse = ctxt->cnbe->zs;
	bool ok;

	/*
	 * First check if image info is valid.
	 * This check is done incase the flash area has a corrupted image.
	 */
	rc = img_mgmt_read_info(1, &ver, NULL, NULL);

	if (rc == 0) {
		/* Image info is valid. */
		if (img_mgmt_slot_in_use(1)) {
			/* No free slot. */
			return MGMT_ERR_EBADSTATE;
		}
	}

	rc = img_mgmt_impl_erase_slot();

	if (rc != 0) {
		img_mgmt_dfu_stopped();
	}

	ok = zcbor_tstr_put_lit(zse, "rc")	&&
	     zcbor_int32_put(zse, rc);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}

static int
img_mgmt_upload_good_rsp(struct mgmt_ctxt *ctxt)
{
	zcbor_state_t *zse = ctxt->cnbe->zs;
	bool ok;

	ok = zcbor_tstr_put_lit(zse, "rc")			&&
	     zcbor_int32_put(zse, MGMT_ERR_EOK)			&&
	     zcbor_tstr_put_lit(zse, "off")			&&
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

	if (is_first) {
		return img_mgmt_impl_log_upload_start(status);
	}

	if (is_last || status != 0) {
		/* Log the image hash if we know it. */
		rc = img_mgmt_read_info(1, NULL, hash, NULL);
		if (rc != 0) {
			hashp = NULL;
		} else {
			hashp = hash;
		}

		return img_mgmt_impl_log_upload_done(status, hashp);
	}

	/* Nothing to log. */
	return 0;
}

/**
 * Command handler: image upload
 */
static int
img_mgmt_upload(struct mgmt_ctxt *ctxt)
{
	struct mgmt_evt_op_cmd_status_arg cmd_status_arg;
	zcbor_state_t *zsd = ctxt->cnbd->zs;
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

	struct zcbor_map_decode_key_val image_upload_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(image, zcbor_uint32_decode, &req.image),
		ZCBOR_MAP_DECODE_KEY_VAL(data, zcbor_bstr_decode, &req.img_data),
		ZCBOR_MAP_DECODE_KEY_VAL(len, zcbor_size_decode, &req.size),
		ZCBOR_MAP_DECODE_KEY_VAL(off, zcbor_size_decode, &req.off),
		ZCBOR_MAP_DECODE_KEY_VAL(sha, zcbor_bstr_decode, &req.data_sha),
		ZCBOR_MAP_DECODE_KEY_VAL(upgrade, zcbor_bool_decode, &req.upgrade)
	};

	ok = zcbor_map_decode_bulk(zsd, image_upload_decode,
		ARRAY_SIZE(image_upload_decode), &decoded) == 0;

	IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action, NULL);

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	/* Determine what actions to take as a result of this request. */
	rc = img_mgmt_impl_upload_inspect(&req, &action);
	if (rc != 0) {
		img_mgmt_dfu_stopped();
		MGMT_CTXT_SET_RC_RSN(ctxt, IMG_MGMT_UPLOAD_ACTION_RC_RSN(&action));
		return rc;
	}

	if (!action.proceed) {
		/* Request specifies incorrect offset.  Respond with a success code and
		 * the correct offset.
		 */
		return img_mgmt_upload_good_rsp(ctxt);
	}

	/* Request is valid.  Give the application a chance to reject this upload
	 * request.
	 */
	if (img_mgmt_upload_cb != NULL) {
		rc = img_mgmt_upload_cb(req, action);

		if (rc != 0) {
			IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action, img_mgmt_err_str_app_reject);
			goto end;
		}
	}

	/* Remember flash area ID and image size for subsequent upload requests. */
	g_img_mgmt_state.area_id = action.area_id;
	g_img_mgmt_state.size = action.size;

	if (req.off == 0) {
		/*
		 * New upload.
		 */
		g_img_mgmt_state.off = 0;

		img_mgmt_dfu_started();
		cmd_status_arg.status = IMG_MGMT_ID_UPLOAD_STATUS_START;

		/*
		 * We accept SHA trimmed to any length by client since it's up to client
		 * to make sure provided data are good enough to avoid collisions when
		 * resuming upload.
		 */
		g_img_mgmt_state.data_sha_len = req.data_sha.len;
		memcpy(g_img_mgmt_state.data_sha, req.data_sha.value, req.data_sha.len);
		memset(&g_img_mgmt_state.data_sha[req.data_sha.len], 0,
			   IMG_MGMT_DATA_SHA_LEN - req.data_sha.len);

#ifndef CONFIG_IMG_ERASE_PROGRESSIVELY
		/* erase the entire req.size all at once */
		if (action.erase) {
			rc = img_mgmt_impl_erase_image_data(0, req.size);
			if (rc != 0) {
				IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action,
					img_mgmt_err_str_flash_erase_failed);
				goto end;
			}
		}
#endif
	} else {
		cmd_status_arg.status = IMG_MGMT_ID_UPLOAD_STATUS_ONGOING;
	}

	/* Write the image data to flash. */
	if (req.img_data.len != 0) {
		/* If this is the last chunk */
		if (g_img_mgmt_state.off + req.img_data.len == g_img_mgmt_state.size) {
			last = true;
		}

		rc = img_mgmt_impl_write_image_data(req.off, req.img_data.value, action.write_bytes,
						    last);
		if (rc == 0) {
			g_img_mgmt_state.off += action.write_bytes;
		} else {
			/* Write failed, currently not able to recover from this */
			cmd_status_arg.status = IMG_MGMT_ID_UPLOAD_STATUS_COMPLETE;
			g_img_mgmt_state.area_id = -1;
			IMG_MGMT_UPLOAD_ACTION_SET_RC_RSN(&action,
				img_mgmt_err_str_flash_write_failed);
			goto end;
		}

		if (g_img_mgmt_state.off == g_img_mgmt_state.size) {
			/* Done */
			img_mgmt_dfu_pending();
			cmd_status_arg.status = IMG_MGMT_ID_UPLOAD_STATUS_COMPLETE;
			g_img_mgmt_state.area_id = -1;
		}
	}
end:

	img_mgmt_upload_log(req.off == 0, g_img_mgmt_state.off == g_img_mgmt_state.size, rc);
	mgmt_evt(MGMT_EVT_OP_CMD_STATUS, MGMT_GROUP_ID_IMAGE, IMG_MGMT_ID_UPLOAD,
			 &cmd_status_arg);

	if (rc != 0) {
		img_mgmt_dfu_stopped();
		return rc;
	}

	return img_mgmt_upload_good_rsp(ctxt);
}

void
img_mgmt_dfu_stopped(void)
{
	if (img_mgmt_dfu_callbacks_fn && img_mgmt_dfu_callbacks_fn->dfu_stopped_cb) {
		img_mgmt_dfu_callbacks_fn->dfu_stopped_cb();
	}
}

void
img_mgmt_dfu_started(void)
{
	if (img_mgmt_dfu_callbacks_fn && img_mgmt_dfu_callbacks_fn->dfu_started_cb) {
		img_mgmt_dfu_callbacks_fn->dfu_started_cb();
	}
}

void
img_mgmt_dfu_pending(void)
{
	if (img_mgmt_dfu_callbacks_fn && img_mgmt_dfu_callbacks_fn->dfu_pending_cb) {
		img_mgmt_dfu_callbacks_fn->dfu_pending_cb();
	}
}

void
img_mgmt_dfu_confirmed(void)
{
	if (img_mgmt_dfu_callbacks_fn && img_mgmt_dfu_callbacks_fn->dfu_confirmed_cb) {
		img_mgmt_dfu_callbacks_fn->dfu_confirmed_cb();
	}
}

void
img_mgmt_set_upload_cb(img_mgmt_upload_fn cb)
{
	img_mgmt_upload_cb = cb;
}

void
img_mgmt_register_callbacks(const struct img_mgmt_dfu_callbacks_t *cb_struct)
{
	img_mgmt_dfu_callbacks_fn = cb_struct;
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


void
img_mgmt_register_group(void)
{
	mgmt_register_group(&img_mgmt_group);
}

void
img_mgmt_unregister_group(void)
{
	mgmt_unregister_group(&img_mgmt_group);
}
