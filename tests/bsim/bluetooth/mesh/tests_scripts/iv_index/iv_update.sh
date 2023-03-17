#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test IV index update procedure
RunTest mesh_ivi_update ivi_ivu_normal

conf=prj_mesh1d1_conf
RunTest mesh_ivi_update_1d1 ivi_ivu_normal
