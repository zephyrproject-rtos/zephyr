/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt_client.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <mgmt/mcumgr/transport/smp_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcumgr_grp_img_client, CONFIG_MCUMGR_GRP_IMG_CLIENT_LOG_LEVEL);

#define MCUMGR_UPLOAD_INIT_HEADER_BUF_SIZE 128

/* Pointer for active Client */
static struct img_mgmt_client *active_client;
/* Image State read or set response pointer */
static struct mcumgr_image_state *image_info;
/* Image upload response pointer */
static struct mcumgr_image_upload *image_upload_buf;

static K_SEM_DEFINE(mcumgr_img_client_grp_sem, 0, 1);
static K_MUTEX_DEFINE(mcumgr_img_client_grp_mutex);

static const char smp_images_str[] = "images";
#define IMAGES_STR_LEN (sizeof(smp_images_str) - 1)

static int image_state_res_fn(struct net_buf *nb, void *user_data)
{
	zcbor_state_t zsd[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	struct zcbor_string value = {0};
	int buf_len, rc;
	uint32_t img_num, slot_num;
	struct zcbor_string hash, version;
	bool bootable, pending, confirmed, active, permanent, ok;
	size_t decoded;
	struct zcbor_map_decode_key_val list_res_decode[] = {
		/* Mandatory */
		ZCBOR_MAP_DECODE_KEY_DECODER("version", zcbor_tstr_decode, &version),
		ZCBOR_MAP_DECODE_KEY_DECODER("hash", zcbor_bstr_decode, &hash),
		ZCBOR_MAP_DECODE_KEY_DECODER("slot", zcbor_uint32_decode, &slot_num),
		/* Optional */
		ZCBOR_MAP_DECODE_KEY_DECODER("image", zcbor_uint32_decode, &img_num),
		ZCBOR_MAP_DECODE_KEY_DECODER("bootable", zcbor_bool_decode, &bootable),
		ZCBOR_MAP_DECODE_KEY_DECODER("pending", zcbor_bool_decode, &pending),
		ZCBOR_MAP_DECODE_KEY_DECODER("confirmed", zcbor_bool_decode, &confirmed),
		ZCBOR_MAP_DECODE_KEY_DECODER("active", zcbor_bool_decode, &active),
		ZCBOR_MAP_DECODE_KEY_DECODER("permanent", zcbor_bool_decode, &permanent)
		};

	buf_len = active_client->image_list_length;

	if (!nb) {
		image_info->status = MGMT_ERR_ETIMEOUT;
		goto out;
	}

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_start_decode(zsd);
	if (!ok) {
		image_info->status = MGMT_ERR_ECORRUPT;
		goto out;
	}

	ok = zcbor_tstr_decode(zsd, &value);
	if (!ok) {
		image_info->status = MGMT_ERR_ECORRUPT;
		goto out;
	}
	if (value.len != IMAGES_STR_LEN || memcmp(value.value, smp_images_str, IMAGES_STR_LEN)) {
		image_info->status = MGMT_ERR_EINVAL;
		goto out;
	}

	ok = zcbor_list_start_decode(zsd);
	if (!ok) {
		image_info->status = MGMT_ERR_ECORRUPT;
		goto out;
	}

	rc = 0;
	/* Parse Image map info to configured buffer */
	while (rc == 0) {
		decoded = 0;
		zcbor_map_decode_bulk_reset(list_res_decode, ARRAY_SIZE(list_res_decode));
		/* Init buffer values */
		active = false;
		bootable = false;
		confirmed = false;
		pending = false;
		permanent = false;
		img_num = 0;
		slot_num = UINT32_MAX;
		hash.len = 0;
		version.len = 0;

		rc = zcbor_map_decode_bulk(zsd, list_res_decode, ARRAY_SIZE(list_res_decode),
					   &decoded);
		if (rc) {
			if (image_info->image_list_length) {
				/* No more map */
				break;
			}
			LOG_ERR("Corrupted Image data %d", rc);
			image_info->status = MGMT_ERR_EINVAL;
			goto out;
		}
		/* Check that mandatory parameters have decoded */
		if (hash.len != IMG_MGMT_DATA_SHA_LEN || !version.len ||
		    !zcbor_map_decode_bulk_key_found(list_res_decode, ARRAY_SIZE(list_res_decode),
						     "slot")) {
			LOG_ERR("Missing mandatory parametrs");
			image_info->status = MGMT_ERR_EINVAL;
			goto out;
		}
		/* Store parsed values */
		if (buf_len) {
			image_info->image_list[image_info->image_list_length].img_num = img_num;
			image_info->image_list[image_info->image_list_length].slot_num = slot_num;
			memcpy(image_info->image_list[image_info->image_list_length].hash,
			       hash.value, IMG_MGMT_DATA_SHA_LEN);
			if (version.len > IMG_MGMT_VER_MAX_STR_LEN) {
				LOG_WRN("Version truncated len %d -> %d", version.len,
					IMG_MGMT_VER_MAX_STR_LEN);
				version.len = IMG_MGMT_VER_MAX_STR_LEN;
			}
			memcpy(image_info->image_list[image_info->image_list_length].version,
			       version.value, version.len);
			image_info->image_list[image_info->image_list_length].version[version.len] =
				'\0';
			/* Set Image flags */
			image_info->image_list[image_info->image_list_length].flags.bootable =
				bootable;
			image_info->image_list[image_info->image_list_length].flags.pending =
				pending;
			image_info->image_list[image_info->image_list_length].flags.confirmed =
				confirmed;
			image_info->image_list[image_info->image_list_length].flags.active = active;
			image_info->image_list[image_info->image_list_length].flags.permanent =
				permanent;
			/* Update valid image count */
			image_info->image_list_length++;
			buf_len--;
		} else {
			LOG_INF("User configured image list buffer size %d can't store all info",
				active_client->image_list_length);
		}
	}

	ok = zcbor_list_end_decode(zsd);
	if (!ok) {
		image_info->status = MGMT_ERR_ECORRUPT;
	} else {

		image_info->status = MGMT_ERR_EOK;
	}

out:
	if (image_info->status != MGMT_ERR_EOK) {
		image_info->image_list_length = 0;
	}
	rc = image_info->status;
	k_sem_give(user_data);
	return rc;
}

static int image_upload_res_fn(struct net_buf *nb, void *user_data)
{
	zcbor_state_t zsd[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	size_t decoded;
	int rc;
	int32_t res_rc = MGMT_ERR_EOK;

	struct zcbor_map_decode_key_val upload_res_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("off", zcbor_size_decode,
					     &image_upload_buf->image_upload_offset),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &res_rc)};

	if (!nb) {
		image_upload_buf->status = MGMT_ERR_ETIMEOUT;
		goto end;
	}

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), nb->data, nb->len, 1, NULL, 0);

	rc = zcbor_map_decode_bulk(zsd, upload_res_decode, ARRAY_SIZE(upload_res_decode), &decoded);
	if (rc || image_upload_buf->image_upload_offset == SIZE_MAX) {
		image_upload_buf->status = MGMT_ERR_EINVAL;
		goto end;
	}
	image_upload_buf->status = res_rc;

	active_client->upload.offset = image_upload_buf->image_upload_offset;
end:
	/* Set status for Upload request handler */
	rc = image_upload_buf->status;
	k_sem_give(user_data);
	return rc;
}

static int erase_res_fn(struct net_buf *nb, void *user_data)
{
	zcbor_state_t zsd[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	size_t decoded;
	int rc, status = MGMT_ERR_EOK;

	struct zcbor_map_decode_key_val upload_res_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &status)
		};

	if (!nb) {
		active_client->status = MGMT_ERR_ETIMEOUT;
		goto end;
	}

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), nb->data, nb->len, 1, NULL, 0);

	rc = zcbor_map_decode_bulk(zsd, upload_res_decode, ARRAY_SIZE(upload_res_decode), &decoded);
	if (rc) {
		LOG_ERR("Erase fail %d", rc);
		active_client->status = MGMT_ERR_EINVAL;
		goto end;
	}
	active_client->status = status;
end:
	rc = active_client->status;
	k_sem_give(user_data);
	return rc;
}

static size_t upload_message_header_size(struct img_gr_upload *upload_state)
{
	bool ok;
	size_t cbor_length;
	int map_count;
	zcbor_state_t zse[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	uint8_t temp_buf[MCUMGR_UPLOAD_INIT_HEADER_BUF_SIZE];
	uint8_t temp_data = 0U;

	/* Calculation of message header with data length of 1 */

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), temp_buf, MCUMGR_UPLOAD_INIT_HEADER_BUF_SIZE,
			       0);
	if (upload_state->hash_initialized) {
		map_count = 12;
	} else {
		map_count = 10;
	}

	/* Init map start and write image info and data */
	ok = zcbor_map_start_encode(zse, map_count) && zcbor_tstr_put_lit(zse, "image") &&
	     zcbor_uint32_put(zse, upload_state->image_num) && zcbor_tstr_put_lit(zse, "data") &&
	     zcbor_bstr_encode_ptr(zse, &temp_data, 1) && zcbor_tstr_put_lit(zse, "len") &&
	     zcbor_size_put(zse, upload_state->image_size) && zcbor_tstr_put_lit(zse, "off") &&
	     zcbor_size_put(zse, upload_state->offset);

	/* Write hash when it defined and offset is 0 */
	if (ok && upload_state->hash_initialized) {
		ok = zcbor_tstr_put_lit(zse, "sha") &&
		     zcbor_bstr_encode_ptr(zse, upload_state->sha256, IMG_MGMT_DATA_SHA_LEN);
	}

	if (ok) {
		ok = zcbor_map_end_encode(zse, map_count);
	}

	if (!ok) {
		LOG_ERR("Failed to encode Image Upload packet");
		return 0;
	}
	cbor_length = zse->payload - temp_buf;
	/* Return Message header length */
	return cbor_length + (CONFIG_MCUMGR_GRP_IMG_UPLOAD_DATA_ALIGNMENT_SIZE - 1);
}

void img_mgmt_client_init(struct img_mgmt_client *client, struct smp_client_object *smp_client,
			  int image_list_size, struct mcumgr_image_data *image_list)
{
	client->smp_client = smp_client;
	client->image_list_length = image_list_size;
	client->image_list = image_list;
}

int img_mgmt_client_upload_init(struct img_mgmt_client *client, size_t image_size,
				uint32_t image_num, const char *image_hash)
{
	int rc;

	k_mutex_lock(&mcumgr_img_client_grp_mutex, K_FOREVER);
	client->upload.image_size = image_size;
	client->upload.offset = 0;
	client->upload.image_num = image_num;
	if (image_hash) {
		memcpy(client->upload.sha256, image_hash, IMG_MGMT_DATA_SHA_LEN);
		client->upload.hash_initialized = true;
	} else {
		client->upload.hash_initialized = false;
	}

	/* Calculate worst case header size for adapt payload length */
	client->upload.upload_header_size = upload_message_header_size(&client->upload);
	if (client->upload.upload_header_size) {
		rc = MGMT_ERR_EOK;
	} else {
		rc = MGMT_ERR_ENOMEM;
	}
	k_mutex_unlock(&mcumgr_img_client_grp_mutex);
	return rc;
}

int img_mgmt_client_upload(struct img_mgmt_client *client, const uint8_t *data, size_t length,
			   struct mcumgr_image_upload *res_buf)
{
	struct net_buf *nb;
	const uint8_t *write_ptr;
	int rc;
	uint32_t map_count;
	bool ok;
	size_t write_length, max_data_length, offset_before_send, request_length, wrote_length;
	zcbor_state_t zse[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];

	k_mutex_lock(&mcumgr_img_client_grp_mutex, K_FOREVER);
	active_client = client;
	image_upload_buf = res_buf;

	request_length = length;
	wrote_length = 0;
	/* Calculate max data length based on
	 * net_buf size - (SMP header + CBOR message_len + 16-bit CRC + 16-bit length)
	 */
	max_data_length = CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE -
			  (active_client->upload.upload_header_size + MGMT_HDR_SIZE + 2U + 2U);
	/* Trim length based on CONFIG_MCUMGR_GRP_IMG_UPLOAD_DATA_ALIGNMENT_SIZE */
	if (max_data_length % CONFIG_MCUMGR_GRP_IMG_UPLOAD_DATA_ALIGNMENT_SIZE) {
		max_data_length -=
			(max_data_length % CONFIG_MCUMGR_GRP_IMG_UPLOAD_DATA_ALIGNMENT_SIZE);
	}

	while (request_length != wrote_length) {
		write_ptr = data + wrote_length;
		write_length = request_length - wrote_length;
		if (write_length > max_data_length) {
			write_length = max_data_length;
		}

		nb = smp_client_buf_allocation(active_client->smp_client, MGMT_GROUP_ID_IMAGE,
					       IMG_MGMT_ID_UPLOAD, MGMT_OP_WRITE,
					       SMP_MCUMGR_VERSION_1);
		if (!nb) {
			image_upload_buf->status = MGMT_ERR_ENOMEM;
			goto end;
		}

		zcbor_new_encode_state(zse, ARRAY_SIZE(zse), nb->data + nb->len,
				       net_buf_tailroom(nb), 0);
		if (active_client->upload.offset) {
			map_count = 6;
		} else if (active_client->upload.hash_initialized) {
			map_count = 12;
		} else {
			map_count = 10;
		}

		/* Init map start and write image info, data and offset */
		ok = zcbor_map_start_encode(zse, map_count) && zcbor_tstr_put_lit(zse, "image") &&
		     zcbor_uint32_put(zse, active_client->upload.image_num) &&
		     zcbor_tstr_put_lit(zse, "data") &&
		     zcbor_bstr_encode_ptr(zse, write_ptr, write_length) &&
		     zcbor_tstr_put_lit(zse, "off") &&
		     zcbor_size_put(zse, active_client->upload.offset);
		/* Write Len and configured hash when offset is zero */
		if (ok && !active_client->upload.offset) {
			ok = zcbor_tstr_put_lit(zse, "len") &&
			     zcbor_size_put(zse, active_client->upload.image_size);
			if (ok && active_client->upload.hash_initialized) {
				ok = zcbor_tstr_put_lit(zse, "sha") &&
				     zcbor_bstr_encode_ptr(zse, active_client->upload.sha256,
							   IMG_MGMT_DATA_SHA_LEN);
			}
		}

		if (ok) {
			ok = zcbor_map_end_encode(zse, map_count);
		}

		if (!ok) {
			LOG_ERR("Failed to encode Image Upload packet");
			smp_packet_free(nb);
			image_upload_buf->status = MGMT_ERR_ENOMEM;
			goto end;
		}

		offset_before_send = active_client->upload.offset;
		nb->len = zse->payload - nb->data;
		k_sem_reset(&mcumgr_img_client_grp_sem);

		image_upload_buf->status = MGMT_ERR_EINVAL;
		image_upload_buf->image_upload_offset = SIZE_MAX;

		rc = smp_client_send_cmd(active_client->smp_client, nb, image_upload_res_fn,
					 &mcumgr_img_client_grp_sem,
					 CONFIG_MCUMGR_GRP_IMG_FLASH_OPERATION_TIMEOUT);
		if (rc) {
			LOG_ERR("Failed to send SMP Upload init packet, err: %d", rc);
			smp_packet_free(nb);
			image_upload_buf->status = rc;
			goto end;

		}
		k_sem_take(&mcumgr_img_client_grp_sem, K_FOREVER);
		if (image_upload_buf->status) {
			LOG_ERR("Upload Fail: %d", image_upload_buf->status);
			goto end;
		}

		if (offset_before_send + write_length < active_client->upload.offset) {
			/* Offset further than expected which indicate upload session resume */
			goto end;
		}

		wrote_length += write_length;
	}
end:
	rc = image_upload_buf->status;
	active_client = NULL;
	image_upload_buf = NULL;
	k_mutex_unlock(&mcumgr_img_client_grp_mutex);

	return rc;
}

int img_mgmt_client_state_write(struct img_mgmt_client *client, char *hash, bool confirm,
				struct mcumgr_image_state *res_buf)
{
	struct net_buf *nb;
	int rc;
	uint32_t map_count;
	zcbor_state_t zse[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS];
	bool ok;

	k_mutex_lock(&mcumgr_img_client_grp_mutex, K_FOREVER);
	active_client = client;
	image_info = res_buf;
	/* Init Response */
	res_buf->image_list_length = 0;
	res_buf->image_list = active_client->image_list;

	nb = smp_client_buf_allocation(active_client->smp_client, MGMT_GROUP_ID_IMAGE,
				       IMG_MGMT_ID_STATE, MGMT_OP_WRITE, SMP_MCUMGR_VERSION_1);
	if (!nb) {
		res_buf->status = MGMT_ERR_ENOMEM;
		goto end;
	}

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), nb->data + nb->len, net_buf_tailroom(nb), 0);
	if (hash) {
		map_count = 4;
	} else {
		map_count = 2;
	}

	/* Write map start init and confirm params */
	ok = zcbor_map_start_encode(zse, map_count) && zcbor_tstr_put_lit(zse, "confirm") &&
	     zcbor_bool_put(zse, confirm);
	/* Write hash data */
	if (ok && hash) {
		ok = zcbor_tstr_put_lit(zse, "hash") &&
		     zcbor_bstr_encode_ptr(zse, hash, IMG_MGMT_DATA_SHA_LEN);
	}
	/* Close map */
	if (ok) {
		ok = zcbor_map_end_encode(zse, map_count);
	}

	if (!ok) {
		smp_packet_free(nb);
		res_buf->status = MGMT_ERR_ENOMEM;
		goto end;
	}

	nb->len = zse->payload - nb->data;
	k_sem_reset(&mcumgr_img_client_grp_sem);
	rc = smp_client_send_cmd(active_client->smp_client, nb, image_state_res_fn,
				 &mcumgr_img_client_grp_sem, CONFIG_SMP_CMD_DEFAULT_LIFE_TIME);
	if (rc) {
		res_buf->status = rc;
		smp_packet_free(nb);
		goto end;
	}
	k_sem_take(&mcumgr_img_client_grp_sem, K_FOREVER);
end:
	rc = res_buf->status;
	active_client = NULL;
	k_mutex_unlock(&mcumgr_img_client_grp_mutex);
	return rc;
}

int img_mgmt_client_state_read(struct img_mgmt_client *client, struct mcumgr_image_state *res_buf)
{
	struct net_buf *nb;
	int rc;
	zcbor_state_t zse[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS];
	bool ok;

	k_mutex_lock(&mcumgr_img_client_grp_mutex, K_FOREVER);
	active_client = client;
	/* Init Response */
	res_buf->image_list_length = 0;
	res_buf->image_list = active_client->image_list;

	image_info = res_buf;

	nb = smp_client_buf_allocation(active_client->smp_client, MGMT_GROUP_ID_IMAGE,
				       IMG_MGMT_ID_STATE, MGMT_OP_READ, SMP_MCUMGR_VERSION_1);
	if (!nb) {
		res_buf->status = MGMT_ERR_ENOMEM;
		goto end;
	}

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), nb->data + nb->len, net_buf_tailroom(nb), 0);
	ok = zcbor_map_start_encode(zse, 1) && zcbor_map_end_encode(zse, 1);
	if (!ok) {
		smp_packet_free(nb);
		res_buf->status = MGMT_ERR_ENOMEM;
		goto end;
	}

	nb->len = zse->payload - nb->data;
	k_sem_reset(&mcumgr_img_client_grp_sem);
	rc = smp_client_send_cmd(active_client->smp_client, nb, image_state_res_fn,
				 &mcumgr_img_client_grp_sem, CONFIG_SMP_CMD_DEFAULT_LIFE_TIME);
	if (rc) {
		smp_packet_free(nb);
		res_buf->status = rc;
		goto end;
	}
	k_sem_take(&mcumgr_img_client_grp_sem, K_FOREVER);
end:
	rc = res_buf->status;
	image_info = NULL;
	active_client = NULL;
	k_mutex_unlock(&mcumgr_img_client_grp_mutex);
	return rc;
}

int img_mgmt_client_erase(struct img_mgmt_client *client, uint32_t slot)
{
	struct net_buf *nb;
	int rc;
	zcbor_state_t zse[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS];
	bool ok;

	k_mutex_lock(&mcumgr_img_client_grp_mutex, K_FOREVER);
	active_client = client;

	nb = smp_client_buf_allocation(active_client->smp_client, MGMT_GROUP_ID_IMAGE,
				       IMG_MGMT_ID_ERASE, MGMT_OP_WRITE, SMP_MCUMGR_VERSION_1);
	if (!nb) {
		active_client->status = MGMT_ERR_ENOMEM;
		goto end;
	}

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), nb->data + nb->len, net_buf_tailroom(nb), 0);

	ok = zcbor_map_start_encode(zse, 2) && zcbor_tstr_put_lit(zse, "slot") &&
	     zcbor_uint32_put(zse, slot) && zcbor_map_end_encode(zse, 2);
	if (!ok) {
		smp_packet_free(nb);
		active_client->status = MGMT_ERR_ENOMEM;
		goto end;
	}

	nb->len = zse->payload - nb->data;
	k_sem_reset(&mcumgr_img_client_grp_sem);
	rc = smp_client_send_cmd(client->smp_client, nb, erase_res_fn, &mcumgr_img_client_grp_sem,
				 CONFIG_MCUMGR_GRP_IMG_FLASH_OPERATION_TIMEOUT);
	if (rc) {
		smp_packet_free(nb);
		active_client->status = rc;
		goto end;
	}
	k_sem_take(&mcumgr_img_client_grp_sem, K_FOREVER);
end:
	rc = active_client->status;
	active_client = NULL;
	k_mutex_unlock(&mcumgr_img_client_grp_mutex);
	return rc;
}
