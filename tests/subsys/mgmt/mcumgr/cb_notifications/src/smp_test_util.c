/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smp_test_util.h"
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zcbor_encode.h>

void smp_make_hdr(struct smp_hdr *rsp_hdr, size_t len)
{
	*rsp_hdr = (struct smp_hdr) {
		.nh_len = sys_cpu_to_be16(len),
		.nh_flags = 0,
		.nh_op = 0,
		.nh_group = sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		.nh_seq = 1,
		.nh_id = OS_MGMT_ID_ECHO,
	};
}

bool create_mcumgr_format_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				 uint16_t *buffer_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2)		&&
	     zcbor_tstr_put_lit(zse, "d")		&&
	     zcbor_tstr_put_lit(zse, "some test data")	&&
	     zcbor_map_end_encode(zse, 2);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}
