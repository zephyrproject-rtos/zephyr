/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include "cbor.h"
#include "cborattr/cborattr.h"
#include "mgmt/mgmt.h"
#include "img_mgmt/img_mgmt.h"
#include "img_mgmt/image.h"
#include "img_mgmt_priv.h"
#include "img_mgmt/img_mgmt_impl.h"

#define IMG_MGMT_STATE_F_PENDING    0x01
#define IMG_MGMT_STATE_F_CONFIRMED  0x02
#define IMG_MGMT_STATE_F_ACTIVE     0x04
#define IMG_MGMT_STATE_F_PERMANENT  0x08

#define IMG_MGMT_VER_MAX_STR_LEN    25  /* 255.255.65535.4294967295\0 */

/**
 * Collects information about the specified image slot.
 */
static uint8_t
img_mgmt_state_flags(int query_slot)
{
    uint8_t flags;
    int swap_type;

    assert(query_slot == 0 || query_slot == 1);

    flags = 0;

    /* Determine if this is is pending or confirmed (only applicable for
     * unified images and loaders.
     */
    swap_type = img_mgmt_impl_swap_type();
    switch (swap_type) {
    case IMG_MGMT_SWAP_TYPE_NONE:
        if (query_slot == 0) {
            flags |= IMG_MGMT_STATE_F_CONFIRMED;
            flags |= IMG_MGMT_STATE_F_ACTIVE;
        }
        break;

    case IMG_MGMT_SWAP_TYPE_TEST:
        if (query_slot == 0) {
            flags |= IMG_MGMT_STATE_F_CONFIRMED;
        } else if (query_slot == 1) {
            flags |= IMG_MGMT_STATE_F_PENDING;
        }
        break;

    case IMG_MGMT_SWAP_TYPE_PERM:
        if (query_slot == 0) {
            flags |= IMG_MGMT_STATE_F_CONFIRMED;
        } else if (query_slot == 1) {
            flags |= IMG_MGMT_STATE_F_PENDING | IMG_MGMT_STATE_F_PERMANENT;
        }
        break;

    case IMG_MGMT_SWAP_TYPE_REVERT:
        if (query_slot == 0) {
            flags |= IMG_MGMT_STATE_F_ACTIVE;
        } else if (query_slot == 1) {
            flags |= IMG_MGMT_STATE_F_CONFIRMED;
        }
        break;
    }

    /* Slot 0 is always active. */
    /* XXX: The slot 0 assumption only holds when running from flash. */
    if (query_slot == 0) {
        flags |= IMG_MGMT_STATE_F_ACTIVE;
    }

    return flags;
}

/**
 * Indicates whether any image slot is pending (i.e., whether a test swap will
 * happen on the next reboot.
 */
static int
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
    return state_flags & IMG_MGMT_STATE_F_ACTIVE       ||
           state_flags & IMG_MGMT_STATE_F_CONFIRMED    ||
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
    uint8_t state_flags;
    int rc;

    state_flags = img_mgmt_state_flags(slot);

    /* Unconfirmed slots are always runable.  A confirmed slot can only be
     * run if it is a loader in a split image setup.
     */
    if (state_flags & IMG_MGMT_STATE_F_CONFIRMED && slot != 0) {
        return MGMT_ERR_EBADSTATE;
    }

    rc = img_mgmt_impl_write_pending(slot, permanent);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
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
        return MGMT_ERR_EBADSTATE;
    }

    rc = img_mgmt_impl_write_confirmed();
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}

/**
 * Command handler: image state read
 */
int
img_mgmt_state_read(struct mgmt_ctxt *ctxt)
{
    char vers_str[IMG_MGMT_VER_MAX_STR_LEN];
    uint8_t hash[IMAGE_HASH_LEN]; /* SHA256 hash */
    struct image_version ver;
    CborEncoder images;
    CborEncoder image;
    CborError err;
    uint32_t flags;
    uint8_t state_flags;
    int rc;
    int i;

    err = 0;
    err |= cbor_encode_text_stringz(&ctxt->encoder, "images");

    err |= cbor_encoder_create_array(&ctxt->encoder, &images,
                                       CborIndefiniteLength);
    for (i = 0; i < 2; i++) {
        rc = img_mgmt_read_info(i, &ver, hash, &flags);
        if (rc != 0) {
            continue;
        }

        state_flags = img_mgmt_state_flags(i);

        err |= cbor_encoder_create_map(&images, &image,
                                         CborIndefiniteLength);
        err |= cbor_encode_text_stringz(&image, "slot");
        err |= cbor_encode_int(&image, i);

        err |= cbor_encode_text_stringz(&image, "version");
        img_mgmt_ver_str(&ver, vers_str);
        err |= cbor_encode_text_stringz(&image, vers_str);

        err |= cbor_encode_text_stringz(&image, "hash");
        err |= cbor_encode_byte_string(&image, hash, IMAGE_HASH_LEN);

        err |= cbor_encode_text_stringz(&image, "bootable");
        err |= cbor_encode_boolean(&image, !(flags & IMAGE_F_NON_BOOTABLE));

        err |= cbor_encode_text_stringz(&image, "pending");
        err |= cbor_encode_boolean(&image,
                                     state_flags & IMG_MGMT_STATE_F_PENDING);

        err |= cbor_encode_text_stringz(&image, "confirmed");
        err |= cbor_encode_boolean(&image,
                                     state_flags & IMG_MGMT_STATE_F_CONFIRMED);

        err |= cbor_encode_text_stringz(&image, "active");
        err |= cbor_encode_boolean(&image,
                                     state_flags & IMG_MGMT_STATE_F_ACTIVE);

        err |= cbor_encode_text_stringz(&image, "permanent");
        err |= cbor_encode_boolean(&image,
                                     state_flags & IMG_MGMT_STATE_F_PERMANENT);

        err |= cbor_encoder_close_container(&images, &image);
    }

    err |= cbor_encoder_close_container(&ctxt->encoder, &images);

    err |= cbor_encode_text_stringz(&ctxt->encoder, "splitStatus");
    err |= cbor_encode_int(&ctxt->encoder, 0);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

/**
 * Command handler: image state write
 */
int
img_mgmt_state_write(struct mgmt_ctxt *ctxt)
{
    /*
     * We add 1 to the 32-byte hash buffer as _cbor_value_copy_string() adds
     * a null character at the end of the buffer.
     */
    uint8_t hash[IMAGE_HASH_LEN + 1];
    size_t hash_len;
    bool confirm;
    int slot;
    int rc;

    const struct cbor_attr_t write_attr[] = {
        [0] = {
            .attribute = "hash",
            .type = CborAttrByteStringType,
            .addr.bytestring.data = hash,
            .addr.bytestring.len = &hash_len,
            .len = sizeof(hash),
        },
        [1] = {
            .attribute = "confirm",
            .type = CborAttrBooleanType,
            .addr.boolean = &confirm,
            .dflt.boolean = false,
        },
        [2] = { 0 },
    };

    hash_len = 0;
    rc = cbor_read_object(&ctxt->it, write_attr);
    if (rc != 0) {
        return MGMT_ERR_EINVAL;
    }

    /* Determine which slot is being operated on. */
    if (hash_len == 0) {
        if (confirm) {
            slot = 0;
        } else {
            /* A 'test' without a hash is invalid. */
            return MGMT_ERR_EINVAL;
        }
    } else {
        slot = img_mgmt_find_by_hash(hash, NULL);
        if (slot < 0) {
            return MGMT_ERR_EINVAL;
        }
    }

    if (slot == 0 && confirm) {
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
