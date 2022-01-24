#!/usr/bin/env bash
# Copyright 2019 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# LL regression tests based on the EDTTool (part 1)

CWD="$(cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P)"

export SIMULATION_ID="edtt_ll_set1"
export TEST_FILE=${CWD}"/ll.set1.test_list"
export TEST_MODULE="ll_verification"
export PRJ_CONF_1="prj_dut_conf"
export PRJ_CONF_2="prj_tst_conf"

${CWD}/_controller_tests_inner.sh
