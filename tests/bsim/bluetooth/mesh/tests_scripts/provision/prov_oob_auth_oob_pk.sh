#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test provisioning with OOB authentication and with OOB public key
RunTest mesh_prov_pb_adv_oob_public_key \
	prov_device_oob_public_key prov_provisioner_oob_public_key \
	-- -argstest prov-brearer=1

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_prov_pb_gatt_oob_public_key \
	prov_device_oob_public_key prov_provisioner_oob_public_key \
	-- -argstest prov-brearer=2

overlay=overlay_psa_conf
RunTest mesh_prov_pb_adv_oob_public_key_psa \
	prov_device_oob_public_key prov_provisioner_oob_public_key \
	-- -argstest prov-brearer=1
