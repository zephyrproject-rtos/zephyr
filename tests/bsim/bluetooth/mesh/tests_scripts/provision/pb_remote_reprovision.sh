#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test a node re-provisioning through Remote Provisioning models. Procedure:
# 1. A provisioner with the RPR client provisions itself;
# 2. The provisioner provisions a device with the RPR server;
# 3. The provisioner starts scanning for an unprovisioned device through the node with RPR server;
# 4. The provisioner finds an unprovisioned device and provisions it;
# 5. The provisioner configures the health server on the recently provisioned device and sends Node
#   Reset;
# 6. Repeat steps 3-5 multiple times.
conf=prj_mesh1d1_conf
RunTest mesh_prov_pb_remote_reprovision \
	prov_provisioner_pb_remote_client_reprovision \
	prov_device_pb_remote_server_unproved \
	prov_device_pb_adv_reprovision

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_prov_pb_remote_reprovision_psa \
	prov_provisioner_pb_remote_client_reprovision \
	prov_device_pb_remote_server_unproved \
	prov_device_pb_adv_reprovision
