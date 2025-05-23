#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test timeout on RPR Client and server during remote provisioning procedure.
# Devices:
# 1. Tester node with RPR Client, provisioner
# 2. Unprovisioned device with RPR server, provisionee
# 3. Unprovisioned device, provisionee
# Procedure
# 1. All devices start as unprovisioned;
# 2. 1st device self provisions and provisions 2nd device over PB-Adv.
# 3. 3rd device enables Mesh and starts advertising unprovisioned device beacons.
# 4. 3rd device disables Mesh scan while still advertising. Mesh stack remains active.
# 5. RPR Client on 1st device requests 2nd device to perform RPR Scan for UUID of 3rd device.
# 6. Unprovisioned device beacon of 3rd device gets reported to RPR Client.
# 7. RPR Client starts Remote Provisioning procedure for 3rd device.
# 8. Remote provisioning timeouts of 10s is reached on RPR Server. 2nd device sends Link Report
#    with status BT_MESH_RPR_ERR_LINK_OPEN_FAILED to 1st device, which closes provisioning link.
# 9. 3rd device enables Mesh scan again and becomes responsive.
# 10. RPR Client restarts provisioning.
# 11. 3rd device opens provisioning link.
# 12. 2nd device stops communicating with either devices.
# 13. After 60s RPR timeout is reached on 1st device. RPR Client closes provisioning link.
RunTest mesh_prov_pb_remote_provisioning_timeout \
	prov_provisioner_pb_remote_client_provision_timeout \
	prov_device_pb_remote_server_unproved_unresponsive \
	prov_device_unresponsive -- -argstest prov-bearer=1

overlay=overlay_psa_conf
RunTest mesh_prov_pb_remote_provisioning_timeout_psa \
	prov_provisioner_pb_remote_client_provision_timeout \
	prov_device_pb_remote_server_unproved_unresponsive \
	prov_device_unresponsive -- -argstest prov-bearer=1
