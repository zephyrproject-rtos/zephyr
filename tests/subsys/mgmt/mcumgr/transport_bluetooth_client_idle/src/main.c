/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bluetooth client SMP transport checks that need no peer: registration, and the
 * behavior of an idle transport. The Bluetooth host is built but never enabled. Discovery,
 * subscription, fragmentation and bridging are covered against a stubbed GATT layer in
 * tests/subsys/mgmt/mcumgr/transport_bluetooth_client_stub.
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt_defines.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#endif

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
#error "Expected Kconfig option CONFIG_MCUMGR_TRANSPORT_BT_CLIENT not enabled"
#endif

/* The transport must select the reassembly context, which has no prompt */
#if !defined(CONFIG_MCUMGR_TRANSPORT_REASSEMBLY)
#error "CONFIG_MCUMGR_TRANSPORT_BT_CLIENT must select CONFIG_MCUMGR_TRANSPORT_REASSEMBLY"
#endif

#if !defined(CONFIG_SMP_CLIENT) && !defined(CONFIG_MCUMGR_GRP_TRANSPORT)
#error "CONFIG_MCUMGR_TRANSPORT_BT_CLIENT needs the SMP client or transport management"
#endif

static struct smp_transport *client_transport(void)
{
	return smp_client_transport_get(SMP_BLUETOOTH_TRANSPORT);
}

struct registry_scan {
	int entries;
	const struct smp_client_transport_entry *entry;
};

static bool registry_scan_cb(const struct smp_client_transport_entry *entry, void *user_data)
{
	struct registry_scan *scan = user_data;

	if (entry->smpt_type == SMP_BLUETOOTH_TRANSPORT) {
		scan->entries++;
		scan->entry = entry;
	}

	return true;
}

ZTEST(mcumgr_transport_bt_client, test_transport_registered_at_boot)
{
	struct smp_transport *smpt = client_transport();

	zassert_not_null(smpt, "Bluetooth client transport did not register itself");
	zassert_not_null(smpt->functions.output, "Transport registered with no transmit function");

	/* get_mtu is deprecated, and the client sizes its writes from the ATT_MTU itself */
	TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	zassert_is_null(smpt->functions.get_mtu, "Transport set the deprecated get_mtu");
	TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
}

/*
 * The server and the client are separate transport objects, and only the client is in the
 * client registry. The server tags its packets with the connection they arrived on (user
 * data) and checks queries against it; the client has a single target and does neither, so
 * those hooks tell the two objects apart.
 */
ZTEST(mcumgr_transport_bt_client, test_registry_entry_is_the_client_transport)
{
	struct registry_scan scan = {0};
	struct smp_transport *smpt;

	(void)smp_client_transport_foreach(registry_scan_cb, &scan);

	zassert_equal(scan.entries, 1, "Expected one SMP_BLUETOOTH_TRANSPORT entry, found %d",
		      scan.entries);

	smpt = scan.entry->smpt;
	zassert_equal_ptr(smpt, client_transport(), "Lookup and iteration disagree");
	zassert_is_null(smpt->functions.ud_copy, "Registered transport copies user data");
	zassert_is_null(smpt->functions.ud_free, "Registered transport frees user data");
	zassert_is_null(smpt->functions.query_valid_check,
			"Registered transport checks query validity like the server");

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS
	zassert_not_null(scan.entry->name, "Registered transport has no name");
	zassert_str_equal(scan.entry->name, "Bluetooth", "Registered transport was renamed");
#endif

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
	zassert_not_null(smpt->functions.bridge_connect, "No outgoing bridge connect");
	zassert_not_null(smpt->functions.bridge_disconnect, "No outgoing bridge disconnect");
	zassert_not_null(smpt->functions.bridge_output, "No outgoing bridge output");
#endif
}

#ifdef CONFIG_SMP_CLIENT
ZTEST(mcumgr_transport_bt_client, test_smp_client_binds_to_transport)
{
	struct smp_client_object client;
	int rc;

	memset(&client, 0, sizeof(client));

	rc = smp_client_object_init(&client, SMP_BLUETOOTH_TRANSPORT);
	zassert_equal(rc, MGMT_ERR_EOK, "SMP client could not bind to the transport (err %d)", rc);
	zassert_equal(client.smpt, client_transport(),
		      "SMP client bound to a different transport than the one registered");
}

/* The transport has no ud_free hook, so request buffers go straight back to the pool */
ZTEST(mcumgr_transport_bt_client, test_smp_client_request_buffer_round_trip)
{
	struct smp_client_object client;
	struct net_buf *nb;
	size_t available;

	memset(&client, 0, sizeof(client));
	zassert_equal(smp_client_object_init(&client, SMP_BLUETOOTH_TRANSPORT), MGMT_ERR_EOK,
		      "SMP client could not bind to the transport");

	available = smp_packet_buffers_available();
	zassert_true(available > 0, "No packet buffers available before the test ran");

	nb = smp_client_buf_allocation(&client, MGMT_GROUP_ID_OS, 0, MGMT_OP_READ,
				       SMP_MCUMGR_VERSION_1);
	zassert_not_null(nb, "SMP client could not allocate a request buffer");
	zassert_equal(smp_packet_buffers_available(), available - 1,
		      "Allocating a request did not take a buffer from the pool");

	smp_client_buf_free(nb);
	zassert_equal(smp_packet_buffers_available(), available,
		      "Freeing a request did not return its buffer to the pool");
}
#endif /* CONFIG_SMP_CLIENT */

ZTEST(mcumgr_transport_bt_client, test_not_attached_at_boot)
{
	zassert_false(smp_client_bt_is_attached(), "Transport reported a target before any attach");
}

#ifdef CONFIG_SMP_CLIENT
ZTEST(mcumgr_transport_bt_client, test_attach_rejects_null_connection)
{
	int rc = smp_client_bt_attach(NULL, K_NO_WAIT);

	zassert_equal(rc, -EINVAL, "Attaching without a connection returned %d, expected -EINVAL",
		      rc);
	zassert_false(smp_client_bt_is_attached(), "A rejected attach left a target behind");
}
#else
/* The SMP client functions are not available when the transport is built for bridges */
ZTEST(mcumgr_transport_bt_client, test_attach_is_unavailable_with_bridges)
{
	int rc = smp_client_bt_attach(NULL, K_NO_WAIT);

	zassert_equal(rc, -ENOSYS, "Attaching in a bridge build returned %d, expected -ENOSYS",
		      rc);
}
#endif /* CONFIG_SMP_CLIENT */

ZTEST(mcumgr_transport_bt_client, test_detach_when_idle_is_safe)
{
	smp_client_bt_detach();
	zassert_false(smp_client_bt_is_attached(), "Detaching an idle transport attached one");

	smp_client_bt_detach();
	zassert_false(smp_client_bt_is_attached(), "A second detach attached a target");

	/* Detach must not leave the transport wedged for the next caller. */
	if (IS_ENABLED(CONFIG_SMP_CLIENT)) {
		zassert_equal(smp_client_bt_attach(NULL, K_NO_WAIT), -EINVAL,
			      "Attach stopped validating its arguments after a detach");
	}
}

/*
 * The transmit function releases the packet on every path, including a refusal with no
 * target. Running it more times than the pool has buffers turns a leak into a failure.
 */
ZTEST(mcumgr_transport_bt_client, test_transmit_while_detached_releases_the_packet)
{
	struct smp_transport *smpt = client_transport();
	size_t available;
	unsigned int i;

	zassert_not_null(smpt, "Bluetooth client transport did not register itself");
	zassert_false(smp_client_bt_is_attached(), "A target was attached before the test ran");

	available = smp_packet_buffers_available();
	zassert_true(available > 0, "No packet buffers available before the test ran");

	for (i = 0; i < (available * 4U); i++) {
		struct net_buf *nb = smp_packet_alloc();
		int rc;

		zassert_not_null(nb,
				 "Packet pool exhausted after %u transmits, the transmit "
				 "path leaked a buffer while refusing to send",
				 i);

		rc = smpt->functions.output(nb);
		zassert_equal(rc, MGMT_ERR_ENOENT,
			      "Transmit with no target returned %d, expected MGMT_ERR_ENOENT", rc);
	}

	zassert_equal(smp_packet_buffers_available(), available,
		      "Refused transmits did not return every buffer to the pool");
}

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
/* The outgoing bridge output also releases the packet when no bridge holds the transport */
ZTEST(mcumgr_transport_bt_client, test_bridge_output_while_idle_releases_the_packet)
{
	struct smp_transport *smpt = client_transport();
	size_t available = smp_packet_buffers_available();
	struct net_buf *nb = smp_packet_alloc();
	int rc;

	zassert_not_null(nb, "No packet buffer");

	rc = smpt->functions.bridge_output(NULL, nb, true);
	zassert_equal(rc, MGMT_ERR_BRIDGED_CONNECTION_UNAVAILABLE,
		      "Bridge output with no bridge returned %d", rc);
	zassert_equal(smp_packet_buffers_available(), available,
		      "Refused bridge output did not release the packet");
}
#endif /* CONFIG_MCUMGR_GRP_TRANSPORT */

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS
/*
 * The client transport is only ever the outgoing side. The incoming side of a Bluetooth bridge
 * is the server transport, so the modes report incoming support only when the server is built.
 */
ZTEST(mcumgr_transport_bt_client, test_bridge_modes_follow_the_server)
{
	struct smp_transport *smpt = client_transport();
	uint8_t buffer[96];
	zcbor_state_t zse[4];
	zcbor_state_t zsd[4];
	struct zcbor_string description = {0};
	uint32_t id = UINT32_MAX;
	bool incoming = false;
	bool outgoing = false;
	size_t decoded = 0;
	int rc = -1;

	struct zcbor_map_decode_key_val modes_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("id", zcbor_uint32_decode, &id),
		ZCBOR_MAP_DECODE_KEY_DECODER("description", zcbor_tstr_decode, &description),
		ZCBOR_MAP_DECODE_KEY_DECODER("incoming", zcbor_bool_decode, &incoming),
		ZCBOR_MAP_DECODE_KEY_DECODER("outgoing", zcbor_bool_decode, &outgoing),
	};

	zassert_not_null(smpt->functions.bridge_modes, "No bridge modes function");

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), buffer, sizeof(buffer), 0);
	zassert_true(smpt->functions.bridge_modes(zse, &rc), "Encoding the modes failed");
	zassert_equal(rc, MGMT_ERR_EOK, "Modes returned %d", rc);

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), buffer, zse->payload - buffer, 1, NULL, 0);
	zassert_ok(zcbor_map_decode_bulk(zsd, modes_decode, ARRAY_SIZE(modes_decode), &decoded),
		   "Decoding the modes failed");
	zassert_equal(decoded, ARRAY_SIZE(modes_decode), "Modes are missing a key");
	zassert_equal(id, 0, "Unexpected mode id %u", id);
	zassert_equal(incoming, IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT),
		      "Incoming support does not follow the server transport");
	zassert_true(outgoing, "Outgoing support not reported");
}
#endif /* CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS */

ZTEST_SUITE(mcumgr_transport_bt_client, NULL, NULL, NULL, NULL, NULL);
