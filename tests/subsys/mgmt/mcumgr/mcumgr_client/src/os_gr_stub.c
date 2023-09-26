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
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt_client.h>
#include <zephyr/sys/byteorder.h>

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#include "smp_stub.h"

static const char *echo_ptr;

void os_stub_init(const char *echo_str)
{
	echo_ptr = echo_str;
}

void os_reset_response(void)
{
	struct net_buf *nb;

	nb = smp_response_buf_allocation();
	if (nb) {
		nb->len = 0;
	}
}

static void os_echo_response(int status, struct zcbor_string *echo_data)
{
	struct net_buf *nb;
	zcbor_state_t zse[3 + 2];
	bool ok;

	nb = smp_response_buf_allocation();

	if (!nb) {
		return;
	}

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), nb->data, net_buf_tailroom(nb), 0);

	if (status) {
		/* Init map start and write image info and data */
		ok = zcbor_map_start_encode(zse, 2) && zcbor_tstr_put_lit(zse, "rc") &&
		     zcbor_int32_put(zse, status) && zcbor_map_end_encode(zse, 2);
	} else {
		/* Init map start and write image info and data */
		ok = zcbor_map_start_encode(zse, 2) && zcbor_tstr_put_lit(zse, "r") &&
		     zcbor_tstr_encode_ptr(zse, echo_data->value, echo_data->len) &&
		     zcbor_map_end_encode(zse, 2);
	}

	if (!ok) {
		smp_client_response_buf_clean();
	} else {
		nb->len = zse->payload - nb->data;
	}
}

void os_echo_verify(struct net_buf *nb)
{
	/* Parse CBOR data: hash and confirm */
	zcbor_state_t zsd[3 + 2];
	int rc;
	int response_status;
	struct zcbor_string echo_data;
	size_t decoded;
	struct zcbor_map_decode_key_val list_res_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("d", zcbor_tstr_decode, &echo_data)
		};

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), nb->data + sizeof(struct smp_hdr), nb->len, 1);
	echo_data.len = 0;

	rc = zcbor_map_decode_bulk(zsd, list_res_decode, ARRAY_SIZE(list_res_decode), &decoded);
	if (rc || !echo_data.len) {
		printf("Corrupted data %d or no echo data %d\r\n", rc, echo_data.len);
		response_status = MGMT_ERR_EINVAL;
	} else if (memcmp(echo_data.value, echo_ptr, echo_data.len)) {
		response_status = MGMT_ERR_EINVAL;
	} else {
		response_status = MGMT_ERR_EOK;
	}

	os_echo_response(response_status, &echo_data);
}
