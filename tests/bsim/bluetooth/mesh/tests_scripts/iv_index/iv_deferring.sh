#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test deferring of the IV index update procedure
RunTest mesh_ivi_deferring ivi_ivu_deferring

overlay=overlay_psa_conf
RunTest mesh_ivi_deferring_psa ivi_ivu_deferring
