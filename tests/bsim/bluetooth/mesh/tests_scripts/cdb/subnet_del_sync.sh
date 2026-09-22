#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test verifies that subnetwork deletion
# over Configuration Client is reflected in CDB.
# Adding and deleting network keys are performed only over
# Configuration Client\Server.
#
# Test procedure:
# 1. Provisions DUT.
# 2. Adds one more subnetwork.
# 3. Checks that it is created in CDB.
# 4. Deletes the added subnetwork.
# 5. Checks that there is no added subnetwork in CDB.
# 6. Adds a subnetwork again.
# 7. Checks that it is created again in CDB.

RunTest mesh_cdb_subnet_delete cdb_sync_subnet_delete
