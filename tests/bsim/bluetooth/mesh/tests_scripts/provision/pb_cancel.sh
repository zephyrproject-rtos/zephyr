#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that PB-ADV and PB-GATT unprovisioned beacons are cancelled when a provisioning link is
# opened.
# Test procedure:
# 1. The provisioner self-provisions and starts scanning for unprovisioned devices.
# 2. The provisionee enables both the PB-ADV and PB-GATT bearers.
# 3. The provisioner finds the unprovisioned device and provisions it with OOB authentication.
#    The OOB authentication is delayed by CONFIG_BT_MESH_UNPROV_BEACON_INT + 1 seconds to ensure
#    that the provisioner has time observe that PB-ADV and PB-GATT beacons are cancelled.
# 4. The provisioning procedure completes.

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_prov_pb_cancel_adv \
	prov_provisioner_pb_cancel \
	prov_device_pb_cancel \
	-- -argstest prov-bearer=3 prov-to-use=1

overlay=overlay_gatt_conf_overlay_psa_conf
RunTest mesh_prov_pb_cancel_gatt \
	prov_provisioner_pb_cancel \
	prov_device_pb_cancel \
	-- -argstest prov-bearer=3 prov-to-use=2
