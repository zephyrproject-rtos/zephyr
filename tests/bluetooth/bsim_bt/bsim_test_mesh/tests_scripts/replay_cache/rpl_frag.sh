#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test fragmentation of RPL:
# 1. Send a message from 3 different consecutive nodes starting from address 100;
# 2. Toggle IV index update;
# 3. Send a new message from even addresses. This should update IVI index of RPL for these nodes.
#   The RPL entry odd address should stay unchanged;
# 4. Complete IVI Update;
# 5. Repeate steps 2 - 4 to remove RPL entry with odd address from RPL and cause fragmentation;
overlay=overlay_pst_conf
RunTest mesh_replay_fragmentation rpc_rx_rpl_frag rpc_tx_rpl_frag

# Simulate reboot and test that RPL entries are restored correctly after defragmentation
overlay=overlay_pst_conf
RunTest mesh_replay_fragmentation rpc_rx_reboot_after_defrag
