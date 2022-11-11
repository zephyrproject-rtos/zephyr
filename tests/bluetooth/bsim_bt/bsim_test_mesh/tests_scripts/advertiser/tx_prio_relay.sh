#!/usr/bin/env bash
# Copyright 2022 Lingao Meng
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test priority adv preempt low priority adv.
overlay=overlay_prio_relay_conf
RunTest mesh_adv_prio_relay_send adv_tx_prio_relay_send adv_rx_prio_relay_receive
