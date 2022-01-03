#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test provisioning with OOB authentication and with OOB public key
RunTest mesh_provision_pb_adv_oob_public_key \
	prov_device_pb_adv_oob_public_key prov_provisioner_pb_adv_oob_public_key
