#!/usr/bin/env bash
# Copyright 2021 Lingao Meng
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_prov_iv_update_one_duration prov_provisioner_iv_update_flag_one

conf=prj_mesh1d1_conf
RunTest mesh_prov_iv_update_one_duration_1d1 prov_provisioner_iv_update_flag_one

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_prov_iv_update_one_duration_psa prov_provisioner_iv_update_flag_one
