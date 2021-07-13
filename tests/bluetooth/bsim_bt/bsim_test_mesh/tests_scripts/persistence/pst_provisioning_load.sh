#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# SKIP=(persistence_provisioning_ld_check_load)
RunTest mesh_persistence_check_loading persistence_provisioning_ld_check_load

