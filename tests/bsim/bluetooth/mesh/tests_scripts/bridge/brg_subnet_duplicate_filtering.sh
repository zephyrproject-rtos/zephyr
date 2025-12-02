#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# This test verifies that Subnet Bridge returns only unique subnet pairs in the Bridged Subnets
# List message, and that the start index is indexing into the list of unique pairs (not the entire
# briding table)
#
# Test procedure:
# 1. Tester configures itself and creates a number of subnets.
# 2. Tester provisions and configures Subnet Bridge node with multiple rows per subnet pair.
# 3. Tester verifies that the Bridged Subnets List message does not contain duplicate subnet
#    pairs and that start indexes index into the filtered Bridged Subnets List. This is done
#    for each filter type.

RunTest mesh_brg_subnet_duplicate_filtering \
	brg_tester_subnet_duplicate_filtering brg_bridge_simple
