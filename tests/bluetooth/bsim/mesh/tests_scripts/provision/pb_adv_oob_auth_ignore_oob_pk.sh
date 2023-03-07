#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test provisioning with OOB authentication, with device OOB public key
# but provisioner doesn't have OOB public key
RunTest mesh_prov_pb_adv_device_w_oob_pk_prvnr_wt_pk \
	prov_device_pb_adv_oob_public_key prov_provisioner_pb_adv_oob_auth_no_oob_public_key

conf=prj_mesh1d1_conf
RunTest mesh_prov_pb_adv_device_w_oob_pk_prvnr_wt_pk_1d1 \
	prov_device_pb_adv_oob_public_key prov_provisioner_pb_adv_oob_auth_no_oob_public_key
