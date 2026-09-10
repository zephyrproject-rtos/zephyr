#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the network bsim tests

#set -x #uncomment this line for debugging
set -ue

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

app=samples/net/sockets/echo_server conf_overlay=overlay-802154.conf compile
app=tests/bsim/net/sockets/echo_test conf_overlay=overlay-802154.conf compile

app=samples/net/sockets/echo_server conf_overlay=overlay-ot.conf compile
app=tests/bsim/net/sockets/echo_test conf_overlay=overlay-ot.conf compile

wait_for_background_jobs
