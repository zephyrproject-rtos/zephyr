/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/mcuboot.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <bootutil/bootutil_public.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <mgmt/mcumgr/grp/img_mgmt/img_mgmt_priv.h>

#ifdef CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#endif

#ifdef CONFIG_MCUMGR_GRP_IMG_IMAGE_SLOT_STATE_HOOK
#include <mgmt/mcumgr/transport/smp_internal.h>
#endif

LOG_MODULE_DECLARE(mcumgr_img_grp, CONFIG_MCUMGR_GRP_IMG_LOG_LEVEL);

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

/* Flags returned by img_mgmt_state_read() for queried slot */
#define REPORT_SLOT_ACTIVE	BIT(0)
#define REPORT_SLOT_PENDING	BIT(1)
#define REPORT_SLOT_CONFIRMED	BIT(2)
#define REPORT_SLOT_PERMANENT	BIT(3)

#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)
#define DIRECT_XIP_BOOT_UNSET		0
#define DIRECT_XIP_BOOT_ONCE		1
#define DIRECT_XIP_BOOT_REVERT		2
#define DIRECT_XIP_BOOT_FOREVER		3
#endif

/**
 * Collects information about the specified image slot.
 */
#ifndef CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP
uint8_t
img_mgmt_state_flags(int query_slot)
{
	uint8_t flags;
	int swap_type;
	int image = query_slot / 2;	/* We support max 2 images for now */
	int active_slot = img_mgmt_active_slot(image);

	flags = 0;

	/* Determine if this is pending or confirmed (only applicable for
	 * unified images and loaders.
	 */
	swap_type = img_mgmt_swap_type(query_slot);
	switch (swap_type) {
	case IMG_MGMT_SWAP_TYPE_NONE:
		if (query_slot == active_slot) {
			flags |= IMG_MGMT_STATE_F_CONFIRMED;
		}
		break;

	case IMG_MGMT_SWAP_TYPE_TEST:
		if (query_slot == active_slot) {
			flags |= IMG_MGMT_STATE_F_CONFIRMED;
		} else {
			flags |= IMG_MGMT_STATE_F_PENDING;
		}
		break;

	case IMG_MGMT_SWAP_TYPE_PERM:
		if (query_slot == active_slot) {
			flags |= IMG_MGMT_STATE_F_CONFIRMED;
		} else {
			flags |= IMG_MGMT_STATE_F_PENDING | IMG_MGMT_STATE_F_PERMANENT;
		}
		break;

	case IMG_MGMT_SWAP_TYPE_REVERT:
		if (query_slot != active_slot) {
			flags |= IMG_MGMT_STATE_F_CONFIRMED;
		}
		break;
	}

	/* Only running application is active */
	if (image == img_mgmt_active_image() && query_slot == active_slot) {
		flags |= IMG_MGMT_STATE_F_ACTIVE;
	}

	return flags;
}
#else
uint8_t
img_mgmt_state_flags(int query_slot)
{
	uint8_t flags = 0;
	int image = query_slot / 2;	/* We support max 2 images for now */
	int active_slot = img_mgmt_active_slot(image);

	/* In case when MCUboot is configured for DirectXIP slot may only be
	 * active or pending. Slot is marked pending only when version in that slot
	 * is higher than version of active slot.
	 */
	if (image == img_mgmt_active_image() && query_slot == active_slot) {
		flags = IMG_MGMT_STATE_F_ACTIVE;
	} else {
		struct image_version sver;
		struct image_version aver;
		int rcs = img_mgmt_read_info(query_slot, &sver, NULL, NULL);
		int rca = img_mgmt_read_info(active_slot, &aver, NULL, NULL);

		if (rcs == 0 && rca == 0 && img_mgmt_vercmp(&aver, &sver) < 0) {
			flags = IMG_MGMT_STATE_F_PENDING | IMG_MGMT_STATE_F_PERMANENT;
		}
	}

	return flags;
}
#endif

#if !defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP) && \
	!defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)
int img_mgmt_get_next_boot_slot(int image, enum img_mgmt_next_boot_type *type)
{
	const int active_slot = img_mgmt_active_slot(image);
	const int state = mcuboot_swap_type_multi(image);
	/* All cases except BOOT_SWAP_TYPE_NONE return opposite slot */
	int slot = img_mgmt_get_opposite_slot(active_slot);
	enum img_mgmt_next_boot_type lt = NEXT_BOOT_TYPE_NORMAL;

	switch (state) {
	case BOOT_SWAP_TYPE_NONE:
		/* Booting to the same slot, keeping type to NEXT_BOOT_TYPE_NORMAL */
		slot = active_slot;
		break;
	case BOOT_SWAP_TYPE_PERM:
		/* For BOOT_SWAP_TYPE_PERM reported type will be NEXT_BOOT_TYPE_NORMAL,
		 * and only difference between this and BOOT_SWAP_TYPE_NONE is that
		 * the later boots to the application in currently active slot while the former
		 * to the application in the opposite to active slot.
		 * Normal here means that it is ordinary boot and slot has not been marked
		 * for revert or pending for test, and will change on reset.
		 */
		break;
	case BOOT_SWAP_TYPE_REVERT:
		/* Application is in test mode and has not yet been confirmed,
		 * which means that on the next boot the application will revert to
		 * the copy from reported slot.
		 */
		lt = NEXT_BOOT_TYPE_REVERT;
		break;
	case BOOT_SWAP_TYPE_TEST:
		/* Reported next boot slot is set for one boot only and app needs to
		 * confirm itself or it will be reverted.
		 */
		lt = NEXT_BOOT_TYPE_TEST;
		break;
	default:
		/* Should never, ever happen */
		LOG_DBG("Unexpected swap state %d", state);
		return -1;
	}
	LOG_DBG("(%d, *) => slot = %d, type = %d", image, slot, lt);

	if (type != NULL) {
		*type = lt;
	}
	return slot;
}
#else

#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)

static int read_directxip_state(int slot)
{
	struct boot_swap_state bss;
	int fa_id = img_mgmt_flash_area_id(slot);
	const struct flash_area *fa;
	int rc = 0;

	__ASSERT(fa_id != -1, "Could not map slot to area ID");

	rc = flash_area_open(fa_id, &fa);
	if (rc < 0) {
		return rc;
	}
	rc = boot_read_swap_state(fa, &bss);
	flash_area_close(fa);
	if (rc != 0) {
		LOG_ERR("Failed to read state of slot %d with error %d", slot, rc);
		return -1;
	}

	if (bss.magic == BOOT_MAGIC_GOOD) {
		if (bss.image_ok == BOOT_FLAG_SET) {
			return DIRECT_XIP_BOOT_FOREVER;
		} else if (bss.copy_done == BOOT_FLAG_SET) {
			return DIRECT_XIP_BOOT_REVERT;
		}
		return DIRECT_XIP_BOOT_ONCE;
	}
	return DIRECT_XIP_BOOT_UNSET;
}
#endif /* defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT) */

int img_mgmt_get_next_boot_slot(int image, enum img_mgmt_next_boot_type *type)
{
	struct image_version aver;
	struct image_version over;
	int active_slot = img_mgmt_active_slot(image);
	int other_slot = img_mgmt_get_opposite_slot(active_slot);
#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)
	int active_slot_state;
	int other_slot_state;
#endif /* defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT) */
	enum img_mgmt_next_boot_type lt = NEXT_BOOT_TYPE_NORMAL;
	int return_slot = active_slot;


	int rcs = img_mgmt_read_info(other_slot, &over, NULL, NULL);
	int rca = img_mgmt_read_info(active_slot, &aver, NULL, NULL);

#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)
	active_slot_state = read_directxip_state(active_slot);
	other_slot_state = read_directxip_state(other_slot);
	if (rca != 0 ||
	    (rcs != 0 && rcs != IMG_MGMT_ERR_NO_IMAGE)) {
		/* We do not really know what will happen, as we can not
		 * read states from bootloader.
		 */
		LOG_ERR("img_mgmt_read_info_failed rca = %d, rcs = %d",
			rca, rcs);
		goto out;
	}
	if (other_slot_state < 0 || active_slot_state < 0) {
		LOG_ERR("Slot state read failed with status: active %d, other %d",
			active_slot_state, other_slot_state);
		/* We do not really know what will happen, as we can not
		 * read states from bootloader.
		 */
		goto out;
	}

	/* There is not other image, the active one will boot next time */
	if (rcs == IMG_MGMT_ERR_NO_IMAGE) {
		goto out;
	}

	if (active_slot_state == DIRECT_XIP_BOOT_REVERT) {
		lt = NEXT_BOOT_TYPE_REVERT;
		return_slot = other_slot;
	} else if (other_slot_state == DIRECT_XIP_BOOT_UNSET) {
		if (active_slot_state == DIRECT_XIP_BOOT_ONCE) {
			lt = NEXT_BOOT_TYPE_TEST;
		}
	} else if (img_mgmt_vercmp(&aver, &over) < 0) {
		if (other_slot_state == DIRECT_XIP_BOOT_FOREVER) {
			return_slot = other_slot;
		} else if (other_slot_state == DIRECT_XIP_BOOT_ONCE) {
			lt = NEXT_BOOT_TYPE_TEST;
			return_slot = other_slot;
		}
	}

out:
#else
	if (rcs == 0 && rca == 0 && img_mgmt_vercmp(&aver, &over) < 0) {
		return_slot = other_slot;
	}
#endif /* defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT) */

	if (type != NULL) {
		*type = lt;
	}

	return return_slot;
}
#endif /* !defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP) && \
	* !defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)
	*/


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
	int image = img_mgmt_slot_to_image(slot);
	int active_slot = img_mgmt_active_slot(image);

#if !defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP) && \
	!defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD)
	enum img_mgmt_next_boot_type type = NEXT_BOOT_TYPE_NORMAL;
	int nbs = img_mgmt_get_next_boot_slot(image, &type);

	if (slot == nbs && type == NEXT_BOOT_TYPE_REVERT) {
		LOG_DBG("(%d) Refused erase revert", slot);
		return 1;
	}

	if ((slot == nbs && type == NEXT_BOOT_TYPE_TEST) ||
	    (active_slot != nbs && type == NEXT_BOOT_TYPE_NORMAL)) {
#if defined(CONFIG_MCUMGR_GRP_IMG_ALLOW_ERASE_PENDING)
		LOG_DBG("(%d) Allowed erase pending", slot);
		/* Pass through to return (active_slot == slot) */
#else
		LOG_DBG("(%d) Refused erase pending", slot);
		return 1;
#endif
	}
#endif

	return (active_slot == slot);
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
	uint8_t state_flags;
	int rc;

	state_flags = img_mgmt_state_flags(slot);

	/* Unconfirmed slots are always runnable.  A confirmed slot can only be
	 * run if it is a loader in a split image setup.
	 */
	if (state_flags & IMG_MGMT_STATE_F_CONFIRMED && slot != 0) {
		rc = IMG_MGMT_ERR_IMAGE_ALREADY_PENDING;
		goto done;
	}

	rc = img_mgmt_write_pending(slot, permanent);

done:

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
		rc = IMG_MGMT_ERR_IMAGE_ALREADY_PENDING;
		goto err;
	}

	rc = img_mgmt_write_confirmed();

#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
	if (!rc) {
		int32_t err_rc;
		uint16_t err_group;
		struct img_mgmt_image_confirmed confirmed_data = {
			.image = 0
		};

		(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED, &confirmed_data,
					   sizeof(confirmed_data), &err_rc, &err_group);
	}
#endif

err:
	return rc;
}

/* Return zcbor encoding result */
static bool img_mgmt_state_encode_slot(struct smp_streamer *ctxt, uint32_t slot, int state_flags)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	uint32_t flags;
	char vers_str[IMG_MGMT_VER_MAX_STR_LEN];
	uint8_t hash[IMAGE_HASH_LEN]; /* SHA256 hash */
	struct zcbor_string zhash = { .value = hash, .len = IMAGE_HASH_LEN };
	struct image_version ver;
	bool ok;
	int rc = img_mgmt_read_info(slot, &ver, hash, &flags);

#if defined(CONFIG_MCUMGR_GRP_IMG_IMAGE_SLOT_STATE_HOOK)
	int32_t err_rc;
	uint16_t err_group;
	struct img_mgmt_state_slot_encode slot_encode_data = {
		.ok = &ok,
		.zse = zse,
		.slot = slot,
		.version = vers_str,
		.hash = hash,
		.flags = flags,
	};
#endif

	if (rc != 0) {
		/* zcbor encoding did not fail */
		return true;
	}

	ok = zcbor_map_start_encode(zse, CONFIG_MCUMGR_GRP_IMG_IMAGE_SLOT_STATE_STATES)	&&
	     (CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER == 1	||
	      (zcbor_tstr_put_lit(zse, "image")						&&
	       zcbor_uint32_put(zse, slot >> 1)))					&&
	     zcbor_tstr_put_lit(zse, "slot")						&&
	     zcbor_uint32_put(zse, slot % 2)						&&
	     zcbor_tstr_put_lit(zse, "version");

	if (ok) {
		if (img_mgmt_ver_str(&ver, vers_str) < 0) {
			ok = zcbor_tstr_put_lit(zse, "<\?\?\?>");
		} else {
			vers_str[sizeof(vers_str) - 1] = '\0';
			ok = zcbor_tstr_put_term(zse, vers_str, sizeof(vers_str));
		}
	}

	ok = ok && zcbor_tstr_put_lit(zse, "hash")					&&
	     zcbor_bstr_encode(zse, &zhash)						&&
	     ZCBOR_ENCODE_FLAG(zse, "bootable", !(flags & IMAGE_F_NON_BOOTABLE))	&&
	     ZCBOR_ENCODE_FLAG(zse, "pending", state_flags & REPORT_SLOT_PENDING)	&&
	     ZCBOR_ENCODE_FLAG(zse, "confirmed", state_flags & REPORT_SLOT_CONFIRMED)	&&
	     ZCBOR_ENCODE_FLAG(zse, "active", state_flags & REPORT_SLOT_ACTIVE)		&&
	     ZCBOR_ENCODE_FLAG(zse, "permanent", state_flags & REPORT_SLOT_PERMANENT);

	if (!ok) {
		goto failed;
	}

#if defined(CONFIG_MCUMGR_GRP_IMG_IMAGE_SLOT_STATE_HOOK)
	/* Send notification to application to optionally append more fields */
	(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_IMAGE_SLOT_STATE, &slot_encode_data,
				   sizeof(slot_encode_data), &err_rc, &err_group);
#endif

	ok &= zcbor_map_end_encode(zse, CONFIG_MCUMGR_GRP_IMG_IMAGE_SLOT_STATE_STATES);

failed:
	return ok;
}

/**
 * Command handler: image state read
 */
int
img_mgmt_state_read(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	uint32_t i;
	bool ok;

	ok = zcbor_tstr_put_lit(zse, "images") &&
	     zcbor_list_start_encode(zse, 2 * CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER);

	img_mgmt_take_lock();

	for (i = 0; ok && i < CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER; i++) {
		/* _a is active slot, _o is opposite slot */
		enum img_mgmt_next_boot_type type = NEXT_BOOT_TYPE_NORMAL;
		int next_boot_slot = img_mgmt_get_next_boot_slot(i, &type);
		int slot_a = img_mgmt_active_slot(i);
		int slot_o = img_mgmt_get_opposite_slot(slot_a);
		int flags_a = REPORT_SLOT_ACTIVE;
		int flags_o = 0;

		if (type != NEXT_BOOT_TYPE_REVERT) {
			flags_a |= REPORT_SLOT_CONFIRMED;
		}

		if (next_boot_slot != slot_a) {
			if (type == NEXT_BOOT_TYPE_NORMAL) {
				flags_o = REPORT_SLOT_PENDING | REPORT_SLOT_PERMANENT;
			} else if (type == NEXT_BOOT_TYPE_REVERT) {
				flags_o = REPORT_SLOT_CONFIRMED;
			} else if (type == NEXT_BOOT_TYPE_TEST) {
				flags_o = REPORT_SLOT_PENDING;
			}
		}

		/* Need to report slots in proper order */
		if (slot_a < slot_o) {
			ok = img_mgmt_state_encode_slot(ctxt, slot_a, flags_a) &&
			     img_mgmt_state_encode_slot(ctxt, slot_o, flags_o);
		} else {
			ok = img_mgmt_state_encode_slot(ctxt, slot_o, flags_o) &&
			     img_mgmt_state_encode_slot(ctxt, slot_a, flags_a);
		}
	}

	/* Ending list encoding for two slots per image */
	ok = ok && zcbor_list_end_encode(zse, 2 * CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER);
	/* splitStatus is always 0 so in frugal list it is not present at all */
	if (!IS_ENABLED(CONFIG_MCUMGR_GRP_IMG_FRUGAL_LIST) && ok) {
		ok = zcbor_tstr_put_lit(zse, "splitStatus") &&
		     zcbor_int32_put(zse, 0);
	}

	img_mgmt_release_lock();

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}

static int img_mgmt_set_next_boot_slot_common(int slot, int active_slot, bool confirm)
{
	const struct flash_area *fa;
	int area_id = img_mgmt_flash_area_id(slot);
	int rc = 0;

	if (flash_area_open(area_id, &fa) != 0) {
		return IMG_MGMT_ERR_FLASH_OPEN_FAILED;
	}

	rc = boot_set_next(fa, slot == active_slot, confirm);
	if (rc != 0) {
		/* Failed to set next slot for boot as desired */
		LOG_ERR("Faled boot_set_next with code %d, for slot %d,"
			" with active slot %d and confirm %d",
			 rc, slot, active_slot, confirm);

		/* Translate from boot util error code to IMG mgmt group error code */
		if (rc == BOOT_EFLASH) {
			rc = IMG_MGMT_ERR_FLASH_WRITE_FAILED;
		} else if (rc == BOOT_EBADVECT) {
			rc = IMG_MGMT_ERR_INVALID_IMAGE_VECTOR_TABLE;
		} else if (rc == BOOT_EBADIMAGE) {
			rc = IMG_MGMT_ERR_INVALID_IMAGE_HEADER_MAGIC;
		} else {
			rc = IMG_MGMT_ERR_UNKNOWN;
		}
	}
	flash_area_close(fa);

#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS)
	if (rc == 0 && slot == active_slot && confirm) {
		/* Confirm event is only sent for active slot */
		int32_t err_rc;
		uint16_t err_group;
		struct img_mgmt_image_confirmed confirmed_data = {
			.image = img_mgmt_slot_to_image(slot)
		};

		(void)mgmt_callback_notify(MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED, &confirmed_data,
					   sizeof(confirmed_data), &err_rc, &err_group);
	}
#endif

	return rc;
}

#ifndef CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT
int img_mgmt_set_next_boot_slot(int slot, bool confirm)
{
	/* image the requested slot is defined within */
	int image = img_mgmt_slot_to_image(slot);
	/* active_slot is slot that is considered active/primary/executing
	 * for a given image.
	 */
	int active_slot = img_mgmt_active_slot(image);
	enum img_mgmt_next_boot_type type = NEXT_BOOT_TYPE_NORMAL;
	int next_boot_slot = img_mgmt_get_next_boot_slot(image, &type);

	LOG_DBG("(%d, %s)", slot, confirm ? "confirm" : "test");
	LOG_DBG("aimg = %d, img = %d, aslot = %d, slot = %d, nbs = %d",
		img_mgmt_active_image(), image, active_slot, slot, next_boot_slot);

	/* MCUmgr should not allow to confirm non-active image slots to prevent
	 * confirming something that might not have been verified to actually be bootable
	 * or have stuck in primary slot of other image. Unfortunately there was
	 * a bug in logic that always allowed to confirm secondary slot of any
	 * image. Now the behaviour is controlled via Kconfig options.
	 */
#ifndef CONFIG_MCUMGR_GRP_IMG_ALLOW_CONFIRM_NON_ACTIVE_IMAGE_ANY
	if (confirm && image != img_mgmt_active_image() &&
	    (!IS_ENABLED(CONFIG_MCUMGR_GRP_IMG_ALLOW_CONFIRM_NON_ACTIVE_IMAGE_SECONDARY) ||
	     slot == active_slot)) {
		LOG_DBG("Not allowed to confirm non-active images");
		return IMG_MGMT_ERR_IMAGE_CONFIRMATION_DENIED;
	}
#endif

	/* Setting test to active slot is not allowed. */
	if (!confirm && slot == active_slot) {
		return IMG_MGMT_ERR_IMAGE_SETTING_TEST_TO_ACTIVE_DENIED;
	}

	if (type == NEXT_BOOT_TYPE_TEST) {
		/* Do nothing when requested to test the slot already set for test. */
		if (!confirm && slot == next_boot_slot) {
			return 0;
		}
		/* Changing to other, for test or not, is not allowed/ */
		return IMG_MGMT_ERR_IMAGE_ALREADY_PENDING;
	}

	/* Normal boot means confirmed boot to either active slot or the opposite slot. */
	if (type == NEXT_BOOT_TYPE_NORMAL) {
		/* Do nothing when attempting to confirm slot that will be boot next time. */
		if (confirm && slot == next_boot_slot) {
			return 0;
		}

		/* Can not change slot once other than running has been confirmed. */
		if ((slot == active_slot && active_slot != next_boot_slot) ||
		    (!confirm && slot != active_slot && slot == next_boot_slot)) {
			return IMG_MGMT_ERR_IMAGE_ALREADY_PENDING;
		}
		/* Allow selecting non-active slot for boot */
	}

	if (type == NEXT_BOOT_TYPE_REVERT) {
		/* Nothing to do when requested to confirm the next boot slot,
		 * as it is already confirmed in this mode.
		 */
		if (confirm && slot == next_boot_slot) {
			return 0;
		}
		/* Trying to set any slot for test is an error */
		if (!confirm) {
			return IMG_MGMT_ERR_IMAGE_ALREADY_PENDING;
		}
		/* Allow confirming slot == active_slot */
	}

	return img_mgmt_set_next_boot_slot_common(slot, active_slot, confirm);
}
#else
int img_mgmt_set_next_boot_slot(int slot, bool confirm)
{
	int active_image = img_mgmt_active_image();
	int active_slot = img_mgmt_active_slot(active_image);

	LOG_DBG("(%d, %s)", slot, confirm ? "confirm" : "test");
	LOG_DBG("aimg = %d, aslot = %d, slot = %d",
		active_image, active_slot, slot);

	if (slot == active_slot && !confirm) {
		return IMG_MGMT_ERR_IMAGE_SETTING_TEST_TO_ACTIVE_DENIED;
	}

	return img_mgmt_set_next_boot_slot_common(slot, active_slot, confirm);
}
#endif

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

	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;

	ok = zcbor_map_decode_bulk(zsd, image_list_decode,
		ARRAY_SIZE(image_list_decode), &decoded) == 0;

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	img_mgmt_take_lock();

	/* Determine which slot is being operated on. */
	if (zhash.len == 0) {
		if (confirm) {
			slot = img_mgmt_active_slot(img_mgmt_active_image());
		} else {
			/* A 'test' without a hash is invalid. */
			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_IMAGE,
					     IMG_MGMT_ERR_INVALID_HASH);
			goto end;
		}
	} else if (zhash.len != IMAGE_HASH_LEN) {
		/* The img_mgmt_find_by_hash does exact length compare
		 * so just fail here.
		 */
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_IMAGE, IMG_MGMT_ERR_INVALID_HASH);
		goto end;
	} else {
		uint8_t hash[IMAGE_HASH_LEN];

		memcpy(hash, zhash.value, zhash.len);

		slot = img_mgmt_find_by_hash(hash, NULL);
		if (slot < 0) {
			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_IMAGE,
					     IMG_MGMT_ERR_HASH_NOT_FOUND);
			goto end;
		}
	}

	rc = img_mgmt_set_next_boot_slot(slot, confirm);
	if (rc != 0) {
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_IMAGE, rc);
		goto end;
	}

	/* Send the current image state in the response. */
	rc = img_mgmt_state_read(ctxt);
	if (rc != 0) {
		img_mgmt_release_lock();
		return rc;
	}

end:
	img_mgmt_release_lock();

	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	return MGMT_ERR_EOK;
}
