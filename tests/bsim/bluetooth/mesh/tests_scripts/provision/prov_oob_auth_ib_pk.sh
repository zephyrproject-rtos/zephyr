#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test provisioning with OOB authentication and with inband public key
RunTest mesh_prov_pb_adv_oob_auth \
	prov_device_oob_auth prov_provisioner_oob_auth \
	-- -argstest prov-bearer=1

overlay=overlay_gatt_conf
RunTest mesh_prov_pb_gatt_oob_auth \
	prov_device_oob_auth prov_provisioner_oob_auth \
	-- -argstest prov-bearer=2
