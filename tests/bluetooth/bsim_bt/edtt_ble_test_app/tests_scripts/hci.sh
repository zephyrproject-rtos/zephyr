#!/usr/bin/env bash
# Copyright 2019 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# HCI regression tests based on the EDTTool
CWD="$(cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P)"

export SIMULATION_ID="edtt_hci"
export TEST_FILE=${CWD}"/hci.test_list"
export TEST_MODULE="hci_verification"

${CWD}/_controller_tests_inner.sh
