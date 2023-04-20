#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test deferring of the IV index update procedure
RunTest mesh_ivi_deferring ivi_ivu_deferring

conf=prj_mesh1d1_conf
RunTest mesh_ivi_deferring_1d1 ivi_ivu_deferring
