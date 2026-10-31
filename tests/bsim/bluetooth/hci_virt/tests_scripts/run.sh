#!/usr/bin/env bash
# Copyright 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

set -eu

source "${ZEPHYR_BASE}/tests/bsim/sh_common.source"

simulation_id="${BOARD_TS}_hci_virt"
verbosity_level=2
controller="./bs_${BOARD_TS}_samples_bluetooth_hci_virt"
host="./bs_${BOARD_TS}_tests_bsim_bluetooth_hci_virt_host"
work_dir="$(mktemp -d /tmp/hci_virt_XXXXXX)"
trap 'rm -rf -- "${work_dir}"' EXIT

# Boot controllers before Hosts by synchronizing at time zero in device-number order.
cd "${BSIM_OUT_PATH}/bin"

Execute "${controller}" -v=${verbosity_level} -s="${simulation_id}" -d=0 -sync_preboot -mro=1000 \
  -hci-socket="${work_dir}/central.sock" -bd-addr=02:00:00:00:00:01
Execute "${controller}" -v=${verbosity_level} -s="${simulation_id}" -d=1 -sync_preboot -mro=1000 \
  -hci-socket="${work_dir}/peripheral.sock" -bd-addr=02:00:00:00:00:02

Execute "${host}" -v=${verbosity_level} -s="${simulation_id}" -d=2 -sync_preboot -mro=1000 \
  -disconnect_on_exit=1 --bt-dev="${work_dir}/peripheral.sock" \
  -testid=peripheral
Execute "${host}" -v=${verbosity_level} -s="${simulation_id}" -d=3 -sync_preboot -mro=1000 \
  -disconnect_on_exit=1 --bt-dev="${work_dir}/central.sock" \
  -testid=central

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" -D=4 -sim_length=2e6 "$@"

wait_for_background_jobs
