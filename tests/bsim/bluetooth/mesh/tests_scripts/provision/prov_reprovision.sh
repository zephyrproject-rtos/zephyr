#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_prov_pb_adv_repr \
	prov_device_reprovision \
	prov_provisioner_reprovision \
	-- -argstest prov-bearer=1

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_prov_pb_gatt_repr \
	prov_device_reprovision \
	prov_provisioner_reprovision \
	-- -argstest prov-bearer=2

overlay=overlay_psa_conf
RunTest mesh_prov_pb_adv_repr_psa \
	prov_device_reprovision \
	prov_provisioner_reprovision \
	-- -argstest prov-bearer=1
