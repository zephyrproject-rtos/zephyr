/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/buf.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt_client.h>
#include <zephyr/sys/byteorder.h>

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#include "smp_stub.h"
#include "img_gr_stub.h"

static struct mcumgr_image_data image_dummy_info[2];
static size_t test_offset;
static uint8_t *image_hash_ptr;

#ifdef CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER
#define IMG_UPDATABLE_IMAGE_COUNT CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER
#else
#define IMG_UPDATABLE_IMAGE_COUNT 1
#endif

#define ZCBOR_ENCODE_FLAG(zse, label, value)                                                       \
	(zcbor_tstr_put_lit(zse, label) && zcbor_bool_put(zse, value))

void img_upload_stub_init(void)
{
	test_offset = 0;
}

void img_upload_response(size_t offset, int status)
{
	struct net_buf *nb;
	zcbor_state_t zse[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	bool ok;

	nb = smp_response_buf_allocation();

	if (!nb) {
		return;
	}

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), nb->data, net_buf_tailroom(nb), 0);

	/* Init map start and write image info and data */
	if (status) {
		ok = zcbor_map_start_encode(zse, 4) && zcbor_tstr_put_lit(zse, "rc") &&
	     zcbor_int32_put(zse, status) && zcbor_tstr_put_lit(zse, "off") &&
	     zcbor_size_put(zse, offset) && zcbor_map_end_encode(zse, 4);
	} else {
		/* Drop Status away from response */
		ok = zcbor_map_start_encode(zse, 2) && zcbor_tstr_put_lit(zse, "off") &&
	     zcbor_size_put(zse, offset) && zcbor_map_end_encode(zse, 4);
	}


	if (!ok) {
		smp_client_response_buf_clean();
	} else {
		nb->len = zse->payload - nb->data;
	}
}

void img_fail_response(int status)
{
	struct net_buf *nb;
	zcbor_state_t zse[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	bool ok;

	nb = smp_response_buf_allocation();

	if (!nb) {
		return;
	}

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), nb->data, net_buf_tailroom(nb), 0);

	/* Init map start and write image info and data */
	ok = zcbor_map_start_encode(zse, 2) && zcbor_tstr_put_lit(zse, "rc") &&
	     zcbor_int32_put(zse, status) && zcbor_map_end_encode(zse, 2);
	if (!ok) {
		smp_client_response_buf_clean();
	} else {
		nb->len = zse->payload - nb->data;
	}
}

void img_read_response(int count)
{
	struct net_buf *nb;
	zcbor_state_t zse[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	bool ok;

	nb = smp_response_buf_allocation();

	if (!nb) {
		return;
	}

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), nb->data, net_buf_tailroom(nb), 0);

	ok = zcbor_map_start_encode(zse, 15) && zcbor_tstr_put_lit(zse, "images") &&
	     zcbor_list_start_encode(zse, 2);

	for (int i = 0; ok && i < count; i++) {

		ok = zcbor_map_start_encode(zse, 15) &&
		     ((zcbor_tstr_put_lit(zse, "image") &&
		       zcbor_uint32_put(zse, image_dummy_info[i].img_num))) &&
		     zcbor_tstr_put_lit(zse, "slot") &&
		     zcbor_uint32_put(zse, image_dummy_info[i].slot_num) &&
		     zcbor_tstr_put_lit(zse, "version") &&
		     zcbor_tstr_put_term(zse, image_dummy_info[i].version,
					sizeof(image_dummy_info[i].version)) &&

		     zcbor_tstr_put_lit(zse, "hash") &&
		     zcbor_bstr_encode_ptr(zse, image_dummy_info[i].hash, IMG_MGMT_DATA_SHA_LEN) &&
		     ZCBOR_ENCODE_FLAG(zse, "bootable", image_dummy_info[i].flags.bootable) &&
		     ZCBOR_ENCODE_FLAG(zse, "pending", image_dummy_info[i].flags.pending) &&
		     ZCBOR_ENCODE_FLAG(zse, "confirmed", image_dummy_info[i].flags.confirmed) &&
		     ZCBOR_ENCODE_FLAG(zse, "active", image_dummy_info[i].flags.active) &&
		     ZCBOR_ENCODE_FLAG(zse, "permanent", image_dummy_info[i].flags.permanent) &&
		     zcbor_map_end_encode(zse, 15);
	}

	ok = ok && zcbor_list_end_encode(zse, 2 * IMG_UPDATABLE_IMAGE_COUNT);

	ok = ok && zcbor_map_end_encode(zse, 15);

	if (!ok) {
		smp_client_response_buf_clean();
	} else {
		nb->len = zse->payload - nb->data;
	}
}

void img_erase_response(int status)
{
	struct net_buf *nb;
	zcbor_state_t zse[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	bool ok;

	nb = smp_response_buf_allocation();

	if (!nb) {
		return;
	}

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), nb->data, net_buf_tailroom(nb), 0);

	/* Init map start and write image info and data */
	ok = zcbor_map_start_encode(zse, 2) && zcbor_tstr_put_lit(zse, "rc") &&
	     zcbor_int32_put(zse, status) && zcbor_map_end_encode(zse, 2);
	if (!ok) {
		smp_client_response_buf_clean();
	} else {
		nb->len = zse->payload - nb->data;
	}
}

void img_state_write_verify(struct net_buf *nb)
{
	/* Parse CBOR data: hash and confirm */
	zcbor_state_t zsd[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	int rc;
	struct zcbor_string hash;
	bool confirm;
	size_t decoded;
	struct zcbor_map_decode_key_val list_res_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("confirm", zcbor_bool_decode, &confirm),
		ZCBOR_MAP_DECODE_KEY_DECODER("hash", zcbor_bstr_decode, &hash)
		};

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), nb->data + sizeof(struct smp_hdr), nb->len, 1,
				NULL, 0);

	decoded = 0;
	/* Init buffer values */
	confirm = false;
	hash.len = 0;

	rc = zcbor_map_decode_bulk(zsd, list_res_decode, ARRAY_SIZE(list_res_decode), &decoded);
	if (rc) {
		printf("Corrupted data %d\r\n", rc);
		img_fail_response(MGMT_ERR_EINVAL);
		return;
	}
	if (hash.len) {
		printf("HASH %d", hash.len);
		if (memcmp(hash.value, image_dummy_info[1].hash, 32) == 0) {
			if (confirm) {
				/* Set Permanent bit */
				image_dummy_info[1].flags.permanent = true;
			} else {
				/* Set pending */
				image_dummy_info[1].flags.pending = true;
			}
			img_read_response(2);
		} else {
			img_fail_response(MGMT_ERR_EINVAL);
		}
	} else {
		if (confirm) {
			image_dummy_info[0].flags.confirmed = true;
		}
		img_read_response(2);
	}
}

void img_upload_init_verify(struct net_buf *nb)
{
	/* Parse CBOR data: hash and confirm */
	zcbor_state_t zsd[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	int rc;
	uint32_t image;
	struct zcbor_string sha, data;
	size_t decoded, length, offset;
	struct zcbor_map_decode_key_val list_res_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("image", zcbor_uint32_decode, &image),
		ZCBOR_MAP_DECODE_KEY_DECODER("data", zcbor_bstr_decode, &data),
		ZCBOR_MAP_DECODE_KEY_DECODER("len", zcbor_size_decode, &length),
		ZCBOR_MAP_DECODE_KEY_DECODER("off", zcbor_size_decode, &offset),
		ZCBOR_MAP_DECODE_KEY_DECODER("sha", zcbor_bstr_decode, &sha)
		};

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), nb->data + sizeof(struct smp_hdr), nb->len, 1,
				NULL, 0);

	decoded = 0;
	/* Init buffer values */
	sha.len = 0;
	data.len = 0;
	length = SIZE_MAX;
	offset = SIZE_MAX;
	image = UINT32_MAX;

	rc = zcbor_map_decode_bulk(zsd, list_res_decode, ARRAY_SIZE(list_res_decode), &decoded);
	if (rc || data.len == 0 || offset == SIZE_MAX || image != TEST_IMAGE_NUM) {
		printf("Corrupted data %d or %d data len\r\n", rc, data.len);
		img_upload_response(0, MGMT_ERR_EINVAL);
		return;
	}

	if (sha.len) {
		if (memcmp(sha.value, image_hash_ptr, 32)) {
			printf("Hash not same\r\n");
			img_upload_response(0, MGMT_ERR_EINVAL);
			return;
		}
	}

	if (offset != test_offset) {
		printf("Offset not exepected %d vs received %d\r\n", test_offset, offset);
	}

	if (offset == 0) {
		if (length != TEST_IMAGE_SIZE) {
			img_upload_response(0, MGMT_ERR_EINVAL);
		}
	}

	test_offset += data.len;
	printf("Upload offset %d\r\n", test_offset);
	if (test_offset <= TEST_IMAGE_SIZE) {
		img_upload_response(test_offset, MGMT_ERR_EOK);
	} else {
		img_upload_response(0, MGMT_ERR_EINVAL);
	}
}

void img_gr_stub_data_init(uint8_t *hash_ptr)
{
	image_hash_ptr = hash_ptr;
	for (int i = 0; i < 32; i++) {
		image_hash_ptr[i] = i;
		image_dummy_info[0].hash[i] = i + 32;
		image_dummy_info[1].hash[i] = i + 64;
	}
	/* Set dummy image list data */
	for (int i = 0; i < 2; i++) {
		image_dummy_info[i].img_num = i;
		image_dummy_info[i].slot_num = i;
		/* Write version */
		snprintf(image_dummy_info[i].version, IMG_MGMT_VER_MAX_STR_LEN, "1.1.%u", i);
		image_dummy_info[i].version[sizeof(image_dummy_info[i].version) - 1] = '\0';
		image_dummy_info[i].flags.bootable = true;
		image_dummy_info[i].flags.pending = false;
		if (i) {
			image_dummy_info[i].flags.confirmed = false;
			image_dummy_info[i].flags.active = false;
		} else {
			image_dummy_info[i].flags.confirmed = true;
			image_dummy_info[i].flags.active = true;
		}
		image_dummy_info[i].flags.permanent = false;
	}
}
