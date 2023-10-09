#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that the composition data page 1 (CDP1) is encoded correctly. The
# composition consists of model extensions within and between elements,
# extensions requiring long and short formats, and correspondence between
# a SIG and vendor model.
#
# Test procedure:
# 0. Provisioning and setup.
# 1. Configuration client requests the node's CDP1.
# 2. The received CDP1 is compared to a hardcoded version.
conf=prj_mesh1d1_conf
RunTest mesh_cdp1_test \
	cdp1_node_data_comparison

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_cdp1_test_psa \
	cdp1_node_data_comparison
