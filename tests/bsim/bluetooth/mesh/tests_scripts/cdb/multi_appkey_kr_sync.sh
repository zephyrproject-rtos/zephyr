#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test verifies that multiple application keys are synchronized between
# mesh and CDB during the key refresh procedure.
# Adding, deleting and updating keys are performed only over Configuration Client\Server.
#
# Test procedure:
# 1. Provisions DUT.
# 2. Adds two application keys bound to the primary subnet.
# 3. Checks that they are created in CDB.
# 4. Updates primary network key and application keys to trigger the key refresh procedure.
# 5. Checks that the application keys are updated in CDB.
# 6. Puts the key refresh procedure to phase 2 and then to phase 3.
# 7. Checks that CDB application key instances and corresponding application keys in core
#    have the same values.

RunTest mesh_cdb_multi_appkey_kr cdb_sync_multiple_appkeys_kr
