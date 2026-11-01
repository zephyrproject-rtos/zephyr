#!/usr/bin/env bash
# Copyright (c) 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

# Build the nrf52_bsim twister scenarios that the BSWorld tests in
# tests/bsim/bluetooth/bsworld run, each image with its own Controller and
# radio, then run the tests against them.
# BSWorld is installed from scripts/requirements-actions.txt. The script can
# also be run locally with BSIM_OUT_PATH set.

export ZEPHYR_BASE="${ZEPHYR_BASE:-${PWD}}"
cd "${ZEPHYR_BASE}"

set -uex

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must point to the BabbleSim build output}"
OUTDIR="${OUTDIR:-${ZEPHYR_BASE}/twister-out-bsworld}"
tests="${ZEPHYR_BASE}/tests/bsim/bluetooth/bsworld"
export BSWORLD_ZEPHYR_BUILD_DIR="${OUTDIR}/build"

# The default shell with its console on stdio.
scenarios=(-T tests/bluetooth/shell -s bluetooth.native_shell)
# The samples with their own prj.conf.
scenarios+=(-T samples/bluetooth/peripheral_hr -s sample.bluetooth.peripheral_hr)
scenarios+=(-T samples/bluetooth/central_hr -s sample.bluetooth.central_hr.bsim)

scripts/twister -p nrf52_bsim/native --build-only --clobber-output --inline-logs \
  -O "${BSWORLD_ZEPHYR_BUILD_DIR}" "${scenarios[@]}"

python3 -m pytest -p no:cacheprovider -c "${tests}/pytest.ini" "${tests}" \
  --out-dir "${OUTDIR}/runtime" --junitxml="${OUTDIR}/bsworld.xml"
