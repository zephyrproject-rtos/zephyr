#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test provisioning with OOB authentication, with device OOB public key
# but provisioner doesn't have OOB public key
RunTest mesh_prov_pb_adv_device_w_oob_pk_prvnr_wt_pk \
	prov_device_oob_public_key prov_provisioner_oob_auth_no_oob_public_key \
	-- -argstest prov-bearer=1

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_prov_pb_gatt_device_w_oob_pk_prvnr_wt_pk \
	prov_device_oob_public_key prov_provisioner_oob_auth_no_oob_public_key \
	-- -argstest prov-bearer=2

overlay=overlay_psa_conf
RunTest mesh_prov_pb_adv_device_w_oob_pk_prvnr_wt_pk_psa \
	prov_device_oob_public_key prov_provisioner_oob_auth_no_oob_public_key \
	-- -argstest prov-bearer=1
