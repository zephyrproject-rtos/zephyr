#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Compile with all permutations of a given set of KConfigs
# Specifically for going through possible combinations of
# optional control procedures

#set -x #uncomment this line for debugging
# set DEBUG_PERMUTATE to 'true' for extra debug output
DEBUG_PERMUTATE=false

: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root\
 directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source
BOARD_ROOT="${BOARD_ROOT:-${ZEPHYR_BASE}}"
BOARD_TS="${BOARD//\//_}"

mkdir -p ${WORK_DIR}

declare -a list=(
"CONFIG_BT_CENTRAL="
"CONFIG_BT_PERIPHERAL="
"CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG="
"CONFIG_BT_DATA_LEN_UPDATE="
"CONFIG_BT_PHY_UPDATE="
"CONFIG_BT_CTLR_MIN_USED_CHAN="
"CONFIG_BT_CTLR_LE_PING="
"CONFIG_BT_CTLR_LE_ENC="
"CONFIG_BT_CTLR_CONN_PARAM_REQ="
)

perm_compile() {
    local -a results=()
    # We set a unique exe-name, so that we don't overwrite the executables
    # created by the compile scripts since that may mess up other tests
    # We also delete the executable to avoid having artifacts from
    # a previous run
    local exe_name="bs_${BOARD_TS}_tests_kconfig_perm"
    local executable_name=${exe_name}
    local executable_name=${BSIM_OUT_PATH}/bin/$executable_name

    rm -f ${executable_name}

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
    if test "$DEBUG_PERMUTATE" = "true"; then
	echo "Compile with config overlay:"
	cat $3
    fi
    local app=tests/bsim/bluetooth/ll/edtt/hci_test_app
    local conf_file=prj_dut_llcp.conf
    local conf_overlay=$3
# Note: we need to call '_compile' instead of 'compile' because the latter starts
# compilations in parallel, but here we need to do it in serial since we modify
# the configuration file between each run
    _compile
    if [ ! -f ${executable_name} ]; then
    	compile_failures=$(expr $compile_failures + 1)
    fi
}
let n=${#list[@]}
temp_conf_file=$(mktemp -p ${WORK_DIR})
# compile_failures will be equal to the number of failed compilations
let compile_failures=0

for (( i = 0; i < 2**n; i++ )); do
    ## don't compile for CENTRAL=n AND PERIPHERAL=n
    if (( (i & 0x3) != 0x3 )); then
        perm_compile $n $i ${temp_conf_file}
    fi
done

# We set exit code based on type of failure
# 0 means all configurations compiled w/o error

trap "{ rm "${temp_conf_file}" ; exit 255; }" SIGINT
trap "{ rm "${temp_conf_file}" ; exit 254; }" SIGTERM
trap "{ rm "${temp_conf_file}" ; exit 253; }" ERR
trap "{ rm "${temp_conf_file}" ; exit ${compile_failures}; }" EXIT
