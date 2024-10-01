#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test Private over GATT connection
#
# Test procedure:
# 0. Both TX and RX device disables SNB and GATT proxy, and enables
#    Private GATT proxy. Test mode for IV update is also enabled on both devices.
# 1. The RX device (Proxy CLI) establish a GATT connection to the TX device
#    (Proxy SRV), using Private Network Identity.
# 2. Both TX and RX device disables the scanner to prevent interferance
#    by the adv bearer.
# 3. The TX device (Proxy SRV) starts an IV update procedure.
# 4. Both TX and RX device verifies that the IV index has been updated.
#    This proves that the RX device (Proxy CLI) successfully received
#    a Private beacon over the GATT connection
overlay=overlay_gatt_conf
RunTest mesh_priv_proxy_gatt_priv_beacon \
	beacon_tx_priv_gatt_proxy \
	beacon_rx_priv_gatt_proxy

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_priv_proxy_gatt_priv_beacon \
	beacon_tx_priv_gatt_proxy \
	beacon_rx_priv_gatt_proxy
