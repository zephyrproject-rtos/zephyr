#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_prov_pb_adv_repr \
	prov_device_pb_adv_reprovision \
	prov_provisioner_pb_adv_reprovision

overlay=overlay_psa_conf
RunTest mesh_prov_pb_adv_repr_psa \
	prov_device_pb_adv_reprovision \
	prov_provisioner_pb_adv_reprovision
