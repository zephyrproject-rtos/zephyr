/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include "cborattr/cborattr.h"
#include "mgmt/mgmt.h"

#include "img_mgmt/image.h"
#include "img_mgmt/img_mgmt.h"
#include "img_mgmt/img_mgmt_impl.h"
#include "img_mgmt_priv.h"
#include "img_mgmt/img_mgmt_config.h"

static void *img_mgmt_upload_arg;
static img_mgmt_upload_fn img_mgmt_upload_cb;

const struct img_mgmt_dfu_callbacks_t *img_mgmt_dfu_callbacks_fn;

struct img_mgmt_state g_img_mgmt_state;

#if IMG_MGMT_VERBOSE_ERR
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

#if IMG_MGMT_DUMMY_HDR
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

	for (i = 0; i < 2 * IMG_MGMT_UPDATABLE_IMAGE_NUMBER; i++) {
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

	for (i = 0; i < 2 * IMG_MGMT_UPDATABLE_IMAGE_NUMBER; i++) {
		if (img_mgmt_read_info(i, ver, hash, NULL) != 0) {
			continue;
		}
		if (!memcmp(hash, find, IMAGE_HASH_LEN)) {
			return i;
		}
	}
	return -1;
}

#if IMG_MGMT_VERBOSE_ERR
int
img_mgmt_error_rsp(struct mgmt_ctxt *ctxt, int rc, const char *rsn)
{
	/*
	 * This is an error response so returning a different error when failed to
	 * encode other error probably does not make much sense - just ignore errors
	 * here.
	 */
	cbor_encode_text_stringz(&ctxt->encoder, "rsn");
	cbor_encode_text_stringz(&ctxt->encoder, rsn);
	return rc;
}
#endif

/**
 * Command handler: image erase
 */
static int
img_mgmt_erase(struct mgmt_ctxt *ctxt)
{
	struct image_version ver;
	CborError err;
	int rc;

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

	if (!rc) {
		img_mgmt_dfu_stopped();
	}

	err = 0;
	err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
	err |= cbor_encode_int(&ctxt->encoder, rc);

	if (err != 0) {
		return MGMT_ERR_ENOMEM;
	}

	return 0;
}

static int
img_mgmt_upload_good_rsp(struct mgmt_ctxt *ctxt)
{
	CborError err = CborNoError;

	err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
	err |= cbor_encode_int(&ctxt->encoder, MGMT_ERR_EOK);
	err |= cbor_encode_text_stringz(&ctxt->encoder, "off");
	err |= cbor_encode_int(&ctxt->encoder, g_img_mgmt_state.off);

	if (err != 0) {
		return MGMT_ERR_ENOMEM;
	}

	return 0;
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
	struct img_mgmt_upload_req req = {
		.off = -1,
		.size = -1,
		.data_len = 0,
		.data_sha_len = 0,
		.upgrade = false,
		.image = 0,
	};

	const struct cbor_attr_t off_attr[] = {
		[0] = {
			.attribute = "image",
			.type = CborAttrUnsignedIntegerType,
			.addr.uinteger = &req.image,
			.nodefault = true
		},
		[1] = {
			.attribute = "data",
			.type = CborAttrByteStringType,
			.addr.bytestring.data = req.img_data,
			.addr.bytestring.len = &req.data_len,
			.len = sizeof(req.img_data)
		},
		[2] = {
			.attribute = "len",
			.type = CborAttrUnsignedIntegerType,
			.addr.uinteger = &req.size,
			.nodefault = true
		},
		[3] = {
			.attribute = "off",
			.type = CborAttrUnsignedIntegerType,
			.addr.uinteger = &req.off,
			.nodefault = true
		},
		[4] = {
			.attribute = "sha",
			.type = CborAttrByteStringType,
			.addr.bytestring.data = req.data_sha,
			.addr.bytestring.len = &req.data_sha_len,
			.len = sizeof(req.data_sha)
		},
		[5] = {
			.attribute = "upgrade",
			.type = CborAttrBooleanType,
			.addr.boolean = &req.upgrade,
			.dflt.boolean = false,
		},
		[6] = { 0 },
	};
	int rc;
	const char *errstr = NULL;
	struct img_mgmt_upload_action action;
	bool last = false;

	rc = cbor_read_object(&ctxt->it, off_attr);
	if (rc != 0) {
		return MGMT_ERR_EINVAL;
	}

	/* Determine what actions to take as a result of this request. */
	rc = img_mgmt_impl_upload_inspect(&req, &action, &errstr);
	if (rc != 0) {
		img_mgmt_dfu_stopped();
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
		rc = img_mgmt_upload_cb(req.off, action.size, img_mgmt_upload_arg);
		if (rc != 0) {
			errstr = img_mgmt_err_str_app_reject;
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
		g_img_mgmt_state.data_sha_len = req.data_sha_len;
		memcpy(g_img_mgmt_state.data_sha, req.data_sha, req.data_sha_len);
		memset(&g_img_mgmt_state.data_sha[req.data_sha_len], 0,
			   IMG_MGMT_DATA_SHA_LEN - req.data_sha_len);

#if IMG_MGMT_LAZY_ERASE
		/* setup for lazy sector by sector erase */
		g_img_mgmt_state.sector_id = -1;
		g_img_mgmt_state.sector_end = 0;
#else
		/* erase the entire req.size all at once */
		if (action.erase) {
			rc = img_mgmt_impl_erase_image_data(0, req.size);
			if (rc != 0) {
				rc = MGMT_ERR_EUNKNOWN;
				errstr = img_mgmt_err_str_flash_erase_failed;
				goto end;
			}
		}
#endif
	} else {
		cmd_status_arg.status = IMG_MGMT_ID_UPLOAD_STATUS_ONGOING;
	}

	/* Write the image data to flash. */
	if (req.data_len != 0) {
#if IMG_MGMT_LAZY_ERASE
		/* erase as we cross sector boundaries */
		if (img_mgmt_impl_erase_if_needed(req.off, action.write_bytes) != 0) {
			rc = MGMT_ERR_EUNKNOWN;
			errstr = img_mgmt_err_str_flash_erase_failed;
			goto end;
		}
#endif
		/* If this is the last chunk */
		if (g_img_mgmt_state.off + req.data_len == g_img_mgmt_state.size) {
			last = true;
		}

		rc = img_mgmt_impl_write_image_data(req.off, req.img_data, action.write_bytes,
						    last);
		if (rc != 0) {
			rc = MGMT_ERR_EUNKNOWN;
			errstr = img_mgmt_err_str_flash_write_failed;
			goto end;
		} else {
			g_img_mgmt_state.off += action.write_bytes;
			if (g_img_mgmt_state.off == g_img_mgmt_state.size) {
				/* Done */
				img_mgmt_dfu_pending();
				cmd_status_arg.status = IMG_MGMT_ID_UPLOAD_STATUS_COMPLETE;
				g_img_mgmt_state.area_id = -1;
			}
		}
	}
end:

	img_mgmt_upload_log(req.off == 0, g_img_mgmt_state.off == g_img_mgmt_state.size, rc);
	mgmt_evt(MGMT_EVT_OP_CMD_STATUS, MGMT_GROUP_ID_IMAGE, IMG_MGMT_ID_UPLOAD,
			 &cmd_status_arg);

	if (rc != 0) {
		img_mgmt_dfu_stopped();
		return img_mgmt_error_rsp(ctxt, rc, errstr);
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
img_mgmt_set_upload_cb(img_mgmt_upload_fn cb, void *arg)
{
	img_mgmt_upload_cb = cb;
	img_mgmt_upload_arg = arg;
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
