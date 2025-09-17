#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Precondition:
# setup two device, tx node provisioned node, rx node is not provisioned and only has
# the network creentials.
# Verify:
# tx node is able to adapt observed beacon interval and able to send at least one
# SNB in 600s. And veify tx node doesnn't send SNB faster than 10s.
# Procedure:
# 1- rx node waits for the tx node to send the first SNB,
#  rx node sends two SNBs with 20ms period, verifies that tx node only sends SNB 10s later,
#  after rx node sends out beacon. rx node skips the 3rd SNB and verifies that tx nodes keeps
#  sending SNB #  in 20ms until it adapts again and starts to send in 10s.
# 2- rx node sends SNBs with 4s period for 600s and verifies that tx node doesn't send any
#  SNB until 600s elapses.

RunTest mesh_beacon_interval \
	beacon_tx_secure_beacon_interval \
	beacon_rx_secure_beacon_interval
