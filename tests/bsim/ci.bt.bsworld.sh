#!/usr/bin/env bash
# Copyright (c) 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

# Build complete nrf52_bsim images, each with its own Controller and radio,
# then run the BSWorld tests in tests/bsim/bluetooth/bsworld against them.
# BSWorld is the "bsworld" west project; enable it with
#   west config manifest.group-filter -- +bsworld && west update bsworld
# It can also be run locally with BSIM_OUT_PATH set.

export ZEPHYR_BASE="${ZEPHYR_BASE:-${PWD}}"
cd "${ZEPHYR_BASE}"

set -uex

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must point to the BabbleSim build output}"
BSWORLD_DIR="${BSWORLD_DIR:-$(west list bsworld -f '{abspath}')}"
OUTDIR="${OUTDIR:-${ZEPHYR_BASE}/twister-out-bsworld}"
tests="${ZEPHYR_BASE}/tests/bsim/bluetooth/bsworld"
export BSWORLD_ZEPHYR_BUILD_DIR="${OUTDIR}/build"

# build <image name> <application> [extra CMake arguments]
build()
{
  local name=$1 app=$2
  shift 2
  west build -p always -b nrf52_bsim/native -d "${BSWORLD_ZEPHYR_BUILD_DIR}/${name}" \
    "${app}" -- "$@"
}

# The default shell with its console on stdio, as bluetooth.native_shell.
build shell tests/bluetooth/shell \
  -DEXTRA_CONF_FILE=xterm-native-shell.conf \
  -DEXTRA_DTC_OVERLAY_FILE=xterm-native-shell.overlay

# Samples are built with their own prj.conf.
for sample in peripheral_hr central_hr; do
  build "${sample}" "samples/bluetooth/${sample}"
done

# Import BSWorld from the west checkout.
export PYTHONPATH="${BSWORLD_DIR}/src${PYTHONPATH:+:${PYTHONPATH}}"
python3 -m pytest -p bsworld.plugin -p no:cacheprovider -c "${tests}/pytest.ini" "${tests}" \
  --out-dir "${OUTDIR}/runtime" --junitxml="${OUTDIR}/bsworld.xml"
