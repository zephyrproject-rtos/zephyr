#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test tx relay sequence for multiple advs
RunTest mesh_adv_tx_send_relay adv_tx_send_relay

overlay="overlay_multi_adv_sets_conf"
RunTest mesh_adv_tx_send_relay_multi_adv_sets adv_tx_send_relay
