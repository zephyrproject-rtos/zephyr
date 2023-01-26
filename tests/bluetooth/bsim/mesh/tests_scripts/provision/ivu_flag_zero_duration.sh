#!/usr/bin/env bash
# Copyright 2021 Lingao Meng
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_provision_iv_update_zero_duration prov_provisioner_iv_update_flag_zero

conf=prj_mesh1d1_conf
RunTest mesh_provision_iv_update_zero_duration prov_provisioner_iv_update_flag_zero
