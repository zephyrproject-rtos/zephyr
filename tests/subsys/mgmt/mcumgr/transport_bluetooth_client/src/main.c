/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bluetooth client SMP transport checks that need no peer: registration, and the
 * behavior of an idle transport. The Bluetooth host is built but never enabled. Discovery,
 * subscription and fragmentation are covered against a stubbed GATT layer in
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
#include <zephyr/mgmt/mcumgr/transport/smp_bt_client.h>

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
#error "Expected Kconfig option CONFIG_MCUMGR_TRANSPORT_BT_CLIENT not enabled"
#endif

/* The transport must select the reassembly context, which has no prompt */
#if !defined(CONFIG_MCUMGR_TRANSPORT_REASSEMBLY)
#error "CONFIG_MCUMGR_TRANSPORT_BT_CLIENT must select CONFIG_MCUMGR_TRANSPORT_REASSEMBLY"
#endif

/* The server transport registers under SMP_BLUETOOTH_TRANSPORT as well */
BUILD_ASSERT(SMP_BLUETOOTH_CLIENT_TRANSPORT != SMP_BLUETOOTH_TRANSPORT,
	     "Bluetooth client transport must not reuse the SMP server transport type");
BUILD_ASSERT(SMP_BLUETOOTH_CLIENT_TRANSPORT < SMP_USER_DEFINED_TRANSPORT,
	     "Bluetooth client transport must not take the user defined transport type");

static struct smp_transport *client_transport(void)
{
	return smp_client_transport_get(SMP_BLUETOOTH_CLIENT_TRANSPORT);
}

ZTEST(mcumgr_transport_bt_client, test_transport_registered_at_boot)
{
	struct smp_transport *smpt = client_transport();

	zassert_not_null(smpt, "Bluetooth client transport did not register itself");
	zassert_not_null(smpt->functions.output, "Transport registered with no transmit function");
	zassert_not_null(smpt->functions.get_mtu, "Transport registered with no MTU function");
}

/* A device may be an SMP server to one peer and an SMP client to another */
ZTEST(mcumgr_transport_bt_client, test_server_and_client_transports_are_distinct)
{
	struct smp_transport *server;
	struct smp_transport *client;

	Z_TEST_SKIP_IFNDEF(CONFIG_MCUMGR_TRANSPORT_BT);

	server = smp_client_transport_get(SMP_BLUETOOTH_TRANSPORT);
	client = client_transport();

	zassert_not_null(server, "SMP server transport did not register itself");
	zassert_not_null(client, "Bluetooth client transport did not register itself");
	zassert_true(server != client,
		     "SMP server and Bluetooth client resolved to the same transport");
}

ZTEST(mcumgr_transport_bt_client, test_smp_client_binds_to_transport)
{
	struct smp_client_object client;
	int rc;

	memset(&client, 0, sizeof(client));

	rc = smp_client_object_init(&client, SMP_BLUETOOTH_CLIENT_TRANSPORT);
	zassert_equal(rc, MGMT_ERR_EOK, "SMP client could not bind to the transport (err %d)", rc);
	zassert_equal(client.smpt, client_transport(),
		      "SMP client bound to a different transport than the one registered");
}

/* The transport has no user data hooks, so request buffers go straight back to the pool */
ZTEST(mcumgr_transport_bt_client, test_smp_client_request_buffer_round_trip)
{
	struct smp_client_object client;
	struct net_buf *nb;
	size_t available;

	memset(&client, 0, sizeof(client));
	zassert_equal(smp_client_object_init(&client, SMP_BLUETOOTH_CLIENT_TRANSPORT), MGMT_ERR_EOK,
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

ZTEST(mcumgr_transport_bt_client, test_not_attached_at_boot)
{
	zassert_false(smp_bt_client_is_attached(), "Transport reported a target before any attach");
}

ZTEST(mcumgr_transport_bt_client, test_attach_rejects_null_connection)
{
	int rc = smp_bt_client_attach(NULL, K_NO_WAIT);

	zassert_equal(rc, -EINVAL, "Attaching without a connection returned %d, expected -EINVAL",
		      rc);
	zassert_false(smp_bt_client_is_attached(), "A rejected attach left a target behind");
}

ZTEST(mcumgr_transport_bt_client, test_detach_when_idle_is_safe)
{
	smp_bt_client_detach();
	zassert_false(smp_bt_client_is_attached(), "Detaching an idle transport attached one");

	smp_bt_client_detach();
	zassert_false(smp_bt_client_is_attached(), "A second detach attached a target");

	/* Detach must not leave the transport wedged for the next caller. */
	zassert_equal(smp_bt_client_attach(NULL, K_NO_WAIT), -EINVAL,
		      "Attach stopped validating its arguments after a detach");
}

ZTEST(mcumgr_transport_bt_client, test_mtu_is_zero_while_detached)
{
	struct smp_transport *smpt = client_transport();

	zassert_not_null(smpt, "Bluetooth client transport did not register itself");
	zassert_equal(smpt->functions.get_mtu(NULL), 0,
		      "Transport advertised an MTU with no target attached");
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
	zassert_false(smp_bt_client_is_attached(), "A target was attached before the test ran");

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
		zassert_not_equal(rc, MGMT_ERR_EOK,
				  "Transmit reported success with no target attached");
	}

	zassert_equal(smp_packet_buffers_available(), available,
		      "Refused transmits did not return every buffer to the pool");
}

ZTEST_SUITE(mcumgr_transport_bt_client, NULL, NULL, NULL, NULL, NULL);
