/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2023 Jamie M.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smp_test_util.h"
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zcbor_encode.h>

/* SMP header function for generating os_mgmt datetime command header with sequence number set
 * to 1
 */
void smp_make_hdr(struct smp_hdr *rsp_hdr, size_t len, bool version2, bool write)
{
	*rsp_hdr = (struct smp_hdr) {
		.nh_len = sys_cpu_to_be16(len),
		.nh_flags = 0,
		.nh_version = (version2 == true ? SMP_MCUMGR_VERSION_2 : SMP_MCUMGR_VERSION_1),
		.nh_op = (write == true ? MGMT_OP_WRITE : MGMT_OP_READ),
		.nh_group = sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		.nh_seq = 1,
		.nh_id = OS_MGMT_ID_DATETIME_STR,
	};
}

/* Function for creating an os_mgmt datetime get command */
bool create_mcumgr_datetime_get_packet(zcbor_state_t *zse, bool version2, uint8_t *buffer,
				       uint8_t *output_buffer, uint16_t *buffer_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, 2)	&&
	     zcbor_map_end_encode(zse, 2);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, version2, false);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

/* Functions for creating an os_mgmt datetime set command */
bool create_mcumgr_datetime_set_packet_str(zcbor_state_t *zse, bool version2, const char *data,
					   uint8_t *buffer, uint8_t *output_buffer,
					   uint16_t *buffer_size)
{
	bool ok = zcbor_map_start_encode(zse, 2)			&&
	     zcbor_tstr_put_lit(zse, "datetime")			&&
	     zcbor_tstr_put_term(zse, data, CONFIG_ZCBOR_MAX_STR_LEN)	&&
	     zcbor_map_end_encode(zse, 2);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, *buffer_size, version2, true);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_mcumgr_datetime_set_packet(zcbor_state_t *zse, bool version2, struct rtc_time *a_time,
				       uint8_t *buffer, uint8_t *output_buffer,
				       uint16_t *buffer_size)
{
	char tmp_str[32];

	sprintf(tmp_str, "%4d-%02d-%02dT%02d:%02d:%02d", (uint16_t)a_time->tm_year,
		(uint8_t)a_time->tm_mon, (uint8_t)a_time->tm_mday, (uint8_t)a_time->tm_hour,
		(uint8_t)a_time->tm_min, (uint8_t)a_time->tm_sec);

	return create_mcumgr_datetime_set_packet_str(zse, version2, tmp_str, buffer,
						     output_buffer, buffer_size);
}
