#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test tx callbacks sequence for multiple advs
RunTest mesh_adv_tx_cb_multi adv_tx_cb_multi

overlay=overlay_workq_sys_conf
RunTest mesh_adv_tx_cb_multi_workq adv_tx_cb_multi

overlay="overlay_multi_adv_sets_conf"
RunTest mesh_adv_tx_cb_multi_multi_adv_sets adv_tx_cb_multi

overlay=overlay_psa_conf
RunTest mesh_adv_tx_cb_multi_psa adv_tx_cb_multi
