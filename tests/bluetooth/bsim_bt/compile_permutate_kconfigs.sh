#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Compile with all permutations of a given set of KConfigs
# Specifically for going through possible combinations of
# optional control procedures

#set -x #uncomment this line for debugging

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"
: "${BSIM_COMPONENTS_PATH:?BSIM_COMPONENTS_PATH must be defined}"
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root\
 directory}"

WORK_DIR="${WORK_DIR:-${ZEPHYR_BASE}/bsim_bt_out}"
BOARD="${BOARD:-nrf52_bsim}"
BOARD_ROOT="${BOARD_ROOT:-${ZEPHYR_BASE}}"

mkdir -p ${WORK_DIR}

source ${ZEPHYR_BASE}/tests/bluetooth/bsim_bt/compile.source


declare -a list=(
"CONFIG_BT_DATA_LEN_UPDATE="
"CONFIG_BT_PHY_UPDATE="
"CONFIG_BT_CTLR_MIN_USED_CHAN="
"CONFIG_BT_CTLR_LE_PING="
"CONFIG_BT_CTLR_LE_ENC="
#"CONFIG_BT_CTLR_CONN_PARAM_REQ="
)

perm_compile() {
    local -a results=()
    let idx=$2
    for (( j = 0; j < $1; j++ )); do
        if (( idx % 2 )); then
           results=("${results[@]}" "${list[$j]}n")
        else
           results=("${results[@]}" "${list[$j]}y")
        fi
        let idx\>\>=1
    done
    printf '%s\n' "${results[@]}" > $3
    echo "Compile with config overlay:"
    cat $3
    app=tests/bluetooth/bsim_bt/edtt_ble_test_app/hci_test_app conf_file=prj.conf conf_overlay=$3 \
      compile
}
let n=${#list[@]}
temp_conf_file=$(mktemp -p ${WORK_DIR})
for (( i = 1; i < 2**n; i++ )); do
    perm_compile $n $i ${temp_conf_file}
done

trap "{ rm "${temp_conf_file}" ; exit 255; }" SIGINT SIGTERM ERR EXIT

