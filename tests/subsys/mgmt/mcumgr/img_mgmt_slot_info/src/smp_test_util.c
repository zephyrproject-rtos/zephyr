/*
 * Copyright (c) 2022-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smp_test_util.h"
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
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
		.nh_group = sys_cpu_to_be16(MGMT_GROUP_ID_IMAGE),
		.nh_seq = 1,
		.nh_id = type,
		.nh_version = 1,
	};
}

bool create_img_mgmt_slot_info_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				      uint16_t *buffer_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2) &&
	     zcbor_map_end_encode(zse, 2);

	if (!ok) {
		return false;
	}

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, IMG_MGMT_ID_SLOT_INFO, false);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return true;
}
