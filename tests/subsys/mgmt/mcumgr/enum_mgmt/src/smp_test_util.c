/*
 * Copyright (c) 2022-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smp_test_util.h"
#include <zephyr/mgmt/mcumgr/grp/enum_mgmt/enum_mgmt.h>
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
		.nh_group = sys_cpu_to_be16(MGMT_GROUP_ID_ENUM),
		.nh_seq = 1,
		.nh_id = type,
		.nh_version = 1,
	};
}

bool create_enum_mgmt_count_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				   uint16_t *buffer_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2) &&
	     zcbor_map_end_encode(zse, 2);

	if (!ok) {
		return false;
	}

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, ENUM_MGMT_ID_COUNT, false);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return true;
}

bool create_enum_mgmt_list_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				  uint16_t *buffer_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2) &&
	     zcbor_map_end_encode(zse, 2);

	if (!ok) {
		return false;
	}

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, ENUM_MGMT_ID_LIST, false);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return true;
}

bool create_enum_mgmt_single_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				    uint16_t *buffer_size, uint32_t index)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2) &&
	     zcbor_tstr_put_lit(zse, "index") &&
	     zcbor_uint32_put(zse, index) &&
	     zcbor_map_end_encode(zse, 2);

	if (!ok) {
		return false;
	}

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, ENUM_MGMT_ID_SINGLE, false);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return true;
}

bool create_enum_mgmt_details_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				     uint16_t *buffer_size, uint16_t *groups, uint8_t groups_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2);

	if (groups_size > 0) {
		uint8_t i = 0;

		ok &= zcbor_tstr_put_lit(zse, "groups") &&
		      zcbor_list_start_encode(zse, groups_size);

		while (i < groups_size) {
			uint32_t group = (uint32_t)groups[i];

			ok &= zcbor_uint32_encode(zse, &group);
		}

		ok &= zcbor_list_end_encode(zse, groups_size);
	}

	ok &= zcbor_map_end_encode(zse, 2);

	if (!ok) {
		return false;
	}

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, ENUM_MGMT_ID_DETAILS, false);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return true;
}
