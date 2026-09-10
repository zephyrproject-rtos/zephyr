/*
 * Copyright (c) 2022-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smp_test_util.h"
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zcbor_encode.h>

/* SMP header function for generating MCUmgr command header with sequence number set to 1 */
static void smp_make_hdr(struct smp_hdr *rsp_hdr, uint16_t group, uint8_t op, size_t len,
			 uint8_t type)
{
	*rsp_hdr = (struct smp_hdr) {
		.nh_len = sys_cpu_to_be16(len),
		.nh_flags = 0,
		.nh_op = op,
		.nh_group = sys_cpu_to_be16(group),
		.nh_seq = 1,
		.nh_id = type,
		.nh_version = 1,
	};
}

bool create_os_mgmt_echo_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				uint16_t *buffer_size, char *data)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, CBOR_MAP_STATES) &&
	     zcbor_tstr_put_lit(zse, "d")  &&
	     zcbor_tstr_put_term(zse, data, CONFIG_ZCBOR_MAX_STR_LEN) &&
	     zcbor_map_end_encode(zse, CBOR_MAP_STATES);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, MGMT_GROUP_ID_OS, MGMT_OP_READ, *buffer_size,
		     OS_MGMT_ID_ECHO);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_os_mgmt_echo_response_packet(zcbor_state_t *zse, uint8_t *buffer,
					 uint8_t *output_buffer, uint16_t *buffer_size, char *data)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, CBOR_MAP_STATES) &&
	     zcbor_tstr_put_lit(zse, "d")  &&
	     zcbor_tstr_put_term(zse, data, CONFIG_ZCBOR_MAX_STR_LEN) &&
	     zcbor_map_end_encode(zse, CBOR_MAP_STATES);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, MGMT_GROUP_ID_OS, MGMT_OP_READ_RSP,
		     *buffer_size, OS_MGMT_ID_ECHO);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_transport_mgmt_connect_packet(zcbor_state_t *zse, uint8_t *buffer,
					  uint8_t *output_buffer, uint16_t *buffer_size,
					  uint8_t transport_id)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, CBOR_MAP_STATES) &&
	     zcbor_tstr_put_lit(zse, "transport")  &&
	     zcbor_uint32_put(zse, transport_id) &&
	     zcbor_map_end_encode(zse, CBOR_MAP_STATES);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, MGMT_GROUP_ID_TRANSPORT, MGMT_OP_WRITE,
		     *buffer_size, TRANSPORT_MGMT_ID_CONNECT);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_transport_mgmt_disconnect_packet(zcbor_state_t *zse, uint8_t *buffer,
					     uint8_t *output_buffer, uint16_t *buffer_size,
					     uint8_t transport_id, bool all)
{
	bool ok;

	if (all) {
		ok = zcbor_map_start_encode(zse, CBOR_MAP_STATES) &&
		     zcbor_tstr_put_lit(zse, "all")  &&
		     zcbor_bool_put(zse, true) &&
		     zcbor_map_end_encode(zse, CBOR_MAP_STATES);
	} else {
		ok = zcbor_map_start_encode(zse, CBOR_MAP_STATES) &&
		     zcbor_tstr_put_lit(zse, "transport")  &&
		     zcbor_uint32_put(zse, transport_id) &&
		     zcbor_map_end_encode(zse, CBOR_MAP_STATES);
	}

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, MGMT_GROUP_ID_TRANSPORT, MGMT_OP_WRITE,
		     *buffer_size, TRANSPORT_MGMT_ID_DISCONNECT);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_transport_mgmt_status_packet(zcbor_state_t *zse, uint8_t *buffer,
					 uint8_t *output_buffer, uint16_t *buffer_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, CBOR_MAP_STATES) &&
	     zcbor_map_end_encode(zse, CBOR_MAP_STATES);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, MGMT_GROUP_ID_TRANSPORT, MGMT_OP_READ,
		     *buffer_size, TRANSPORT_MGMT_ID_STATUS);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_transport_mgmt_transports_packet(zcbor_state_t *zse, uint8_t *buffer,
					     uint8_t *output_buffer, uint16_t *buffer_size)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, CBOR_MAP_STATES) &&
	     zcbor_map_end_encode(zse, CBOR_MAP_STATES);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, MGMT_GROUP_ID_TRANSPORT, MGMT_OP_READ,
		     *buffer_size, TRANSPORT_MGMT_ID_LIST);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_transport_mgmt_modes_packet(zcbor_state_t *zse, uint8_t *buffer,
					uint8_t *output_buffer, uint16_t *buffer_size,
					uint32_t transport_id)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, CBOR_MAP_STATES) &&
	     zcbor_tstr_put_lit(zse, "transport")  &&
	     zcbor_uint32_put(zse, transport_id) &&
	     zcbor_map_end_encode(zse, CBOR_MAP_STATES);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, MGMT_GROUP_ID_TRANSPORT, MGMT_OP_READ,
		     *buffer_size, TRANSPORT_MGMT_ID_GET_MODES);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}

bool create_transport_mgmt_config_details_packet(zcbor_state_t *zse, uint8_t *buffer,
						 uint8_t *output_buffer, uint16_t *buffer_size,
						 uint32_t transport_id, uint32_t type)
{
	bool ok;

	ok = zcbor_map_start_encode(zse, CBOR_MAP_STATES) &&
	     zcbor_tstr_put_lit(zse, "transport")  &&
	     zcbor_uint32_put(zse, transport_id) &&
	     zcbor_tstr_put_lit(zse, "mode")  &&
	     zcbor_uint32_put(zse, type) &&
	     zcbor_map_end_encode(zse, CBOR_MAP_STATES);

	*buffer_size = (zse->payload_mut - buffer);
	smp_make_hdr((struct smp_hdr *)output_buffer, MGMT_GROUP_ID_TRANSPORT, MGMT_OP_READ,
		     *buffer_size, TRANSPORT_MGMT_ID_GET_CONFIG_DETAILS);
	memcpy(&output_buffer[sizeof(struct smp_hdr)], buffer, *buffer_size);
	*buffer_size += sizeof(struct smp_hdr);

	return ok;
}
