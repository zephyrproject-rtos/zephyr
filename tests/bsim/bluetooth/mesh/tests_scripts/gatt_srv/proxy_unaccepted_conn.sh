#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that the Proxy Server refuses writes on a link that it did not accept.
#
# Test procedure:
# 1. The DUT provisions itself, which registers the Mesh Proxy Service, and connects to the
#    tester as central. The Proxy Server accepts only links in the peripheral role.
# 2. The tester writes a complete Proxy PDU to Mesh Proxy Data In, then enables notifications
#    of Mesh Proxy Data Out. The CCC write must fail, and the CCC must still read as disabled.
# 3. The tester disconnects. The DUT passes when it sees the disconnection.

overlay=overlay_gatt_conf
RunTest mesh_gatt_srv_proxy_unaccepted_conn gatt_srv_dut_proxy gatt_srv_tester_proxy
