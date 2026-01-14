#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test verifies that mesh subnet and CDB implementations keep network key in sync
# during key refresh procedure.
# Adding, deleting and updating keys are performed only over Configuration Client\Server.
#
# Test procedure:
# 1. Provisions DUT.
# 2. Checks that the primary subnet is created.
# 3. Updates primary network key to start key refresh procedure.
# 4. Checks that CDB subnet key refresh phase and key are updated accordingly.
# 5. Puts the key refresh procedure to phase 2.
# 6. Checks CDB instance phase.
# 7. Completes key refresh procedure.
# 8. Checks that CDB subnetwork instance and primary network keys have the same values.

RunTest mesh_cdb_subnet_kr cdb_sync_subnet_kr
