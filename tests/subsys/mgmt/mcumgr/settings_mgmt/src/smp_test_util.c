/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smp_test_util.h"
#include <zephyr/mgmt/mcumgr/grp/settings_mgmt/settings_mgmt.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zcbor_encode.h>

/* SMP header function for generating MCUmgr command header with sequence number set to 1 */
static void smp_make_hdr(struct smp_hdr *rsp_hdr, size_t len, uint8_t type, bool write)
{
	*rsp_hdr = (struct smp_hdr) {
		.nh_len = sys_cpu_to_be16(len),
		.nh_flags = 0,
		.nh_op = (write ? MGMT_OP_WRITE : MGMT_OP_READ),
		.nh_group = sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		.nh_seq = 1,
		.nh_id = type,
		.nh_version = 1,
	};
}

bool create_settings_mgmt_read_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				      uint16_t *buffer_size, char *name, uint32_t max_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2)				&&
	     zcbor_tstr_put_lit(zse, "name")				&&
	     zcbor_tstr_put_term(zse, name, CONFIG_ZCBOR_MAX_STR_LEN)	&&
	     (max_size == 0 || (zcbor_tstr_put_lit(zse, "max_size")	&&
	      zcbor_uint32_put(zse, max_size)))				&&
	     zcbor_map_end_encode(zse, 2);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, SETTINGS_MGMT_ID_READ_WRITE,
		     false);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_settings_mgmt_write_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				       uint16_t *buffer_size, char *name, const uint8_t *val,
				       size_t val_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2)				&&
	     zcbor_tstr_put_lit(zse, "name")				&&
	     zcbor_tstr_put_term(zse, name, CONFIG_ZCBOR_MAX_STR_LEN)	&&
	     zcbor_tstr_put_lit(zse, "val")				&&
	     zcbor_bstr_encode_ptr(zse, val, val_size)			&&
	     zcbor_map_end_encode(zse, 2);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, SETTINGS_MGMT_ID_READ_WRITE,
		     true);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_settings_mgmt_delete_packet(zcbor_state_t *zse, uint8_t *buffer,
					uint8_t *output_buffer, uint16_t *buffer_size, char *name)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2)				&&
	     zcbor_tstr_put_lit(zse, "name")				&&
	     zcbor_tstr_put_term(zse, name, CONFIG_ZCBOR_MAX_STR_LEN)	&&
	     zcbor_map_end_encode(zse, 2);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, SETTINGS_MGMT_ID_DELETE, true);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_settings_mgmt_commit_packet(zcbor_state_t *zse, uint8_t *buffer,
					uint8_t *output_buffer, uint16_t *buffer_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2)	&&
	     zcbor_map_end_encode(zse, 2);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, SETTINGS_MGMT_ID_COMMIT, true);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_settings_mgmt_load_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				      uint16_t *buffer_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2)	&&
	     zcbor_map_end_encode(zse, 2);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, SETTINGS_MGMT_ID_LOAD_SAVE,
		     false);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_settings_mgmt_save_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				      uint16_t *buffer_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2)	&&
	     zcbor_map_end_encode(zse, 2);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, SETTINGS_MGMT_ID_LOAD_SAVE,
		     true);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}
