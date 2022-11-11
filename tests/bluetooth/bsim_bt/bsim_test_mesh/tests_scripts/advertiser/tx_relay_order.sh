#!/usr/bin/env bash
# Copyright 2022 Lingao Meng
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test the same priority adv order to send.
overlay=overlay_prio_relay_conf
RunTest mesh_adv_relay_send_order adv_tx_relay_send_order adv_rx_relay_receive_order
