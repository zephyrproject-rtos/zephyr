/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/ztest.h>

#include "gptp_messages.h"
#include "gptp_data_set.h"
#include "gptp_private.h"

/* Exposed only in Ztest builds. No Ethernet device is registered, so the
 * background gPTP thread has no ports to process in this isolated fixture.
 */
void gptp_md_sync_send_state_machine(int port);

static struct net_pkt *pending;

static void before(void *fixture)
{
	struct gptp_sync_send_state *state = &GPTP_PORT_STATE(1)->sync_send;
	struct gptp_port_ds *port = GPTP_PORT_DS(1);

	ARG_UNUSED(fixture);
	memset(state, 0, sizeof(*state));
	port->ptt_port_enabled = true;
	port->as_capable = true;
	pending = net_pkt_alloc(K_NO_WAIT);
	zassert_not_null(pending);
	/* Keep an observer reference to check that the state releases only its
	 * own reference when discarding the timestamp-less packet.
	 */
	state->sync_ptr = net_pkt_ref(pending);
	state->state = GPTP_SYNC_SEND_SEND_FUP;
}

static void after(void *fixture)
{
	struct gptp_sync_send_state *state = &GPTP_PORT_STATE(1)->sync_send;

	ARG_UNUSED(fixture);
	gptp_sync_timestamp_cb_unregister(1);
	net_pkt_unref(state->sync_ptr);
	state->sync_ptr = NULL;
	net_pkt_unref(pending);
}

ZTEST(gptp_sync_send, test_fresh_sync_supersedes_missing_timestamp)
{
	struct gptp_sync_send_state *state = &GPTP_PORT_STATE(1)->sync_send;

	state->rcvd_md_sync = true;
	gptp_md_sync_send_state_machine(1);

	zassert_equal(state->state, GPTP_SYNC_SEND_SEND_SYNC);
	zassert_true(state->rcvd_md_sync, "Fresh sample must remain pending for transmission");
	zassert_is_null(state->sync_ptr);
	zassert_false(state->md_sync_timestamp_avail);
	zassert_equal(atomic_get(&pending->atomic_ref), 1);
}

ZTEST(gptp_sync_send, test_waits_for_timestamp_without_fresh_sync)
{
	struct gptp_sync_send_state *state = &GPTP_PORT_STATE(1)->sync_send;

	gptp_md_sync_send_state_machine(1);

	zassert_equal(state->state, GPTP_SYNC_SEND_SEND_FUP);
	zassert_equal_ptr(state->sync_ptr, pending);
	zassert_equal(atomic_get(&pending->atomic_ref), 2);
}

ZTEST(gptp_sync_send, test_peer_loss_discards_pending_and_fresh_sync)
{
	struct gptp_sync_send_state *state = &GPTP_PORT_STATE(1)->sync_send;

	state->rcvd_md_sync = true;
	GPTP_PORT_DS(1)->as_capable = false;
	gptp_md_sync_send_state_machine(1);

	zassert_equal(state->state, GPTP_SYNC_SEND_INITIALIZING);
	zassert_false(state->rcvd_md_sync);
	zassert_is_null(state->sync_ptr);
	zassert_equal(atomic_get(&pending->atomic_ref), 1);
}

ZTEST_SUITE(gptp_sync_send, NULL, NULL, before, after, NULL);
