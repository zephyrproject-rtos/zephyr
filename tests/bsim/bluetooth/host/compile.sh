#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

#set -x #uncomment this line for debugging
set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

#Set a default value to BOARD if it does not have one yet
BOARD="${BOARD:-nrf52_bsim/native}"

west twister -T ${ZEPHYR_BASE}/tests/bsim/bluetooth/host/ -p ${BOARD}
