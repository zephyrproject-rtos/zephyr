/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/ztest.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, __wrap_bt_conn_le_create, const bt_addr_le_t *,
		const struct bt_conn_le_create_param *, const struct bt_le_conn_param *,
		struct bt_conn **);
FAKE_VALUE_FUNC(int, __wrap_bt_conn_disconnect, struct bt_conn *, uint8_t);

static void check_connect_failure(int error, uint32_t expected_error)
{
	struct smp_transport *transport = smp_client_transport_get(SMP_BLUETOOTH_TRANSPORT);
	struct smp_transport_bridge bridge = {0};
	uint8_t request[64];

	ZCBOR_STATE_E(encoder, 2, request, sizeof(request), 1);

	zassert_not_null(transport);
	RESET_FAKE(__wrap_bt_conn_le_create);
	RESET_FAKE(__wrap_bt_conn_disconnect);
	__wrap_bt_conn_le_create_fake.return_val = error;
	__wrap_bt_conn_disconnect_fake.return_val = -EINVAL;
	zassert_true(zcbor_map_start_encode(encoder, 1));
	zassert_true(zcbor_tstr_put_lit(encoder, "address"));
	zassert_true(zcbor_tstr_put_lit(encoder, "01:02:03:04:05:06"));
	zassert_true(zcbor_map_end_encode(encoder, 1));

	/* More attempts than slots must reach Bluetooth without an intervening disconnect. */
	for (size_t i = 0; i < CONFIG_BT_MAX_CONN + 1U; i++) {
		struct cbor_nb_reader reader = {0};
		struct cbor_nb_writer writer = {0};
		uint8_t response[64];

		zcbor_new_decode_state(reader.zs, ARRAY_SIZE(reader.zs), request,
				       encoder[0].payload - request, 1, NULL, 0);
		zcbor_new_encode_state(writer.zs, ARRAY_SIZE(writer.zs), response, sizeof(response),
				       1);
		zassert_true(zcbor_map_start_encode(writer.zs, 1));
		zassert_false(transport->functions.bridge_connect(&bridge, true, 0, false,
								  reader.zs, writer.zs));
		zassert_equal(__wrap_bt_conn_le_create_fake.call_count, i + 1U);
		zassert_true(zcbor_map_end_encode(writer.zs, 1));

		ZCBOR_STATE_D(decoder, 3, response, writer.zs[0].payload - response, 1, 0);

		zassert_true(zcbor_map_start_decode(decoder));
		zassert_true(zcbor_tstr_expect_lit(decoder, "err"));
		zassert_true(zcbor_map_start_decode(decoder));
		zassert_true(zcbor_tstr_expect_lit(decoder, "group"));
		zassert_true(zcbor_uint32_expect(decoder, MGMT_GROUP_ID_TRANSPORT));
		zassert_true(zcbor_tstr_expect_lit(decoder, "rc"));
		zassert_true(zcbor_uint32_expect(decoder, expected_error));
		zassert_true(zcbor_map_end_decode(decoder));
		zassert_true(zcbor_map_end_decode(decoder), "duplicate error in response");
	}

	transport->functions.bridge_disconnect(&bridge, true);
	zassert_equal(__wrap_bt_conn_disconnect_fake.call_count, 0,
		      "failed connection left an outgoing bridge slot");
}

ZTEST(bluetooth_client_transport, test_connect_invalid_parameter)
{
	check_connect_failure(-EINVAL, TRANSPORT_MGMT_ERR_CONNECT_UNSUPPORTED_PARAMETER);
}

ZTEST(bluetooth_client_transport, test_connect_io_error)
{
	check_connect_failure(-EIO, TRANSPORT_MGMT_ERR_CONNECT_UNSUPPORTED_PARAMETER);
}

ZTEST(bluetooth_client_transport, test_connect_other_error)
{
	check_connect_failure(-ENOMEM, TRANSPORT_MGMT_ERR_CONNECT_FAILED);
}

ZTEST_SUITE(bluetooth_client_transport, NULL, NULL, NULL, NULL, NULL);
