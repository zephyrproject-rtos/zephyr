#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test a parallel work scanning and provisioning work on RPR client and server. Procedure:
# 1. A provisioner with the RPR client provisions itself;
# 2. The provisioner provisions a device with the RPR server;
# 3. The provisioner provisions an unprovisioned device idx 2 through the node with RPR server;
# 4. The provisioner scans for an unprovisioned device idx 3 through the node with RPR server;
# 5. The provisioner checks scanning and provisioning succeeded;
# 6. The provisioner provisions an unprovisioned device idx 3 through the node with RPR server;
RunTest mesh_prov_pb_remote_parallel \
	prov_provisioner_pb_remote_client_parallel \
	prov_device_pb_remote_server_unproved \
	prov_device_no_oob \
	prov_device_no_oob \
	-- -argstest prov-brearer=1

overlay=overlay_psa_conf
RunTest mesh_prov_pb_remote_parallel_psa \
	prov_provisioner_pb_remote_client_parallel \
	prov_device_pb_remote_server_unproved \
	prov_device_no_oob \
	prov_device_no_oob \
	-- -argstest prov-brearer=1
