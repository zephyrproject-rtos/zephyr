#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim_bt tests

#set -x #uncomment this line for debugging
set -ue

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"
: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be defined}"
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root\
 directory}"

WORK_DIR="${WORK_DIR:-${ZEPHYR_BASE}/bsim_bt_out}"
BOARD="${BOARD:-nrf52_bsim}"
BOARD_ROOT="${BOARD_ROOT:-${ZEPHYR_BASE}}"

mkdir -p ${WORK_DIR}

function compile(){
  local app_root="${app_root:-${ZEPHYR_BASE}}"
  local conf_file="${conf_file:-prj.conf}"
  local cmake_args="${cmake_args:-"-DCONFIG_COVERAGE=y"}"
  local ninja_args="${ninja_args:-""}"

  local exe_name="${exe_name:-bs_${BOARD}_${app}_${conf_file}}"
  local exe_name=${exe_name//\//_}
  local exe_name=${exe_name//./_}
  local exe_name=${BSIM_OUT_PATH}/bin/$exe_name
  local map_file_name=${exe_name}.Tsymbols

  local this_dir=${WORK_DIR}/${app}/${conf_file}

  echo "Building $exe_name"

  # Set INCR_BUILD when calling to only do an incremental build
  if [ ! -v INCR_BUILD ] || [ ! -d "${this_dir}" ]; then
      [ -d "${this_dir}" ] && rm ${this_dir} -rf
      mkdir -p ${this_dir} && cd ${this_dir}
      cmake -GNinja -DBOARD_ROOT=${BOARD_ROOT} -DBOARD=${BOARD} \
            -DCONF_FILE=${conf_file} ${cmake_args} ${app_root}/${app} \
            &> cmake.out || { cat cmake.out && return 0; }
  else
      cd ${this_dir}
  fi
  ninja ${ninja_args} &> ninja.out || { cat ninja.out && return 0; }
  cp ${this_dir}/zephyr/zephyr.exe ${exe_name}

  nm ${exe_name} | grep -v " [U|w] " | sort | cut -d" " -f1,3 > ${map_file_name}
  sed -i "1i $(wc -l ${map_file_name} | cut -d" " -f1)" ${map_file_name}
}

app=tests/bluetooth/bsim_bt/bsim_test_app conf_file=prj_split.conf \
	compile
app=tests/bluetooth/bsim_bt/bsim_test_app conf_file=prj_split_privacy.conf \
  compile
app=tests/bluetooth/bsim_bt/bsim_test_advx compile
app=tests/bluetooth/bsim_bt/edtt_ble_test_app/hci_test_app compile
app=tests/bluetooth/bsim_bt/edtt_ble_test_app/gatt_test_app compile
