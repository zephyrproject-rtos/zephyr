#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test verifies that mesh app key and CDB implementations keep application key in sync
# during key refresh procedure.
# Adding, deleting and updating keys are performed only over Configuration Client\Server.
#
# Test procedure:
# 1. Provisions DUT.
# 2. Adds application key.
# 3. Checks it is created in CDB.
# 4. Updates primary network and bound application keys to start key refresh procedure
#    on both network and application keys.
# 5. Checks that application key is updated accordingly.
# 6. Puts the key refresh procedure to phase 2 and then to phase 3.
# 7. Checks that CDB application key instance and corresponding application key instance
#    in core have the same values.

RunTest mesh_cdb_appkey_kr cdb_sync_appkey_kr
