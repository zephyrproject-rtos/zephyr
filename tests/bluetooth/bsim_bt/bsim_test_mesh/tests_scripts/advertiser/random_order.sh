#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# test buffer management by filling buffers and sending them in random order.
RunTest mesh_adv_random_order adv_tx_random_order adv_rx_random_order
