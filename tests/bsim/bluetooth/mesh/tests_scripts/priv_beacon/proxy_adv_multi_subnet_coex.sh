#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test Proxy advertisement Coex

# This test verifies correct Proxy advertisement behavior for a device
# where the Proxy adv requirements changes over time, both for single
# and multiple subnets. The TX device is the DUT in this instance, while
# the RX device scans and verifies that the correct proxy adv messages of
# the different subnets is sent within the expected time delta.

# Note 1: The maximum allowed timeslot for a subnet to advertise proxy
# in this scenario is 10 seconds when there is more than one subnet that
# has active proxy adv work. This is reflected in the scanning criteria
# on the RX device.

# Note 2: The expected message received count for each event is based on
# what would be a reasonable/acceptable to receive within a given time
# window. The Mesh Protocol specification does not specify exactly the
# timing for Proxy ADV messages.

# Note 3: The proxy transmitting device mandates emitting of the secure
# network beacons. This allows to check that proxy goes back to normal
# behavior after the device advertises the secure network beacons.

# Test procedure:
# 1. (0-20 seconds) A single subnet is active on the TX device with GATT
#    Proxy enabled. RX device verifies that the single subnet has exclusive
#    access to the adv medium.
# 2. (20-50 seconds) Two additional subnets are added to the TX device. RX
#    device checks that the subnets are sharing the adv medium, advertising
#    NET_ID beacons.
# 3. (50-110 seconds) The second subnet enables Node Identity. RX device
#    checks that NODE_ID is advertised by this subnet, and that the two
#    others continues to advertise NET_ID.
# 4. (110-130 seconds) The first and second subnet gets solicited. RX device
#    checks that PRIVATE_NET_ID is advertised by these subnets.
# 5. (130-150 seconds) The second and third subnet are disabled on the TX
#    device. RX device verifies that the single subnet has exclusive access
#    to the adv medium again.


overlay=overlay_gatt_conf
RunTest proxy_adv_multi_subnet_coex \
	beacon_tx_proxy_adv_multi_subnet_coex \
	beacon_rx_proxy_adv_multi_subnet_coex \
	beacon_tx_proxy_adv_solicit_trigger

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest proxy_adv_multi_subnet_coex \
	beacon_tx_proxy_adv_multi_subnet_coex \
	beacon_rx_proxy_adv_multi_subnet_coex \
	beacon_tx_proxy_adv_solicit_trigger
