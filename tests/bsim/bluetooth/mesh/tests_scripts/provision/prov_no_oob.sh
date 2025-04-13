#!/usr/bin/env bash
# Copyright 2021 Lingao Meng
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_prov_pb_adv_on_oob \
	prov_device_no_oob \
	prov_provisioner_no_oob \
	-- -argstest prov-bearer=1

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_prov_pb_gatt_on_oob \
	prov_device_no_oob \
	prov_provisioner_no_oob \
	-- -argstest prov-bearer=2

overlay=overlay_psa_conf
RunTest mesh_prov_pb_adv_on_oob_psa \
	prov_device_no_oob \
	prov_provisioner_no_oob \
	-- -argstest prov-bearer=1
