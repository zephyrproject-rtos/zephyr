#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test verifies that application key deletion
# over Configuration Client is reflected in CDB.
# Adding and deleting application keys are performed only over
# Configuration Client\Server.
#
# Test procedure:
# 1. Provisions DUT.
# 2. Adds an application key.
# 3. Checks that it is created in CDB.
# 4. Deletes the added application key.
# 5. Checks that there is no added application key in CDB.
# 6. Adds an application key.
# 7. Checks that it is created again in CDB.

RunTest mesh_cdb_appkey_delete cdb_sync_appkey_delete
