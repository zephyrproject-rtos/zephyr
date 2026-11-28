#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Aurelien Rivet
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that the Proxy Server frees the advertising buffers of relayed messages that ATT still
# held for its client when the link was lost.
#
# Test procedure:
# 1. The tester connects to the DUT as Proxy Client and sends messages to the DUT until the DUT
#    replies. The first message the DUT receives adds the tester to the DUT's Proxy filter.
# 2. The tester leaves the simulated PHY without terminating the link.
# 3. The DUT sends messages to the tester, which it relays to the tester's link.
# 4. The DUT loses the link on the supervision timeout, then allocates every advertising buffer
#    of its pool.

overlay=overlay_gatt_conf
RunTest mesh_gatt_srv_proxy_relay_link_loss \
	gatt_srv_dut_proxy_relay_link_loss \
	gatt_srv_tester_proxy_relay_link_loss -disconnect_on_exit=1
