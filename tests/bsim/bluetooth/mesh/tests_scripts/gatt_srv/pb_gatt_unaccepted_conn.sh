#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that the PB-GATT Server refuses writes on a link that it did not accept.
#
# Test procedure:
# 1. The DUT enables the PB-GATT bearer, which registers the Mesh Provisioning Service, and
#    connects to the tester as central. The PB-GATT Server accepts only links in the
#    peripheral role, so it has no client.
# 2. The tester writes a complete Proxy PDU to Mesh Provisioning Data In, then enables
#    notifications of Mesh Provisioning Data Out. The CCC write must fail, and the CCC must
#    still read as disabled.
# 3. The tester disconnects. The DUT passes when it sees the disconnection.

overlay=overlay_gatt_conf
RunTest mesh_gatt_srv_pb_gatt_unaccepted_conn gatt_srv_dut_pb_gatt gatt_srv_tester_pb_gatt
