#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test IV index update procedure
RunTest mesh_ivi_update ivi_ivu_normal

overlay=overlay_psa_conf
RunTest mesh_ivi_update_psa ivi_ivu_normal
