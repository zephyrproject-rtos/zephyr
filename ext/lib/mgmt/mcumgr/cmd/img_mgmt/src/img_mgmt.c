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

#include <limits.h>
#include <assert.h>
#include <string.h>

#include "cborattr/cborattr.h"
#include "mgmt/mgmt.h"

#include "img_mgmt/image.h"
#include "img_mgmt/img_mgmt.h"
#include "img_mgmt/img_mgmt_impl.h"
#include "img_mgmt_priv.h"
#include "img_mgmt_config.h"

#define IMG_MGMT_DATA_SHA_LEN 32

static mgmt_handler_fn img_mgmt_upload;
static mgmt_handler_fn img_mgmt_erase;

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

#define IMG_MGMT_HANDLER_CNT \
    sizeof(img_mgmt_handlers) / sizeof(img_mgmt_handlers[0])

static struct mgmt_group img_mgmt_group = {
    .mg_handlers = (struct mgmt_handler *)img_mgmt_handlers,
    .mg_handlers_count = IMG_MGMT_HANDLER_CNT,
    .mg_group_id = MGMT_GROUP_ID_IMAGE,
};

static struct {
    /* Whether an upload is currently in progress. */
    bool uploading;

    /** Expected offset of next upload request. */
    size_t off;

    /** Total length of image currently being uploaded. */
    size_t len;

    /** Hash of image data; used for resume of a partial upload. */
    uint8_t data_sha_len;
    uint8_t data_sha[IMG_MGMT_DATA_SHA_LEN];
} img_mgmt_ctxt;

/**
 * Finds the TLVs in the specified image slot, if any.
 */
static int
img_mgmt_find_tlvs(const struct image_header *hdr,
                   int slot, size_t *start_off, size_t *end_off)
{
    struct image_tlv_info tlv_info;
    int rc;

    rc = img_mgmt_impl_read(slot, *start_off, &tlv_info, sizeof tlv_info);
    if (rc != 0) {
        /* Read error. */
        return MGMT_ERR_EUNKNOWN;
    }

    if (tlv_info.it_magic != IMAGE_TLV_INFO_MAGIC) {
        /* No TLVs. */
        return MGMT_ERR_ENOENT;
    }

    *start_off += sizeof tlv_info;
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
    struct image_header hdr;
    struct image_tlv tlv;
    size_t data_off;
    size_t data_end;
    bool hash_found;
    int rc;

    rc = img_mgmt_impl_read(image_slot, 0, &hdr, sizeof hdr);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    if (ver != NULL) {
        memset(ver, 0xff, sizeof(*ver));
    }
    if (hdr.ih_magic == IMAGE_MAGIC) {
        if (ver != NULL) {
            memcpy(ver, &hdr.ih_ver, sizeof(*ver));
        }
    } else if (hdr.ih_magic == 0xffffffff) {
        return MGMT_ERR_ENOENT;
    } else {
        return MGMT_ERR_EUNKNOWN;
    }

    if (flags != NULL) {
        *flags = hdr.ih_flags;
    }

    /* Read the image's TLVs.  All images are required to have a hash TLV.  If
     * the hash is missing, the image is considered invalid.
     */
    data_off = hdr.ih_hdr_size + hdr.ih_img_size;
    rc = img_mgmt_find_tlvs(&hdr, image_slot, &data_off, &data_end);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    hash_found = false;
    while (data_off + sizeof tlv <= data_end) {
        rc = img_mgmt_impl_read(image_slot, data_off, &tlv, sizeof tlv);
        if (rc != 0) {
            return MGMT_ERR_EUNKNOWN;
        }
        if (tlv.it_type == 0xff && tlv.it_len == 0xffff) {
            return MGMT_ERR_EUNKNOWN;
        }
        if (tlv.it_type != IMAGE_TLV_SHA256 || tlv.it_len != IMAGE_HASH_LEN) {
            /* Non-hash TLV.  Skip it. */
            data_off += sizeof tlv + tlv.it_len;
            continue;
        }

        if (hash_found) {
            /* More than one hash. */
            return MGMT_ERR_EUNKNOWN;
        }
        hash_found = true;

        data_off += sizeof tlv;
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

    for (i = 0; i < 2; i++) {
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

    for (i = 0; i < 2; i++) {
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
    CborError err;
    int rc;

    if (img_mgmt_slot_in_use(1)) {
        /* No free slot. */
        return MGMT_ERR_EBADSTATE;
    }

    rc = img_mgmt_impl_erase_slot();

    err = 0;
    err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
    err |= cbor_encode_int(&ctxt->encoder, rc);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    /* reset uploading information on erase */
    img_mgmt_ctxt.uploading = false;

    return 0;
}

/**
 * Encodes an image upload response.
 */
static int
img_mgmt_encode_upload_rsp(struct mgmt_ctxt *ctxt, int status)
{
    CborError err;

    err = 0;
    err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
    err |= cbor_encode_int(&ctxt->encoder, status);
    err |= cbor_encode_text_stringz(&ctxt->encoder, "off");
    err |= cbor_encode_int(&ctxt->encoder, img_mgmt_ctxt.off);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }
    return 0;
}

/* check if header for first packet is valid */
static int
img_mgmt_check_header(const uint8_t *req_data, size_t len)
{
    struct image_header hdr;

    if (len < sizeof(hdr)) {
        return MGMT_ERR_EINVAL;
    }

    memcpy(&hdr, req_data, sizeof(hdr));
    if (hdr.ih_magic != IMAGE_MAGIC) {
        return MGMT_ERR_EINVAL;
    }

    return 0;
}

/**
 * Processes an upload request specifying an offset of 0 (i.e., the first image
 * chunk).  The caller is responsible for encoding the response.
 */
static int
img_mgmt_upload_first_chunk(struct mgmt_ctxt *ctxt, const uint8_t *req_data,
                            size_t len, const uint8_t *data_sha,
                            size_t data_sha_len)
{
    int rc;

    if (img_mgmt_slot_in_use(1)) {
        /* No free slot. */
        return MGMT_ERR_ENOMEM;
    }

    rc = img_mgmt_impl_erase_slot();
    if (rc != 0) {
        return rc;
    }

    img_mgmt_ctxt.uploading = true;
    img_mgmt_ctxt.off = 0;
    img_mgmt_ctxt.len = 0;

    /*
     * We accept SHA trimmed to any length by client since it's up to client
     * to make sure provided data are good enough to avoid collisions when
     * resuming upload.
     */
    img_mgmt_ctxt.data_sha_len = data_sha_len;
    memcpy(img_mgmt_ctxt.data_sha, data_sha, data_sha_len);
    memset(&img_mgmt_ctxt.data_sha[data_sha_len], 0,
           IMG_MGMT_DATA_SHA_LEN - data_sha_len);

    return 0;
}

/**
 * Command handler: image upload
 */
static int
img_mgmt_upload(struct mgmt_ctxt *ctxt)
{
    uint8_t img_mgmt_data[IMG_MGMT_UL_CHUNK_SIZE];
    uint8_t data_sha[IMG_MGMT_DATA_SHA_LEN];
    size_t data_sha_len = 0;
    unsigned long long len;
    unsigned long long off;
    size_t data_len;
    size_t new_off;
    bool last;
    int rc;

    const struct cbor_attr_t off_attr[] = {
        [0] = {
            .attribute = "data",
            .type = CborAttrByteStringType,
            .addr.bytestring.data = img_mgmt_data,
            .addr.bytestring.len = &data_len,
            .len = sizeof(img_mgmt_data)
        },
        [1] = {
            .attribute = "len",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &len,
            .nodefault = true
        },
        [2] = {
            .attribute = "off",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &off,
            .nodefault = true
        },
        [3] = {
            .attribute = "sha",
            .type = CborAttrByteStringType,
            .addr.bytestring.data = data_sha,
            .addr.bytestring.len = &data_sha_len,
            .len = sizeof(data_sha)
         },
         [4] = { 0 },
    };

    len = ULLONG_MAX;
    off = ULLONG_MAX;
    data_len = 0;
    rc = cbor_read_object(&ctxt->it, off_attr);
    if (rc || off == ULLONG_MAX) {
        return MGMT_ERR_EINVAL;
    }

    if (off == 0) {
        /* Total image length is a required field in the first request. */
        if (len == ULLONG_MAX) {
            return MGMT_ERR_EINVAL;
        }

        rc = img_mgmt_check_header(img_mgmt_data, data_len);
        if (rc) {
            return rc;
        }

        /* Reject if SHA len is to big */
        if (data_sha_len > IMG_MGMT_DATA_SHA_LEN) {
            return MGMT_ERR_EINVAL;
        }

        /*
         * If request includes proper data hash we can check whether there is
         * upload in progress (interrupted due to e.g. link disconnection) with
         * the same data hash so we can just resume it by simply including
         * current upload offset in response.
         */
         if ((data_sha_len > 0) && img_mgmt_ctxt.uploading) {
            if ((img_mgmt_ctxt.data_sha_len == data_sha_len) &&
                    !memcmp(img_mgmt_ctxt.data_sha, data_sha, data_sha_len)) {
                return img_mgmt_encode_upload_rsp(ctxt, 0);
            }
        }

        rc = img_mgmt_upload_first_chunk(ctxt, img_mgmt_data, data_len,
                                         data_sha, data_sha_len);
        if (rc != 0) {
            return rc;
        }
        img_mgmt_ctxt.len = len;
    } else {
        if (!img_mgmt_ctxt.uploading) {
            return MGMT_ERR_EINVAL;
        }

        if (off != img_mgmt_ctxt.off) {
            /* Invalid offset.  Drop the data and send the expected offset. */
            return img_mgmt_encode_upload_rsp(ctxt, 0);
        }
    }

    new_off = img_mgmt_ctxt.off + data_len;
    if (new_off > img_mgmt_ctxt.len) {
        /* Data exceeds image length. */
        return MGMT_ERR_EINVAL;
    }
    last = new_off == img_mgmt_ctxt.len;

    if (data_len > 0) {
        rc = img_mgmt_impl_write_image_data(off, img_mgmt_data, data_len,
                                            last);
        if (rc != 0) {
            return rc;
        }
    }

    img_mgmt_ctxt.off = new_off;
    if (last) {
        /* Upload complete. */
        img_mgmt_ctxt.uploading = false;
    }

    return img_mgmt_encode_upload_rsp(ctxt, 0);
}

void
img_mgmt_register_group(void)
{
    mgmt_register_group(&img_mgmt_group);
}
