#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test tx callbacks parameters and xmit sequence for single adv
RunTest mesh_adv_tx_cb_single adv_tx_cb_single adv_rx_xmit

conf=prj_mesh1d1_conf
RunTest mesh_adv_tx_cb_single_1d1 adv_tx_cb_single adv_rx_xmit
