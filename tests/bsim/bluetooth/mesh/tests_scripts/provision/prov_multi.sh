#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Provision 3 devices in succession:
# Note that the number of devices must match the
# PROV_MULTI_COUNT define in test_provision.c
RunTest mesh_prov_pb_adv_multi \
	prov_provisioner_multi \
	prov_device_no_oob \
	prov_device_no_oob \
	prov_device_no_oob \
	-- -argstest prov-brearer=1

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_prov_pb_gatt_multi \
	prov_provisioner_multi \
	prov_device_no_oob \
	prov_device_no_oob \
	prov_device_no_oob \
	-- -argstest prov-brearer=2

overlay=overlay_psa_conf
RunTest mesh_prov_pb_adv_multi_psa \
	prov_provisioner_multi \
	prov_device_no_oob \
	prov_device_no_oob \
	prov_device_no_oob \
	-- -argstest prov-brearer=1
