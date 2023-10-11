#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test robustness of NPPI procedures by running them multiple times each. Test procedure:
# 1. Provisioner provisions the second device (prov_device_pb_remote_server) with RPR server through
#    PB-Adv
# 2. RPR client starts to scan for the third device (prov_device_pb_remote_server_nppi_robustness)
#    through RPR (5 second timeout) and provisions it
# 3. Execute device key refresh procedure 3 times for the third device.
# 4. Execute composition refresh procedure 3 times for the third device.
# 5. Execute address refresh procedure 3 times for the third device.
# (Step 3-5 is executed without sending a node reset message)
conf=prj_mesh1d1_conf
RunTest mesh_prov_pb_remote_nppi_robustness \
	prov_provisioner_pb_remote_client_nppi_robustness \
	prov_device_pb_remote_server_unproved \
	prov_device_pb_remote_server_nppi_robustness

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_prov_pb_remote_nppi_robustness_psa \
	prov_provisioner_pb_remote_client_nppi_robustness \
	prov_device_pb_remote_server_unproved \
	prov_device_pb_remote_server_nppi_robustness
