#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_transport_seg_fail transport_tx_seg_fail

conf=prj_mesh1d1_conf
RunTest mesh_transport_seg_fail_1d1 transport_tx_seg_fail

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_transport_seg_fail_psa transport_tx_seg_fail
