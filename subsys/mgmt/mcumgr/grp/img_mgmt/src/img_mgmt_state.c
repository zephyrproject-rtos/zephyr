/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <assert.h>
#include <string.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/image.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <mgmt/mcumgr/grp/img_mgmt/img_mgmt_config.h>
#include <mgmt/mcumgr/grp/img_mgmt/img_mgmt_priv.h>
#include <mgmt/mcumgr/grp/img_mgmt/img_mgmt_impl.h>

#ifdef CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#endif

/* The value here sets how many "characteristics" that describe image is
 * encoded into a map per each image (like bootable flags, and so on).
 * This value is only used for zcbor to predict map size and map encoding
 * and does not affect memory allocation.
 * In case when more "characteristics" are added to image map then
 * zcbor_map_end_encode may fail it this value does not get updated.
 */
#define MAX_IMG_CHARACTERISTICS 15

#ifndef CONFIG_MCUMGR_GRP_IMG_FRUGAL_LIST
#define ZCBOR_ENCODE_FLAG(zse, label, value)					\
		(zcbor_tstr_put_lit(zse, label) && zcbor_bool_put(zse, value))
#else
/* In "frugal" lists flags are added to response only when they evaluate to true */
/* Note that value is evaluated twice! */
#define ZCBOR_ENCODE_FLAG(zse, label, value)					\
		(!(value) ||							\
		 (zcbor_tstr_put_lit(zse, label) && zcbor_bool_put(zse, (value))))
#endif

/**
 * Collects information about the specified image slot.
 */
uint8_t
img_mgmt_state_flags(int query_slot)
{
	uint8_t flags;
	int swap_type;

	flags = 0;

	/* Determine if this is is pending or confirmed (only applicable for
	 * unified images and loaders.
	 */
	swap_type = img_mgmt_swap_type(query_slot);
	switch (swap_type) {
	case IMG_MGMT_SWAP_TYPE_NONE:
		if (query_slot == IMG_MGMT_BOOT_CURR_SLOT) {
			flags |= IMG_MGMT_STATE_F_CONFIRMED;
			flags |= IMG_MGMT_STATE_F_ACTIVE;
		}
		break;

	case IMG_MGMT_SWAP_TYPE_TEST:
		if (query_slot == IMG_MGMT_BOOT_CURR_SLOT) {
			flags |= IMG_MGMT_STATE_F_CONFIRMED;
		} else {
			flags |= IMG_MGMT_STATE_F_PENDING;
		}
		break;

	case IMG_MGMT_SWAP_TYPE_PERM:
		if (query_slot == IMG_MGMT_BOOT_CURR_SLOT) {
			flags |= IMG_MGMT_STATE_F_CONFIRMED;
		} else {
			flags |= IMG_MGMT_STATE_F_PENDING | IMG_MGMT_STATE_F_PERMANENT;
		}
		break;

	case IMG_MGMT_SWAP_TYPE_REVERT:
		if (query_slot == IMG_MGMT_BOOT_CURR_SLOT) {
			flags |= IMG_MGMT_STATE_F_ACTIVE;
		} else {
			flags |= IMG_MGMT_STATE_F_CONFIRMED;
		}
		break;
	}

	/* Slot 0 is always active. */
	/* XXX: The slot 0 assumption only holds when running from flash. */
	if (query_slot == IMG_MGMT_BOOT_CURR_SLOT) {
		flags |= IMG_MGMT_STATE_F_ACTIVE;
	}

	return flags;
}

/**
 * Indicates whether any image slot is pending (i.e., whether a test swap will
 * happen on the next reboot.
 */
int
img_mgmt_state_any_pending(void)
{
	return img_mgmt_state_flags(0) & IMG_MGMT_STATE_F_PENDING ||
		   img_mgmt_state_flags(1) & IMG_MGMT_STATE_F_PENDING;
}

/**
 * Indicates whether the specified slot has any flags.  If no flags are set,
 * the slot can be freely erased.
 */
int
img_mgmt_slot_in_use(int slot)
{
	uint8_t state_flags;

	state_flags = img_mgmt_state_flags(slot);
	return state_flags & IMG_MGMT_STATE_F_ACTIVE	   ||
		   state_flags & IMG_MGMT_STATE_F_CONFIRMED	||
		   state_flags & IMG_MGMT_STATE_F_PENDING;
}

/**
 * Sets the pending flag for the specified image slot.  That is, the system
 * will swap to the specified image on the next reboot.  If the permanent
 * argument is specified, the system doesn't require a confirm after the swap
 * occurs.
 */
int
img_mgmt_state_set_pending(int slot, int permanent)
{
	uint8_t hash[IMAGE_HASH_LEN];
	uint8_t state_flags;
	const uint8_t *hashp;
	int rc;

	state_flags = img_mgmt_state_flags(slot);

	rc = img_mgmt_write_pending(slot, permanent);
	if (rc != 0) {
		rc = MGMT_ERR_EUNKNOWN;
	}

done:
	/* Log the image hash if we know it. */
	if (img_mgmt_read_info(slot, NULL, hash, NULL)) {
		hashp = NULL;
	} else {
		hashp = hash;
	}

	return rc;
}

/**
 * Confirms the current image state.  Prevents a fallback from occurring on the
 * next reboot if the active image is currently being tested.
 */
int
img_mgmt_state_confirm(void)
{
	int rc;

	/* Confirm disallowed if a test is pending. */
	if (img_mgmt_state_any_pending()) {
		rc = MGMT_ERR_EBADSTATE;
		goto err;
	}

	rc = img_mgmt_write_confirmed();
	if (rc != 0) {
		rc = MGMT_ERR_EUNKNOWN;
	}

#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
	(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED, NULL, 0);
#endif

err:
	return 0;
}

/**
 * Command handler: image state read
 */
int
img_mgmt_state_read(struct smp_streamer *ctxt)
{
	char vers_str[IMG_MGMT_VER_MAX_STR_LEN];
	uint8_t hash[IMAGE_HASH_LEN]; /* SHA256 hash */
	struct image_version ver;
	uint32_t flags;
	uint8_t state_flags;
	int i;
	zcbor_state_t *zse = ctxt->writer->zs;
	bool ok;
	struct zcbor_string zhash = { .value = hash, .len = IMAGE_HASH_LEN };

	ok = zcbor_tstr_put_lit(zse, "images") &&
	     zcbor_list_start_encode(zse, 2 * CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER);

	for (i = 0; ok && i < 2 * CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER; i++) {
		int rc = img_mgmt_read_info(i, &ver, hash, &flags);
		if (rc != 0) {
			continue;
		}

		state_flags = img_mgmt_state_flags(i);

		ok = zcbor_map_start_encode(zse, MAX_IMG_CHARACTERISTICS)	&&
		     (CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER == 1	||
		      (zcbor_tstr_put_lit(zse, "image")			&&
		       zcbor_int32_put(zse, i >> 1)))				&&
		     zcbor_tstr_put_lit(zse, "slot")				&&
		     zcbor_int32_put(zse, i % 2)				&&
		     zcbor_tstr_put_lit(zse, "version");

		if (ok) {
			if (img_mgmt_ver_str(&ver, vers_str) < 0) {
				ok = zcbor_tstr_put_lit(zse, "<\?\?\?>");
			} else {
				vers_str[sizeof(vers_str) - 1] = '\0';
				ok = zcbor_tstr_put_term(zse, vers_str);
			}
		}

		ok = zcbor_tstr_put_term(zse, "hash")						&&
		     zcbor_bstr_encode(zse, &zhash)						&&
		     ZCBOR_ENCODE_FLAG(zse, "bootable", !(flags & IMAGE_F_NON_BOOTABLE))	&&
		     ZCBOR_ENCODE_FLAG(zse, "pending",
				       state_flags & IMG_MGMT_STATE_F_PENDING)			&&
		     ZCBOR_ENCODE_FLAG(zse, "confirmed",
				       state_flags & IMG_MGMT_STATE_F_CONFIRMED)		&&
		     ZCBOR_ENCODE_FLAG(zse, "active",
				       state_flags & IMG_MGMT_STATE_F_ACTIVE)			&&
		     ZCBOR_ENCODE_FLAG(zse, "permanent",
				       state_flags & IMG_MGMT_STATE_F_PERMANENT)		&&
		     zcbor_map_end_encode(zse, MAX_IMG_CHARACTERISTICS);
	}

	ok = ok && zcbor_list_end_encode(zse, 2 * CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER);
	/* splitStatus is always 0 so in frugal list it is not present at all */
	if (!IS_ENABLED(CONFIG_MCUMGR_GRP_IMG_FRUGAL_LIST) && ok) {
		ok = zcbor_tstr_put_lit(zse, "splitStatus") &&
		     zcbor_int32_put(zse, 0);
	}

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}

/**
 * Command handler: image state write
 */
int
img_mgmt_state_write(struct smp_streamer *ctxt)
{
	bool confirm = false;
	int slot;
	int rc;
	size_t decoded = 0;
	bool ok;
	struct zcbor_string zhash = { 0 };

	struct zcbor_map_decode_key_val image_list_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("hash", zcbor_bstr_decode, &zhash),
		ZCBOR_MAP_DECODE_KEY_DECODER("confirm", zcbor_bool_decode, &confirm)
	};

	zcbor_state_t *zsd = ctxt->reader->zs;

	ok = zcbor_map_decode_bulk(zsd, image_list_decode,
		ARRAY_SIZE(image_list_decode), &decoded) == 0;

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	/* Determine which slot is being operated on. */
	if (zhash.len == 0) {
		if (confirm) {
			slot = IMG_MGMT_BOOT_CURR_SLOT;
		} else {
			/* A 'test' without a hash is invalid. */
			return MGMT_ERR_EINVAL;
		}
	} else if (zhash.len != IMAGE_HASH_LEN) {
		/* The img_mgmt_find_by_hash does exact length compare
		 * so just fail here.
		 */
		return MGMT_ERR_EINVAL;
	} else {
		uint8_t hash[IMAGE_HASH_LEN];

		memcpy(hash, zhash.value, zhash.len);

		slot = img_mgmt_find_by_hash(hash, NULL);
		if (slot < 0) {
			return MGMT_ERR_EINVAL;
		}
	}

	if (slot == IMG_MGMT_BOOT_CURR_SLOT && confirm) {
		/* Confirm current setup. */
		rc = img_mgmt_state_confirm();
	} else {
		rc = img_mgmt_state_set_pending(slot, confirm);
	}
	if (rc != 0) {
		return rc;
	}

	/* Send the current image state in the response. */
	rc = img_mgmt_state_read(ctxt);
	if (rc != 0) {
		return rc;
	}

	return 0;
}
