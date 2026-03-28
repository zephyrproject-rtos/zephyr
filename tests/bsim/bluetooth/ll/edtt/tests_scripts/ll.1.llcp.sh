#!/usr/bin/env bash
# Copyright 2019 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# LL regression tests based on the EDTTool (part 1)

CWD="$(cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P)"

export SIMULATION_ID="edtt_ll_set1_llcp"
export TEST_FILE=${CWD}"/ll.set1.llcp.test_list"
export TEST_MODULE="ll_verification"
export PRJ_CONF_1="prj_dut_llcp_conf"
export PRJ_CONF_2="prj_tst_llcp_conf"

${CWD}/_controller_tests_inner.sh
