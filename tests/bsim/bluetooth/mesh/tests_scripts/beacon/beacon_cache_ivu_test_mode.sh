#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that a cached beacon is reprocessed once IV Update test mode lifts the 96-hour limit
#
# Test procedure:
# 0. RX device starts monitoring all accepted SNB messages, with IV Update test mode off.
# 1. TX device sends an SNB with the IV Update flag set. RX device authenticates and caches it,
#    but refuses the IV Update because of the 96-hour limit.
# 2. RX device enables IV Update test mode, which lifts that limit.
# 3. TX device sends an identical SNB. RX device verifies that it was not filtered out as a
#    duplicate, and that the IV Update was performed.
# 4. RX device disables IV Update test mode.
# 5. TX device sends the identical SNB twice more. RX device verifies that only the first of
#    the two was processed, i.e. that duplicate filtering still works.
RunTest mesh_beacon_cache_ivu_test_mode \
	beacon_tx_beacon_cache_ivu_test_mode \
	beacon_rx_beacon_cache_ivu_test_mode
